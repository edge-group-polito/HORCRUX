//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  HORCRUX Top-level Controller                                          //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Main controller module orchestrating instruction decode, issue,        //
//               and result routing across all functional units (Keccak, multiplier,   //
//               sampler, SPHINCS+, Falcon FPR, and custom reductions).                //
//////////////////////////////////////////////////////////////////////////////////////////


module horcrux
  import cv32e40px_core_v_xif_pkg::*;
  import horcrux_pkg::*;
#(
    parameter int unsigned NUM_VARIABLES   = 50,
    parameter int unsigned VARIABLE_WIDTH  = 32
) (
  input logic clk_i,
  input logic rst_ni,

  input  in_t         rs_values_i,
  input  horcrux_insn insn_i,
  input  logic        start_keccak_i,
  output logic        done_keccak_o,
  output logic        done_sphincs_o,
  output out_t        rd_values_o
);


  //--------always-present signals declarations ----------------------------------------------------------------------------
  horcrux_pkg::out_t out;
  logic [31:0] a1, b1, c1;
  logic       horcrux_register_enable_i;
  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0] register_array;

  //-------- Keccak signals declarations ----------------------------------------------------------------------------
  logic [1599:0] keccak_input, keccak_result;
  logic          keccak_status;
  logic          KSTART;
  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0] keccak_register_array;

  //-------- New absorption/permute signals ----------------------------------------------------------
  logic horcrux_register_xor_enable;
  logic horcrux_register_init;
  logic keccak_wb_pending;
  logic keccak_writeback;

  //-------- SPHINCS+ unified signals ------------------------------------------------
  logic        sphincs_start_keccak;
  logic [1599:0] sphincs_keccak_input;
  logic        sphincs_wb_enable;
  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0] sphincs_wb_data;
  logic        sphincs_active;
  logic        sphincs_done;

  //-------- input declarations ---------------------------------------------------------------------------
  assign a1 = rs_values_i.rs1;
  assign b1 = rs_values_i.rs2;
  assign c1 = rs_values_i.rs3;

  assign horcrux_register_enable_i  = (insn_i == OP_LOAD);
  assign horcrux_register_xor_enable = (insn_i == OP_KABSORB);
  assign horcrux_register_init       = (insn_i == OP_INIT);
  
  // Keccak start: from normal permute OR from sphincs module
  assign KSTART = start_keccak_i | sphincs_start_keccak;

  // Track whether current keccak operation should write back to register file
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      keccak_wb_pending <= 1'b0;
    else if (insn_i == OP_KPERM)
      keccak_wb_pending <= 1'b1;
    else if (done_keccak_o)
      keccak_wb_pending <= 1'b0;
  end

  // Writeback enable: one cycle pulse when keccak finishes in permute mode
  assign keccak_writeback = done_keccak_o & keccak_wb_pending;

  // Combined writeback: from normal keccak permute OR from sphincs module
  logic combined_wb_enable;
  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0] combined_wb_data;
  assign combined_wb_enable = keccak_writeback | sphincs_wb_enable;
  
  // Mux for writeback data
  always_comb begin
    if (sphincs_wb_enable)
      combined_wb_data = sphincs_wb_data;
    else
      combined_wb_data = keccak_register_array;
  end

  //-------- blocks instantiations ---------------------------------------------------------------------------
  horcrux_register #(
    .NUM_VARIABLES  (NUM_VARIABLES),
    .VARIABLE_WIDTH (VARIABLE_WIDTH)
  ) horcrux_register_inst (
    .clk_i                        (clk_i),
    .rst_ni                       (rst_ni),
    .horcrux_register_in0_i       (a1),
    .horcrux_register_in1_i       (b1),
    .horcrux_register_enable_i    (horcrux_register_enable_i),
    .horcrux_register_xor_enable_i(horcrux_register_xor_enable),
    .horcrux_register_init_i      (horcrux_register_init),
    .horcrux_register_wb_enable_i (combined_wb_enable),
    .horcrux_register_wb_data_i   (combined_wb_data),
    .horcrux_register_index_i     (c1[5:0]),
    .horcrux_register_o           (register_array)
  );

  //--------  Keccak ----------------------------------------------------------------------------
  //logic [31:0] bcop32_result;
  //logic [31:0] rol32_1_result, rol32_2_result;
  //bcop32 bcop32_inst (
  //  .a     (a1),
  //  .b     (b1),
  //  .c     (c1),
  //  .result(bcop32_result)
  //);
  //rol32 rol32_inst (
  //  .high   (b1),
  //  .low    (a1),
  //  .offset (c1),
  //  .result1(rol32_1_result),
  //  .result2(rol32_2_result)
  //);
  
  
  //assign keccak_input          = register_array;
  // Mux keccak_input: when sphincs is active, use its constructed state
  logic [1599:0] register_array_flat;
  assign register_array_flat = {register_array[50], register_array[49], register_array[48], register_array[47], register_array[46], register_array[45], register_array[44], register_array[43], register_array[42], register_array[41], register_array[40], register_array[39], register_array[38], register_array[37], register_array[36], register_array[35], register_array[34], register_array[33], register_array[32], register_array[31], register_array[30], register_array[29], register_array[28], register_array[27], register_array[26], register_array[25], register_array[24], register_array[23], register_array[22], register_array[21], register_array[20], register_array[19], register_array[18], register_array[17], register_array[16], register_array[15], register_array[14], register_array[13], register_array[12], register_array[11], register_array[10], register_array[9], register_array[8], register_array[7], register_array[6], register_array[5], register_array[4], register_array[3], register_array[2], register_array[1], register_array[0]};
  
  // Mux for keccak input
  always_comb begin
    if (sphincs_active)
      keccak_input = sphincs_keccak_input;
    else
      keccak_input = register_array_flat;
  end


  keccak_f keccak_i (
    .clk          (clk_i),
    .rst_n        (rst_ni),
    .start_i      (KSTART),
    .Din          (keccak_input),
    .Dout         (keccak_result),
    .status_d     (keccak_status),
    .keccak_intr  (done_keccak_o)
  );

  //assign keccak_register_array = keccak_result;
  always_comb begin
    for (int i = 0; i < NUM_VARIABLES; i++) begin
      keccak_register_array[i] = keccak_result[i*VARIABLE_WIDTH +: VARIABLE_WIDTH];
    end
  end

  //-------- Chain Lengths (SPHINCS+ WOTS+) ---------------------------------------------------
  logic [31:0] cl_store_result;
  logic        cl_active;

  chain_lengths #(
    .NUM_VARIABLES  (NUM_VARIABLES),
    .VARIABLE_WIDTH (VARIABLE_WIDTH)
  ) chain_lengths_inst (
    .clk_i            (clk_i),
    .rst_ni           (rst_ni),
    .register_array_i (register_array),
    .insn_i           (insn_i),
    .store_index_i    (a1[3:0]),
    .store_result_o   (cl_store_result),
    .cl_active_o      (cl_active)
  );

  //-------- SPHINCS+ Unified Operations (thash1, thash2, prf_addr) --------------------------
  sphincs_ops #(
    .NUM_VARIABLES  (NUM_VARIABLES),
    .VARIABLE_WIDTH (VARIABLE_WIDTH)
  ) sphincs_ops_inst (
    .clk_i                  (clk_i),
    .rst_ni                 (rst_ni),
    .register_array_i       (register_array),
    .insn_i                 (insn_i),
    .simple_mode_i          (|b1),
    .keccak_done_i          (done_keccak_o),
    .keccak_result_i        (keccak_register_array),
    .sphincs_start_keccak_o (sphincs_start_keccak),
    .sphincs_keccak_input_o (sphincs_keccak_input),
    .sphincs_wb_enable_o    (sphincs_wb_enable),
    .sphincs_wb_data_o      (sphincs_wb_data),
    .sphincs_active_o       (sphincs_active),
    .sphincs_done_o         (sphincs_done)
  );

  //-------- Unified Sampler (CBD + ML-DSA sampling) ----------------------
  logic [31:0] sampler_result;

  sampler sampler_inst (
    .data_i(a1),
    .index_i(b1),
    .insn_i(insn_i),
    .result_o(sampler_result)
  );

  //-------- Falcon FPR Unit ------------------------------------------------------------
  logic [31:0] fpr_result;
  logic [31:0] fpr_result_hi;

  fpr fpr_inst (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .data1_i(a1),
    .data2_i(b1),
    .data3_i(c1),
    .insn_i(insn_i),
    .result_o(fpr_result),
    .result_hi_o(fpr_result_hi)
  );



  //-------- Multiplier Tree ----------------------------------------------------------------------------
  logic [31:0] multiplier_tree_result;
  logic [31:0] multiplier_tree_result_hi;

  multiplier_tree multiplier_tree_inst (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .multiplier_tree1_i(a1),
    .multiplier_tree2_i(b1),
    .multiplier_tree3_i(c1),
    .insn_i(insn_i),
    .result_o(multiplier_tree_result),
    .result_hi_o(multiplier_tree_result_hi)
  );

  //-------- OP_KREAD3: read 3 consecutive bytes from register file -------------------------
  // Given byte offset in a1, reads bytes [a1], [a1+1], [a1+2] from the Keccak register file.
  // 3 bytes span at most 2 adjacent 32-bit words. We form a 64-bit window and barrel-shift.
  logic [5:0]  rd3_word0_idx, rd3_word1_idx;
  logic [1:0]  rd3_byte_pos;
  logic [63:0] rd3_window, rd3_window_shifted;
  logic [31:0] OP_KREAD3_result;

  assign rd3_byte_pos  = a1[1:0];                              // byte offset within word
  assign rd3_word0_idx = a1[7:2];                              // first word index = a1 / 4
  assign rd3_word1_idx = a1[7:2] + {5'b0, |a1[1:0]};          // next word if byte_pos != 0

  assign rd3_window = {keccak_register_array[rd3_word1_idx],
                       keccak_register_array[rd3_word0_idx]};
  assign rd3_window_shifted = rd3_window >> ({4'b0, rd3_byte_pos} * 4'd8);

  // Return low 24 bits zero-extended to 32
  assign OP_KREAD3_result = {8'b0, rd3_window_shifted[23:0]};


  always_comb begin
    out.rd1 = '0;  // fills all bits with zero

    case (insn_i)

      none: begin out.rd1 = '0; end

      // Unified sampler (CBD + ML-DSA sampling operations)
      OP_CBD1, OP_CBD2, OP_CBD3, OP_CBD4,
      OP_REJ_UNIFORM, OP_REJ_ETA2, OP_REJ_ETA4, OP_UNPACK_Z: begin 
        out.rd1 = sampler_result; 
      end

      // store horcrux (mux between chain_lengths results and keccak results)
      OP_STORE: begin
        out.rd1 = cl_active ? cl_store_result : keccak_register_array[a1];
      end

      // OP_KREAD3: read 3 bytes from register file at byte offset a1
      OP_KREAD3: begin
        out.rd1 = OP_KREAD3_result;
      end

      OP_FPR_LOAD_SE, OP_FPR_EXEC, OP_FPR_RDHI, OP_FPR_NORM64_EXEC, OP_FPR_NORM64_RDHI, OP_FPR_NORM64_RDE: begin
                      out.rd1 = fpr_result;
      end

      OP_BFNTTK, OP_BARRETT, OP_BARRETT_HQC, OP_BARRETT_HQC3, OP_BARRETT_HQC5, OP_BFINTTK, OP_MQMULF, OP_MQMULK, OP_MQMULD, OP_KARATS_1, OP_KARATS_2, OP_KARATS_3, OP_GFMUL8, OP_BFNTTD, OP_BFNTTDH, OP_BFINTTD, OP_BFINTTDH, OP_KARATS_4, OP_RED32, OP_BFNTTF, OP_BFINTTF, OP_MODP_MONTYMUL, OP_GF_REDUCE, OP_COMPARE_U32: begin
                      out.rd1 = multiplier_tree_result;
      end

      default: begin out.rd1 = '0;  end
    endcase
  end

  assign rd_values_o.rd1 = out.rd1;
  
  // SPHINCS+ accelerator done signal (unified)
  assign done_sphincs_o = sphincs_done;

endmodule
