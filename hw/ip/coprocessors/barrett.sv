// Copyright 2025 EDGE Group, @VLSI Lab - Politecnico di Torino                        //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Barrett Reduction Unit                                                 //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Barrett reduction for ML-KEM, HQC, and ML-DSA modular multiplication.  //
//               Supports Z₃₃₂₉ (ML-KEM), Z₈₃₈₀₄₁₇ (ML-DSA), and HQC-1/3/5 variants.  //
//               2-stage pipeline: stage 1 estimates the quotient (a*mu shift-add +     //
//               shift), stage 2 computes the final reduction from the registered       //
//               quotient. a/insn_i are held stable across both cycles by id_stage's    //
//               pipe_ctx_hold, so only the quotient needs to be registered.            //
///////////////////////////////////////////////////////////////////////////////////////////

module barrett (
    input  logic                      clk_i,
    input  logic                      rst_ni,
    input  logic signed [31:0]        a,
    input  logic signed [31:0]        b,
    input  horcrux_pkg::horcrux_insn  insn_i,
    output logic signed [31:0]        result
);

    // ------------------ Kyber Barrett constants ------------------
    localparam logic signed [31:0] Q_kyber      = 32'sd3329;
    localparam logic signed [31:0] Q_kyber_half = 32'sd1665;

    // ------------------ HQC Barrett constants ------------------
    localparam logic [31:0] HQC1_N = 32'd17669;
    localparam logic [31:0] HQC3_N = 32'd35851;
    localparam logic [31:0] HQC5_N = 32'd57637;

    // =========================================================================
    // STAGE 1 (combinational): quotient estimate = floor(a * mu / 2^k)
    // =========================================================================
    logic signed [31:0] t1, t2, t3, t4, t5, t6, t7, t8, t9;
    logic signed [31:0] ta;
    logic signed [31:0] quot_kyber;

    logic [31:0] hqc_x;
    logic [63:0] hqc_mu_sum;
    logic [31:0] quot_hqc;

    always_comb begin
        t1 = '0; t2 = '0; t3 = '0; t4 = '0; t5 = '0;
        t6 = '0; t7 = '0; t8 = '0; t9 = '0;
        ta = '0;
        quot_kyber = '0;
        hqc_x      = $unsigned(a);
        hqc_mu_sum = '0;
        quot_hqc   = '0;

        unique case (insn_i)

            // -----------------------------------------------------
            // Kyber Barrett Reduction - quotient estimate
            // -----------------------------------------------------
            horcrux_pkg::OP_BFINTTK,
            horcrux_pkg::OP_BARRETT: begin
                t1 = a;              // 1·a
                t2 = a << 1;         // 2·a
                t3 = a << 2;         // 4·a
                t4 = a << 3;         // 8·a
                t5 = a << 5;         // 32·a
                t6 = a << 7;         // 128·a
                t7 = a << 8;         // 256·a
                t8 = a << 9;         // 512·a
                t9 = a << 12;        // 4096·a

                ta = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8 + t9;
                quot_kyber = ta >>> 24;      // floor(ta / 2^24)
            end

            // -----------------------------------------------------
            // HQC-128 Barrett (N=17669, mu=243079) - quotient estimate
            // -----------------------------------------------------
            horcrux_pkg::OP_BARRETT_HQC: begin
                // mu = 2^17 + 2^16 + 2^15 + 2^13 + 2^12 + 2^10 + 2^8 + 2^7 + 2^2 + 2^1 + 1
                hqc_mu_sum = (64'(hqc_x) << 17) + (64'(hqc_x) << 16) + (64'(hqc_x) << 15) +
                             (64'(hqc_x) << 13) + (64'(hqc_x) << 12) + (64'(hqc_x) << 10) +
                             (64'(hqc_x) << 8)  + (64'(hqc_x) << 7)  + (64'(hqc_x) << 2)  +
                             (64'(hqc_x) << 1)  + 64'(hqc_x);

                quot_hqc = hqc_mu_sum[63:32]; // >> 32
            end

            // -----------------------------------------------------
            // HQC-192 Barrett (N=35851, mu=119800) - quotient estimate
            // -----------------------------------------------------
            horcrux_pkg::OP_BARRETT_HQC3: begin
                // mu = 2^16 + 2^15 + 2^14 + 2^12 + 2^9 + 2^8 + 2^7 + 2^6 + 2^5 + 2^4 + 2^3
                hqc_mu_sum = (64'(hqc_x) << 16) + (64'(hqc_x) << 15) + (64'(hqc_x) << 14) +
                             (64'(hqc_x) << 12) + (64'(hqc_x) << 9)  + (64'(hqc_x) << 8)  +
                             (64'(hqc_x) << 7)  + (64'(hqc_x) << 6)  + (64'(hqc_x) << 5)  +
                             (64'(hqc_x) << 4)  + (64'(hqc_x) << 3);

                quot_hqc = hqc_mu_sum[63:32];
            end

            // -----------------------------------------------------
            // HQC-256 Barrett (N=57637, mu=74517) - quotient estimate
            // -----------------------------------------------------
            horcrux_pkg::OP_BARRETT_HQC5: begin
                // mu = 2^16 + 2^13 + 2^9 + 2^8 + 2^4 + 2^2 + 1
                hqc_mu_sum = (64'(hqc_x) << 16) + (64'(hqc_x) << 13) + (64'(hqc_x) << 9) +
                             (64'(hqc_x) << 8)  + (64'(hqc_x) << 4)  + (64'(hqc_x) << 2) +
                             64'(hqc_x);

                quot_hqc = hqc_mu_sum[63:32];
            end

            default: begin
            end

        endcase
    end

    // =========================================================================
    // PIPELINE REGISTER
    // Cuts the combinational chain between the quotient estimate (phase 1,
    // dominated by the 9/11-term shift-add tree) and the final reduction
    // (phase 2, another shift-add tree + subtract + compare-correct).
    // Registered unconditionally every cycle, mirroring full_prod_q/mode_q
    // in shared_multiplication_logic.sv. Every current caller (OP_BFINTTK,
    // OP_BARRETT, OP_BARRETT_HQC/_HQC3/_HQC5) needs the 2-cycle path, so
    // there is no combinational bypass mux to maintain here.
    // =========================================================================
    logic signed [31:0] quot_kyber_q;
    logic [31:0]         quot_hqc_q;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            quot_kyber_q <= '0;
            quot_hqc_q   <= '0;
        end else begin
            quot_kyber_q <= quot_kyber;
            quot_hqc_q   <= quot_hqc;
        end
    end

    // =========================================================================
    // STAGE 2 (combinational): final reduction from the registered quotient.
    // a/insn_i are read live: id_stage's pipe_ctx_hold keeps rs_values_o and
    // select_insn_o (and therefore a/insn_i here) stable across both cycles
    // of a pipelined instruction, so no separate register is needed for them.
    // =========================================================================
    logic signed [31:0] m_kyber, z_kyber;
    logic [31:0] hqc_n_sum, hqc_r;

    always_comb begin
        result  = '0;
        m_kyber = '0;
        z_kyber = '0;
        hqc_n_sum = '0;
        hqc_r      = '0;

        unique case (insn_i)

            horcrux_pkg::OP_BFINTTK,
            horcrux_pkg::OP_BARRETT: begin
                m_kyber = (quot_kyber_q <<< 11) + (quot_kyber_q <<< 10) +
                          (quot_kyber_q <<< 8)  + quot_kyber_q;
                z_kyber = a - m_kyber;

                if (z_kyber >= Q_kyber_half)
                    z_kyber = z_kyber - Q_kyber;

                result = z_kyber;
            end

            horcrux_pkg::OP_BARRETT_HQC: begin
                // n = 2^14 + 2^10 + 2^8 + 2^2 + 1
                hqc_n_sum = (quot_hqc_q << 14) + (quot_hqc_q << 10) + (quot_hqc_q << 8) +
                            (quot_hqc_q << 2)  + quot_hqc_q;

                hqc_r = $unsigned(a) - hqc_n_sum;

                if (hqc_r >= HQC1_N)
                    hqc_r = hqc_r - HQC1_N;

                result = $signed(hqc_r);
            end

            horcrux_pkg::OP_BARRETT_HQC3: begin
                // n = 2^15 + 2^11 + 2^10 + 2^3 + 2^1 + 1
                hqc_n_sum = (quot_hqc_q << 15) + (quot_hqc_q << 11) + (quot_hqc_q << 10) +
                            (quot_hqc_q << 3)  + (quot_hqc_q << 1)  + quot_hqc_q;

                hqc_r = $unsigned(a) - hqc_n_sum;

                if (hqc_r >= HQC3_N)
                    hqc_r = hqc_r - HQC3_N;

                result = $signed(hqc_r);
            end

            horcrux_pkg::OP_BARRETT_HQC5: begin
                // n = 2^15 + 2^14 + 2^13 + 2^8 + 2^5 + 2^2 + 1
                hqc_n_sum = (quot_hqc_q << 15) + (quot_hqc_q << 14) + (quot_hqc_q << 13) +
                            (quot_hqc_q << 8)  + (quot_hqc_q << 5)  + (quot_hqc_q << 2) +
                            quot_hqc_q;

                hqc_r = $unsigned(a) - hqc_n_sum;

                if (hqc_r >= HQC5_N)
                    hqc_r = hqc_r - HQC5_N;

                result = $signed(hqc_r);
            end

            default: begin
                result = '0;
            end

        endcase
    end

endmodule
