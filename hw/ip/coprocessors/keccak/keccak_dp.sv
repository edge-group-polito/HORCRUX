// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Keccak Data Path                                                       //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Keccak-f[1600] datapath with state register and round function        //
//               pipeline. Based on design by M. Peeters and G. Van Assche.            //
///////////////////////////////////////////////////////////////////////////////////////////

import pkg_keccak::k_state;
import pkg_keccak::KEC_N;
import pkg_keccak::IN_BUF_SIZE;
import pkg_keccak::OUT_BUF_SIZE;

module keccak_dp (
    input clk,
    input rst_n,  // asynchronous, active-low
    input start_i,   // pulse: start a new permutation
    input [1600-1:0] Din,
    output ready_o,   // ’1’ when permutation finished
    output [1600-1:0] Dout);


    //--------------------------------------------------------------------
    //  Internal signals
    //--------------------------------------------------------------------

    k_state               reg_data;       // state register
    k_state               round_in;
    k_state               round_out;

    logic        [4:0]            counter_nr_rounds;
    logic        [7:0]      round_constant_signal;
    logic                       compute_permutation;
    logic                       permutation_computed;

    //--------------------------------------------------------------------
    //  Sub-blocks
    //--------------------------------------------------------------------
    keccak_round round_map (
        .Round_in              ( round_in              ),
        .Round_constant_signal ( round_constant_signal ),
        .Round_out             ( round_out             )
    );

    keccak_round_constants_gen round_constants_gen (
        .round_number              ( counter_nr_rounds   ),
        .round_constant_signal_out ( round_constant_signal )
    );

    //--------------------------------------------------------------------
    //  Flatten reg_data → Dout  (combinational)
    //--------------------------------------------------------------------
    genvar y, x, i;
    generate
        for (y = 0; y < 5; y++) begin : g_y
            for (x = 0; x < 5; x++) begin : g_x
                for (i = 0; i < 64; i++) begin : g_bit
                    assign Dout[320*y + 64*x + i] = reg_data[y][x][i];
                end
            end
        end
    endgenerate

    //--------------------------------------------------------------------
    //  Main FSM / state register
    //--------------------------------------------------------------------
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            // asynchronous clear
            reg_data              <= '0;
            counter_nr_rounds     <= '0;
            permutation_computed  <= 1'b1;
            compute_permutation   <= 1'b0;
        end
         else begin
            if (start_i) begin
                // load zero state and arm the core
                reg_data              <= '0;
                counter_nr_rounds     <= '0;
                compute_permutation   <= 1'b1;
                permutation_computed  <= 1'b1;
            end
            else begin
                // normal operation
                if (compute_permutation && permutation_computed) begin
                    // start round 0
                    counter_nr_rounds      <= 5'b00001;
                    permutation_computed   <= 1'b0;
                    reg_data               <= round_out;
                end
                else begin
                    if ((counter_nr_rounds < 24) && !permutation_computed) begin
                        counter_nr_rounds  <= counter_nr_rounds + 1'b1;
                        reg_data           <= round_out;
                    end
                    if (counter_nr_rounds == 23) begin
                        permutation_computed <= 1'b1;   // done!
                        compute_permutation  <= 1'b0;
                        counter_nr_rounds    <= '0;
                    end
                end
            end
        end
    end

    //--------------------------------------------------------------------
    //  Build round_in = current_state ⊕ Din  (only while idle)
    //--------------------------------------------------------------------
    generate
        for (y = 0; y < 5; y++) begin : g_y2
            for (x = 0; x < 5; x++) begin : g_x2
                for (i = 0; i < 64; i++) begin : g_bit2
                    // XOR the rate input *only* when permutation_computed == 1
                    assign round_in[y][x][i] =
                        reg_data[y][x][i] ^
                        ( Din[320*y + 64*x + i] & permutation_computed );
                end
            end
        end
    endgenerate

    //--------------------------------------------------------------------
    //  Output flag
    //--------------------------------------------------------------------
    assign ready_o = permutation_computed;

endmodule

