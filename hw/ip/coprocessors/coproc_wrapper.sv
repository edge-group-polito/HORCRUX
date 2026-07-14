// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  HORCRUX Coprocessor Wrapper                                            //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Top-level wrapper connecting CV32E40Px CORE-V XIF interface to the     //
//               HORCRUX coprocessor with compressed/issue/commit/mem/result signals.  //
///////////////////////////////////////////////////////////////////////////////////////////

module coproc_wrapper
  import cv32e40px_pkg::*;
  import cv32e40px_core_v_xif_pkg::*;
(
  input logic clk_i,
  input logic rst_ni,

  // eXtension interface
  if_xif.coproc_compressed xif_compressed_if,
  if_xif.coproc_issue      xif_issue_if,
  if_xif.coproc_commit     xif_commit_if,
  if_xif.coproc_mem        xif_mem_if,
  if_xif.coproc_mem_result xif_mem_result_if,
  if_xif.coproc_result     xif_result_if
);


  horcrux_top horcrux_top_i (
    .clk_i (clk_i),
    .rst_ni(rst_ni),

    .xif_compressed_if,
    .xif_issue_if,
    .xif_commit_if,
    .xif_mem_if,
    .xif_mem_result_if,
    .xif_result_if
  );

endmodule

