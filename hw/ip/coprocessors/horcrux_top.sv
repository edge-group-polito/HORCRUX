//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  HORCRUX-X-Heep Interface Wrapper                                      //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Bridge between CV32E40Px core (via CORE-V XIF) and HORCRUX             //
//               coprocessor, handling instruction compression and result routing.     //
//////////////////////////////////////////////////////////////////////////////////////////



module horcrux_top
  import cv32e40px_pkg::*;
  import cv32e40px_core_v_xif_pkg::*;
(
  input clk_i,
  input rst_ni,

  if_xif.coproc_compressed xif_compressed_if,
  if_xif.coproc_issue      xif_issue_if,
  if_xif.coproc_commit     xif_commit_if,
  if_xif.coproc_mem        xif_mem_if,
  if_xif.coproc_mem_result xif_mem_result_if,
  if_xif.coproc_result     xif_result_if
);


  horcrux_pkg::in_t       rs_values;
  horcrux_pkg::out_t      rd_values;
  //result_to_commit;
  horcrux_pkg::horcrux_insn select_insn;
  logic [4:0] rd_idex, rd_excom;
  logic [3:0] id_idex, id_excom;
  logic       save_rd;
  logic       done;

  logic      OP_KSTART;
  logic      keccak_done_o;
  logic      sphincs_done_o;

  //not used
  //*****COMPRESSED INTERFACE*********************************************
  assign xif_compressed_if.compressed_ready = '0;
  assign xif_compressed_if.compressed_resp  = '0;
  //*****MEMORY INTERFACE**************************************************
  assign xif_mem_if.mem_valid               = '0;
  assign xif_mem_if.mem_req                 = '0;


  id_stage id_stage_i (
    .clk_i          (clk_i),
    .rst_ni         (rst_ni),
    .xif_issue_if   (xif_issue_if),
    .keccak_done_i  (keccak_done_o),
    .sphincs_done_i (sphincs_done_o),

    .rs_values_o    (rs_values),
    .rd_o           (rd_idex),
    .id_o           (id_idex),
    .select_insn_o  (select_insn),
    .save_rd_o      (save_rd),
    .done_o         (done)
  );

  assign OP_KSTART = (select_insn == horcrux_pkg::OP_KSTART) ||
                        (select_insn == horcrux_pkg::OP_KPERM);

  horcrux  #(
    .NUM_VARIABLES  (50),
    .VARIABLE_WIDTH (32)
  ) i_horcrux (
    .clk_i          (clk_i),
    .rst_ni         (rst_ni),
    .rs_values_i    (rs_values),
    .insn_i         (select_insn),
    .start_keccak_i (OP_KSTART),
    .done_keccak_o  (keccak_done_o),
    .done_sphincs_o (sphincs_done_o),
    .rd_values_o    (rd_values)
  );


  generate
    always @(posedge clk_i or negedge rst_ni) begin
      if (~rst_ni) begin
        rd_excom <= '0;
        id_excom <= '0;
      end else begin
        if (save_rd) begin
          rd_excom <= rd_idex;
          id_excom <= id_idex;
        end else begin
          rd_excom <= rd_excom;
          id_excom <= id_excom;
        end
      end
    end
  endgenerate

  //assign result_to_commit.rd1 = rd_values.rd1;
  //assign result_to_commit.rd2 = rd_values.rd2;


  commit_stage commit_stage_i (
    .clk_i          (clk_i),
    .rst_ni         (rst_ni),
    .rd_i           (rd_excom),
    .id_i           (id_excom),
    .result_i       (rd_values),
    .done_i         (done),
    .xif_result_if  (xif_result_if)
  );

endmodule
