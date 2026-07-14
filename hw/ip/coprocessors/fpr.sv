//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Falcon Floating-Point Representation (FPR) Unit                       //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Falcon FPR packing, normalization, and associated operations.          //
//               Supports stateful FPR_EXEC and FPR_NORM64_EXEC with two-stage muxing. //
//////////////////////////////////////////////////////////////////////////////////////////


module fpr (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic [31:0] data1_i,  // rs1
    input  logic [31:0] data2_i,  // rs2
    input  logic [31:0] data3_i,  // rs3
    input  horcrux_pkg::horcrux_insn insn_i,
    output logic [31:0] result_o,
    output logic [31:0] result_hi_o
);
    import horcrux_pkg::*;

    // Stateful FPR packing (Falcon): stage-1 stores sign/exponent, stage-2 executes with mantissa.
    logic        fpr_s_reg;
    logic [31:0] fpr_e_reg;
    logic [31:0] fpr_hi_reg;
    logic [63:0] fpr_result_full;

    // Stateful FPR_NORM64 datapath (LZC + barrel shifter)
    logic [63:0] fpr_norm64_m_in;
    logic [31:0] fpr_norm64_e_in;
    logic [6:0]  fpr_norm64_lzc;
    logic [5:0]  fpr_norm64_shamt;
    logic [63:0] fpr_norm64_m_out;
    logic [31:0] fpr_norm64_e_out;
    logic [31:0] fpr_norm64_m_hi_reg;
    logic [31:0] fpr_norm64_e_reg;

    function automatic logic [63:0] fpr_pack64(
        input logic s_i,
        input logic [31:0] e_i,
        input logic [63:0] m_i
    );
        logic [63:0] x;
        logic [63:0] m;
        logic [31:0] e;
        logic [31:0] t;
        logic [31:0] f;
        logic [31:0] neg_t;
        begin
            m = m_i;
            e = e_i + 32'd1076;

            // Clamp to zero when original exponent was below -1076.
            t = e >> 31;
            m = m & ({32'b0, t} - 64'd1);

            // If mantissa is zero then force exponent to zero too.
            t = m[63:54];
            neg_t = (~t) + 32'd1;
            e = e & neg_t;

            x = ({63'b0, s_i} << 63) | (m >> 2);
            x = x + (({32'b0, e}) << 52);

            // Round-to-nearest, ties-to-even using the low three mantissa bits.
            f = {29'b0, m[2:0]};
            x = x + ((32'h000000C8 >> f) & 32'h1);

            fpr_pack64 = x;
        end
    endfunction

    // Leading-zero count for the 64-bit mantissa.
    always_comb begin
        fpr_norm64_lzc = 7'd64;
        for (int i = 63; i >= 0; i--) begin
            if (fpr_norm64_m_in[i] && (fpr_norm64_lzc == 7'd64)) begin
                fpr_norm64_lzc = 7'(63 - i);
            end
        end
    end

    assign fpr_result_full = fpr_pack64(fpr_s_reg, fpr_e_reg, {data2_i, data1_i});
    assign fpr_norm64_m_in = {data2_i, data1_i};
    assign fpr_norm64_e_in = data3_i;

    // Keep behavior aligned with Falcon macro: if m==0 then m_out=0 and e_out=e_in-63.
    assign fpr_norm64_shamt = (fpr_norm64_lzc == 7'd64) ? 6'd63 : fpr_norm64_lzc[5:0];
    assign fpr_norm64_m_out = fpr_norm64_m_in << fpr_norm64_shamt;
    assign fpr_norm64_e_out = fpr_norm64_e_in - {26'b0, fpr_norm64_shamt};

    always_comb begin
        result_o = 32'b0;
        result_hi_o = 32'b0;

        case (insn_i)
            OP_FPR_LOAD_SE: begin
                result_o = 32'b0;
            end

            OP_FPR_EXEC: begin
                result_o    = fpr_result_full[31:0];
                result_hi_o = fpr_result_full[63:32];
            end

            OP_FPR_RDHI: begin
                result_o = fpr_hi_reg;
            end

            OP_FPR_NORM64_EXEC: begin
                result_o = fpr_norm64_m_out[31:0];
            end

            OP_FPR_NORM64_RDHI: begin
                result_o = fpr_norm64_m_hi_reg;
            end

            OP_FPR_NORM64_RDE: begin
                result_o = fpr_norm64_e_reg;
            end

            default: begin
                result_o = 32'b0;
                result_hi_o = 32'b0;
            end
        endcase
    end

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            fpr_s_reg <= 1'b0;
            fpr_e_reg <= '0;
            fpr_hi_reg <= '0;
            fpr_norm64_m_hi_reg <= '0;
            fpr_norm64_e_reg <= '0;
        end else begin
            case (insn_i)
                OP_FPR_LOAD_SE: begin
                    fpr_s_reg <= data1_i[0];
                    fpr_e_reg <= data2_i;
                end

                OP_FPR_EXEC: begin
                    fpr_hi_reg <= fpr_result_full[63:32];
                end

                OP_FPR_NORM64_EXEC: begin
                    fpr_norm64_m_hi_reg <= fpr_norm64_m_out[63:32];
                    fpr_norm64_e_reg <= fpr_norm64_e_out;
                end

                default: begin
                    // Keep registers stable
                end
            endcase
        end
    end

endmodule
