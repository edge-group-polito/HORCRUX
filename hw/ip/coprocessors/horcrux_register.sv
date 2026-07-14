//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  HORCRUX Internal Register File                                         //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  50×32-bit register file with XOR absorption path for Keccak           //
//               sponge construction and multi-operand write support.                   //
//////////////////////////////////////////////////////////////////////////////////////////


module horcrux_register #(
    parameter int unsigned NUM_VARIABLES   = 50,
    parameter int unsigned VARIABLE_WIDTH  = 32
) (
    input  logic                                          clk_i,
    input  logic                                          rst_ni,
    input  logic [31:0]                                   horcrux_register_in0_i,
    input  logic [31:0]                                   horcrux_register_in1_i,
    input  logic                                          horcrux_register_enable_i,
    input  logic                                          horcrux_register_xor_enable_i,
    input  logic                                          horcrux_register_init_i,
    input  logic                                          horcrux_register_wb_enable_i,
    input  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0]  horcrux_register_wb_data_i,
    input  logic [5:0]                                    horcrux_register_index_i,
    output logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0]  horcrux_register_o
);

    // Define register with 50 variables, each 32 bits wide
    logic [VARIABLE_WIDTH-1:0] register_array [0:NUM_VARIABLES-1];



    always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
        register_array <= '{default: '0};
    end else if (horcrux_register_init_i) begin
        register_array <= '{default: '0};
    end else if (horcrux_register_wb_enable_i) begin
        for (int i = 0; i < NUM_VARIABLES; i++) begin
            register_array[i] <= horcrux_register_wb_data_i[i];
        end
    end else if (horcrux_register_enable_i) begin
        register_array[horcrux_register_index_i]     <= horcrux_register_in0_i;
        register_array[horcrux_register_index_i + 1] <= horcrux_register_in1_i;
    end else if (horcrux_register_xor_enable_i) begin
        register_array[horcrux_register_index_i]     <= register_array[horcrux_register_index_i]     ^ horcrux_register_in0_i;
        register_array[horcrux_register_index_i + 1] <= register_array[horcrux_register_index_i + 1] ^ horcrux_register_in1_i;
    end
    end

    // Pack array into a single vector: highest index becomes MSBs (same order as your manual concat)
    always_comb begin
    horcrux_register_o = '0;
    for (int i = 0; i < NUM_VARIABLES; i++) begin
        horcrux_register_o[i] = register_array[i];
    end
    end

endmodule
