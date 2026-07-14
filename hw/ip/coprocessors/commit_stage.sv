// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Instruction Result Commit Stage                                        //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Routes coprocessor execution results to the CORE-V XIF interface       //
//               with proper result handshake and register write control.               //
///////////////////////////////////////////////////////////////////////////////////////////

module commit_stage
  import horcrux_pkg::*;
(
  input logic clk_i,
  input logic rst_ni,

  input logic            [4:0] rd_i,
  input logic            [3:0] id_i,
  input horcrux_pkg::out_t     result_i,
  input logic                  done_i,

  if_xif.coproc_result xif_result_if

);

  always_comb begin
    xif_result_if.result_valid   = '0;
    xif_result_if.result.we      = '0;
    xif_result_if.result.rd      = '0;
    xif_result_if.result.id      = '0;

    xif_result_if.result.ecsdata = '0;
    xif_result_if.result.exc     = '0;
    xif_result_if.result.ecswe   = '0;
    xif_result_if.result.exccode = '0;
    xif_result_if.result.err     = '0;
    xif_result_if.result.dbg     = '0;
    xif_result_if.result.data    = result_i.rd1;

    if (done_i) begin
      xif_result_if.result_valid = '1;
      xif_result_if.result.we    = done_i;
      xif_result_if.result.rd    = rd_i;
      xif_result_if.result.id    = id_i;
    end
  end

endmodule


