// Copyright 2025 EDGE Group, @VLSI Lab - Politecnico di Torino                        //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Shared Multiplication Logic                                            //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Unified Montgomery multiplication for ML-KEM, ML-DSA, and Falcon.      //
//               Resource-optimized with single auxiliary multiplier and muxed Q.      //
//               3-stage pipeline breaks the 3-multiply Montgomery chain for NTT/INTT  //
//               modes: cycle N computes primary a*b (-> full_prod_q), cycle N+1       //
//               computes the stage_a multiply (-> t_value_q), cycle N+2 computes      //
//               stage_b (the 3rd multiply) and the final combine/reduce.              //
///////////////////////////////////////////////////////////////////////////////////////////

module shared_multiplication_logic (
    input  logic        clk_i,
    input  logic        rst_ni,
    // pipe_en_i = 1: insert register between primary multiply and reduction stages.
    // Must be driven HIGH for Montgomery modes (DSA/KEM/Falcon/MODP) and LOW for
    // carryless (HQC), RED32, and Barrett paths which must remain single-cycle.
    input  logic        pipe_en_i,
    input  logic [31:0] op_a_i,
    input  logic [31:0] op_b_i,
    input  logic [31:0] modp_p_i,
    input  logic [31:0] modp_p0i_i,
    input  logic [2:0]  mode_i,
    output logic [31:0] res_lo_o,
    output logic [31:0] res_hi_o
);

    // =========================================================================
    // CONSTANTS
    // =========================================================================
    localparam logic [31:0] KEM_Q     = 32'd3329;
    localparam logic [15:0] KEM_QINV  = 16'hF301;
    localparam logic [31:0] DSA_Q     = 32'd8380417;
    localparam logic [31:0] DSA_QINV  = 32'd58728449;
    localparam logic [31:0] FALCON_Q  = 32'd12289;
    localparam logic [15:0] FAL_Q0I   = 16'd12287;

    // Mode encodings
    localparam logic [2:0] MODE_DSA_MONT  = 3'd0;
    localparam logic [2:0] MODE_KEM_MONT  = 3'd1;
    localparam logic [2:0] MODE_FAL_MONT  = 3'd2;
    localparam logic [2:0] MODE_HQC_RAW   = 3'd4;
    localparam logic [2:0] MODE_DSA_RED32 = 3'd5;
    localparam logic [2:0] MODE_MODP_MONT = 3'd6;

    // =========================================================================
    // PRIMARY MULTIPLIER: op_a * op_b
    // The regular and carry-less products are returned separately rather than
    // muxed inside the multiplier: an internal mux would create a false path
    // for STA, since only one of the two can ever reach the output.
    // =========================================================================
    logic [63:0] full_prod;
    logic [63:0] mul_regular_prod;
    logic [63:0] mul_clmul_prod;

    unified_mul_32x32 u_primary_mul (
        .a_i            (op_a_i),
        .b_i            (op_b_i),
        .regular_prod_o (mul_regular_prod),
        .clmul_prod_o   (mul_clmul_prod)
    );

    assign full_prod = mul_regular_prod;

    // =========================================================================
    // PIPELINE REGISTER
    // Breaks the critical path: primary multiply in cycle N, reduction in N+1.
    // Registered unconditionally, with no live-value bypass mux: the modes
    // that do not go through the pipeline (RED32, Karatsuba, HQC_RAW) never
    // read full_prod, so a bypass would only add a false path to the output.
    // =========================================================================
    logic [63:0] full_prod_q;
    logic [2:0]  mode_q;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            full_prod_q <= '0;
            mode_q      <= '0;
        end else begin
            full_prod_q <= full_prod;
            mode_q      <= mode_i;
        end
    end

    logic [2:0] mode_use;
    assign mode_use = pipe_en_i ? mode_q : mode_i;

    // =========================================================================
    // SHARED AUXILIARY MULTIPLIER with operand muxing
    //
    // Stage A: t = (prod_low * QINV)   — uses full_prod_q
    // Stage B: t * Q                   — uses t from stage A
    // =========================================================================

    // --- Stage A: Compute t = (prod_low * QINV) ---
    logic [31:0] stage_a_op1, stage_a_op2;
    logic [63:0] stage_a_result;
    logic [31:0] t_value;

    // stage_a reads the registered full_prod_q/mode_q unconditionally. The modes
    // that bypass the pipeline never use the stage_a result, so a live-value mux
    // here would only expose the combinational multiplier as a false path.
    always_comb begin
        unique case (mode_q)
            MODE_KEM_MONT:  stage_a_op1 = {16'b0, full_prod_q[15:0]};
            MODE_DSA_MONT:  stage_a_op1 = full_prod_q[31:0];
            MODE_FAL_MONT:  stage_a_op1 = full_prod_q[31:0];
            MODE_MODP_MONT: stage_a_op1 = {1'b0, full_prod_q[30:0]};
            default:        stage_a_op1 = '0;
        endcase
    end

    // stage_a_op2 and t_value use the registered mode_q for the same reason:
    // decoding from the live mode_i would run the instruction decode into the
    // stage_a DSP and the modp chain within one cycle.
    always_comb begin
        unique case (mode_q)
            MODE_KEM_MONT:  stage_a_op2 = {16'b0, KEM_QINV};
            MODE_DSA_MONT:  stage_a_op2 = DSA_QINV;
            MODE_FAL_MONT:  stage_a_op2 = {16'b0, FAL_Q0I};
            MODE_MODP_MONT: stage_a_op2 = modp_p0i_i;
            default:        stage_a_op2 = '0;
        endcase
    end

    assign stage_a_result = stage_a_op1 * stage_a_op2;

    always_comb begin
        unique case (mode_q)
            MODE_KEM_MONT:  t_value = {{16{stage_a_result[15]}}, stage_a_result[15:0]};
            MODE_DSA_MONT:  t_value = stage_a_result[31:0];
            MODE_FAL_MONT:  t_value = {16'b0, stage_a_result[15:0]};
            MODE_MODP_MONT: t_value = {1'b0, stage_a_result[30:0]};
            default:        t_value = '0;
        endcase
    end

    // =========================================================================
    // SECOND PIPELINE REGISTER
    // Breaks the remaining chain: stage_a multiply in cycle N+1, stage_b plus
    // the final combine in cycle N+2. t_value already depends only on
    // registered signals, so it is the natural cut point.
    // =========================================================================
    logic [31:0] t_value_q;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            t_value_q <= '0;
        end else begin
            t_value_q <= t_value;
        end
    end

    // --- Stage B: Compute t * Q ---
    logic [31:0] stage_b_op1, stage_b_op2;
    logic signed [63:0] stage_b_result;

    logic signed [31:0] reduce32_t;
    assign reduce32_t = ($signed(op_a_i) + 32'sd4194304) >>> 23;

    always_comb begin
        unique case (mode_use)
            MODE_DSA_RED32: stage_b_op1 = reduce32_t;
            MODE_FAL_MONT:  stage_b_op1 = t_value_q;
            MODE_MODP_MONT: stage_b_op1 = t_value_q;
            default:        stage_b_op1 = t_value_q;
        endcase
    end

    always_comb begin
        unique case (mode_use)
            MODE_KEM_MONT:   stage_b_op2 = KEM_Q;
            MODE_DSA_MONT:   stage_b_op2 = DSA_Q;
            MODE_FAL_MONT:   stage_b_op2 = FALCON_Q;
            MODE_MODP_MONT:  stage_b_op2 = modp_p_i;
            MODE_DSA_RED32:  stage_b_op2 = DSA_Q;
            default:         stage_b_op2 = '0;
        endcase
    end

    assign stage_b_result = $signed({{32{stage_b_op1[31]}}, stage_b_op1}) *
                            $signed({{32{1'b0}}, stage_b_op2});

    logic [63:0] modp_tp;
    assign modp_tp = {32'b0, t_value_q} * {32'b0, modp_p_i};

    // =========================================================================
    // SHARED SUBTRACTOR
    // =========================================================================
    logic signed [63:0] minuend;
    logic signed [63:0] subtrahend;
    logic signed [63:0] difference;

    always_comb begin
        unique case (mode_use)
            MODE_DSA_RED32: minuend = $signed({{32{op_a_i[31]}}, op_a_i});
            default:        minuend = $signed(full_prod_q);
        endcase
    end

    assign subtrahend = stage_b_result;
    assign difference = minuend - subtrahend;

    // =========================================================================
    // FALCON SPECIAL PATH
    // =========================================================================
    logic [31:0] falcon_w;
    logic [31:0] falcon_sum;
    logic [31:0] falcon_shifted;
    logic [31:0] falcon_final;
    logic [63:0] modp_sum;
    logic [31:0] modp_shifted;
    logic [31:0] modp_sub;
    logic [31:0] modp_final;

    assign falcon_w       = stage_b_result[31:0];
    assign falcon_sum     = full_prod_q[31:0] + falcon_w;
    assign falcon_shifted = falcon_sum >> 16;
    assign falcon_final   = (falcon_shifted >= FALCON_Q) ?
                            (falcon_shifted - FALCON_Q) : falcon_shifted;

    logic [63:0] modp_z;
    assign modp_z       = {2'b0, full_prod_q[61:0]};
    assign modp_sum     = modp_z + modp_tp;
    assign modp_shifted = modp_sum[62:31];
    assign modp_sub     = modp_shifted[31:0] - modp_p_i;
    assign modp_final   = modp_sub + (modp_p_i & {32{modp_sub[31]}});

    // =========================================================================
    // OUTPUT MUX
    // =========================================================================
    always_comb begin
        res_lo_o = '0;
        res_hi_o = '0;

        unique case (mode_use)
            MODE_KEM_MONT: begin
                res_lo_o = difference[47:16];
            end
            MODE_DSA_MONT: begin
                res_lo_o = difference[63:32];
            end
            MODE_FAL_MONT: begin
                res_lo_o = falcon_final;
            end
            MODE_HQC_RAW: begin
                // Reads mul_clmul_prod directly (not full_prod_q/full_prod):
                // this path is always single-cycle regardless of pipe_en_i,
                // and routing through the registered full_prod_q would give
                // a one-cycle-stale (wrong) value here instead of a false
                // path, since HQC_RAW is genuinely never pipelined.
                res_lo_o = mul_clmul_prod[31:0];
                res_hi_o = mul_clmul_prod[63:32];
            end
            MODE_MODP_MONT: begin
                res_lo_o = modp_final;
                res_hi_o = 32'b0;
            end
            MODE_DSA_RED32: begin
                res_lo_o = difference[31:0];
            end
            default: begin
                res_lo_o = full_prod_q[31:0];
            end
        endcase
    end

endmodule
