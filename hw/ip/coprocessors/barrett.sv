// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
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
///////////////////////////////////////////////////////////////////////////////////////////

module barrett (
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

    // Intermediate signals for Kyber logic
    logic signed [31:0] t1, t2, t3, t4, t5, t6, t7, t8, t9;
    logic signed [31:0] ta, tb;
    logic signed [31:0] m_kyber;
    logic signed [31:0] z_kyber;

    // Intermediate signals for HQC logic
    logic [31:0] hqc_x;
    logic [31:0] hqc_q;
    logic [31:0] hqc_r;
    logic [63:0] hqc_mu_sum;
    logic [31:0] hqc_n_sum;

    always_comb begin
        // Default values
        result      = '0;
        t1          = '0; t2 = '0; t3 = '0; t4 = '0; t5 = '0;
        t6          = '0; t7 = '0; t8 = '0; t9 = '0;
        ta          = '0; tb = '0;
        m_kyber     = '0;
        z_kyber     = '0;
        hqc_x       = $unsigned(a);
        hqc_q       = '0;
        hqc_r       = '0;
        hqc_mu_sum  = '0;
        hqc_n_sum   = '0;

        unique case (insn_i)

            // -----------------------------------------------------
            // Kyber Barrett Reduction
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
                tb = ta >>> 24;      // floor(ta / 2^24)
                
                m_kyber = (tb << 11) + (tb << 10) + (tb << 8) + tb;
                z_kyber = a - m_kyber;
                
                if (z_kyber >= Q_kyber_half)
                    z_kyber = z_kyber - Q_kyber;

                result = z_kyber;
            end

            // -----------------------------------------------------
            // HQC-128 Barrett (N=17669, mu=243079)
            // -----------------------------------------------------
            horcrux_pkg::OP_BARRETT_HQC: begin
                // mu = 2^17 + 2^16 + 2^15 + 2^13 + 2^12 + 2^10 + 2^8 + 2^7 + 2^2 + 2^1 + 1
                hqc_mu_sum = (64'(hqc_x) << 17) + (64'(hqc_x) << 16) + (64'(hqc_x) << 15) + 
                             (64'(hqc_x) << 13) + (64'(hqc_x) << 12) + (64'(hqc_x) << 10) + 
                             (64'(hqc_x) << 8)  + (64'(hqc_x) << 7)  + (64'(hqc_x) << 2)  + 
                             (64'(hqc_x) << 1)  + 64'(hqc_x);
                
                hqc_q = hqc_mu_sum[63:32]; // >> 32

                // n = 2^14 + 2^10 + 2^8 + 2^2 + 1
                hqc_n_sum = (hqc_q << 14) + (hqc_q << 10) + (hqc_q << 8) + (hqc_q << 2) + hqc_q;
                
                hqc_r = hqc_x - hqc_n_sum;

                if (hqc_r >= HQC1_N)
                    hqc_r = hqc_r - HQC1_N;

                result = $signed(hqc_r);
            end

            // -----------------------------------------------------
            // HQC-192 Barrett (N=35851, mu=119800)
            // -----------------------------------------------------
            horcrux_pkg::OP_BARRETT_HQC3: begin
                // mu = 2^16 + 2^15 + 2^14 + 2^12 + 2^9 + 2^8 + 2^7 + 2^6 + 2^5 + 2^4 + 2^3
                hqc_mu_sum = (64'(hqc_x) << 16) + (64'(hqc_x) << 15) + (64'(hqc_x) << 14) + 
                             (64'(hqc_x) << 12) + (64'(hqc_x) << 9)  + (64'(hqc_x) << 8)  + 
                             (64'(hqc_x) << 7)  + (64'(hqc_x) << 6)  + (64'(hqc_x) << 5)  + 
                             (64'(hqc_x) << 4)  + (64'(hqc_x) << 3);
                
                hqc_q = hqc_mu_sum[63:32];

                // n = 2^15 + 2^11 + 2^10 + 2^3 + 2^1 + 1
                hqc_n_sum = (hqc_q << 15) + (hqc_q << 11) + (hqc_q << 10) + (hqc_q << 3) + 
                            (hqc_q << 1)  + hqc_q;
                
                hqc_r = hqc_x - hqc_n_sum;

                if (hqc_r >= HQC3_N)
                    hqc_r = hqc_r - HQC3_N;

                result = $signed(hqc_r);
            end

            // -----------------------------------------------------
            // HQC-256 Barrett (N=57637, mu=74517)
            // -----------------------------------------------------
            horcrux_pkg::OP_BARRETT_HQC5: begin
                // mu = 2^16 + 2^13 + 2^9 + 2^8 + 2^4 + 2^2 + 1
                hqc_mu_sum = (64'(hqc_x) << 16) + (64'(hqc_x) << 13) + (64'(hqc_x) << 9) + 
                             (64'(hqc_x) << 8)  + (64'(hqc_x) << 4)  + (64'(hqc_x) << 2) + 
                             64'(hqc_x);
                
                hqc_q = hqc_mu_sum[63:32];

                // n = 2^15 + 2^14 + 2^13 + 2^8 + 2^5 + 2^2 + 1
                hqc_n_sum = (hqc_q << 15) + (hqc_q << 14) + (hqc_q << 13) + (hqc_q << 8) + 
                            (hqc_q << 5)  + (hqc_q << 2)  + hqc_q;
                
                hqc_r = hqc_x - hqc_n_sum;

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
