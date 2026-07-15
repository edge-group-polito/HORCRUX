//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                        //
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

module unified_mul_32x32 (
    input  logic [31:0] a_i,
    input  logic [31:0] b_i,
    input  logic        carryless_mode_i, 
    output logic [63:0] prod_o
);
    logic [63:0] accumulator;
    logic [63:0] partial_products [31:0];

    always_comb begin
        accumulator = '0;
        for (int i = 0; i < 32; i++) begin
            if (a_i[i]) begin
                if (carryless_mode_i)
                    partial_products[i] = {32'b0, b_i} << i;
                else
                    partial_products[i] = 64'(signed'(b_i)) << i;
            end else begin
                partial_products[i] = '0;
            end

            if (carryless_mode_i)
                accumulator ^= partial_products[i];
            else begin
                if (i < 31) accumulator += partial_products[i];
                else        accumulator -= partial_products[i]; // Signed MSB
            end
        end
        prod_o = accumulator;
    end
endmodule
