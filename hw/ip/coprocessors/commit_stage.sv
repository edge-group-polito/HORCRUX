// Copyright 2025 EDGE Group, @VLSI Lab - Politecnico di Torino                        //
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
//               For pipelined NTT/INTT instructions done_i arrives one cycle later     //
//               than for single-cycle instructions (WAIT_PIPE stall in id_stage).      //
//               result_i.rd1 is valid at whichever cycle done_i is asserted, so this   //
//               module requires no structural change — only result.data is now driven  //
//               strictly inside the done_i guard to prevent stale data leaking onto    //
//               the XIF bus during the pipeline settling cycle.                        //
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
    xif_result_if.result.data    = '0;
    xif_result_if.result.ecsdata = '0;
    xif_result_if.result.exc     = '0;
    xif_result_if.result.ecswe   = '0;
    xif_result_if.result.exccode = '0;
    xif_result_if.result.err     = '0;
    xif_result_if.result.dbg     = '0;

    if (done_i) begin
      xif_result_if.result_valid = '1;
      xif_result_if.result.we    = 1'b1;
      xif_result_if.result.rd    = rd_i;
      xif_result_if.result.id    = id_i;
      // For pipelined instructions this cycle is WAIT_PIPE: result_i.rd1 is
      // computed from full_prod_q (the pipeline register) and is valid here.
      xif_result_if.result.data  = result_i.rd1;
    end
  end

endmodule


