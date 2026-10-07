//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 EDGE Group, @VLSI Lab - Politecnico di Torino                         //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                 //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                     //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Unified 32x32 Multiplier                                               //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                             //
//                                                                                      //
// Description:  Dual-mode 32x32 multiplier, core of the Shared Multiplication Logic.   //
//               Switches between carry-propagating integer multiplication (lattice     //
//               schemes) and carry-free GF(2) accumulation (HQC) via carryless_mode_i. //
//////////////////////////////////////////////////////////////////////////////////////////


// Both products are exposed separately instead of being muxed internally: the
// caller already selects between them, and an internal mux would add a false
// path for static timing analysis.
module unified_mul_32x32 (
    input  logic [31:0] a_i,
    input  logic [31:0] b_i,
    output logic [63:0] regular_prod_o,
    output logic [63:0] clmul_prod_o
);

    // --------------------------------------------------------
    // Signal Declarations
    // --------------------------------------------------------
    logic [63:0] regular_prod;
    logic [63:0] clmul_prod;

    logic [15:0] a_h;
    logic [15:0] a_l;
    logic [15:0] b_h;
    logic [15:0] b_l;
    logic [15:0] a_xor;
    logic [15:0] b_xor;

    logic [31:0] m0_clmul; 
    logic [31:0] m1_clmul; 
    logic [31:0] m2_clmul; 

    // --------------------------------------------------------
    // Regular signed 32x32 multiply 
    // (Vivado automatically decomposes this into DSPs)
    // --------------------------------------------------------
    assign regular_prod = $signed({{32{a_i[31]}}, a_i}) *
                          $signed({{32{b_i[31]}}, b_i});

    // --------------------------------------------------------
    // Carryless 32x32 multiply via Karatsuba (Pure Combinational)
    // --------------------------------------------------------
    assign a_h = a_i[31:16];
    assign a_l = a_i[15:0];
    assign b_h = b_i[31:16];
    assign b_l = b_i[15:0];

    assign a_xor = a_h ^ a_l;
    assign b_xor = b_h ^ b_l;

    always_comb begin
        m0_clmul = '0;
        m1_clmul = '0;
        m2_clmul = '0;

        // Three 16x16 Carryless Multipliers
        // Logic depth is reduced compared to a monolithic 32x32 block
        for (int i = 0; i < 16; i++) begin
            if (a_l[i])   m0_clmul ^= ({16'b0, b_l} << i);
            if (a_h[i])   m1_clmul ^= ({16'b0, b_h} << i);
            if (a_xor[i]) m2_clmul ^= ({16'b0, b_xor} << i);
        end
    end

    // Karatsuba Reconstruction: 
    // Result = (M1 << 32) ^ ((M0 ^ M1 ^ M2) << 16) ^ M0
    assign clmul_prod = {m1_clmul, 32'b0} ^
                        ({32'b0, (m0_clmul ^ m1_clmul ^ m2_clmul)} << 16) ^
                        {32'b0, m0_clmul};

    // --------------------------------------------------------
    // Outputs (no mux: see module header comment)
    // --------------------------------------------------------
    assign regular_prod_o = regular_prod;
    assign clmul_prod_o   = clmul_prod;

endmodule