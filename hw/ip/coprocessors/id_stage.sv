// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Instruction Decode Stage                                               //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Decodes HORCRUX instructions from XIF interface, extracts operands,    //
//               and routes instruction to appropriate functional units (Keccak, etc).  //
///////////////////////////////////////////////////////////////////////////////////////////

module id_stage
  import horcrux_pkg::*;
(
  input clk_i,
  input rst_ni,

  if_xif.coproc_issue xif_issue_if,
  input logic                        keccak_done_i,
  input logic                        sphincs_done_i,

  output horcrux_pkg::in_t           rs_values_o,
  output logic                 [4:0] rd_o,
  output logic                 [3:0] id_o,
  output horcrux_pkg::horcrux_insn   select_insn_o,
  output logic       save_rd_o,
  output logic       done_o
);

  typedef enum logic [1:0] {
    IDLE,
    WAIT_KECCAK,
    WAIT_SPHINCS
  } state_e;

  state_e state_q, state_d;
  logic state_keccak;
  logic state_sphincs;

  horcrux_pkg::instruction_u       instruction;
  horcrux_pkg::in_t                rs_values;
  horcrux_pkg::horcrux_insn        select_insn;
  logic [NbInstr-1:0] sel;
  logic                          sample_in;
  logic                          save_rd;
  logic                          result_reg_en;
  logic                          done;

  assign instruction = xif_issue_if.issue_req.instr;
  for (genvar i = 0; i < NbInstr; i++) begin : gen_predecoder_selector
    assign sel[i] = ((CoproInstr[i].mask & instruction) == CoproInstr[i].instr);
  end

  // --- FSM & Decoder Logic ---
  always_comb begin

    state_d   = state_q;

    case (state_q)
      IDLE: begin
        // Standard Ready state: can accept instructions
        if (state_sphincs) begin
          state_d    = WAIT_SPHINCS;
        end else if (state_keccak) begin
          state_d    = WAIT_KECCAK;
        end else begin
          state_d  = IDLE;
        end
      end

      WAIT_KECCAK: begin
        // Wait for keccak done signal
        if (keccak_done_i) begin
          state_d   = IDLE;
        end
      end

      WAIT_SPHINCS: begin
        // Wait for sphincs done signal (thash1, thash2, or prf_addr)
        if (sphincs_done_i) begin
          state_d   = IDLE;
        end
      end

      default: state_d = IDLE;
    endcase
  end

  //DECODER
  always_comb begin

    xif_issue_if.issue_resp.loadstore = '0;
    xif_issue_if.issue_resp.ecswrite  = '0;
    xif_issue_if.issue_resp.exc       = '0;

    xif_issue_if.issue_resp.accept    = '0;
    xif_issue_if.issue_resp.writeback = '0;
    xif_issue_if.issue_resp.dualread  = '0;
    xif_issue_if.issue_resp.dualwrite = '0;
    xif_issue_if.issue_resp.loadstore = '0;
    xif_issue_if.issue_resp.ecswrite  = '0;
    xif_issue_if.issue_resp.exc       = '0;
    xif_issue_if.issue_ready          = '0;

    sample_in                         = '0;
    select_insn                       = none;
    save_rd                           = '0;
    rs_values                         = '0;
    rd_o <= '0;
    id_o <= '0;

    for (int unsigned i = 0; i < NbInstr; i++) begin
      if (sel[i] && xif_issue_if.issue_valid) begin

        xif_issue_if.issue_ready          = 1'b1;
        xif_issue_if.issue_resp.accept    = CoproInstr[i].resp.accept;
        xif_issue_if.issue_resp.writeback = CoproInstr[i].resp.writeback;
        // Enable dual-read path for instructions that consume rs3.
        xif_issue_if.issue_resp.dualread  = CoproInstr[i].resp.register_read[2];

        sample_in                         = 1'b1;
        select_insn                       = CoproInstr[i].resp.insn;
        save_rd                           = 1'b1;

        rs_values.rs1                   = xif_issue_if.issue_req.rs[0];
        rs_values.rs2                   = xif_issue_if.issue_req.rs[1];
        rs_values.rs3                   = xif_issue_if.issue_req.rs[2];

        rd_o <= instruction.as_horcrux_R.rd;
        id_o <= xif_issue_if.issue_req.id;
      end
    end
  end


  //generate
  //  always @(posedge clk_i or negedge rst_ni) begin
  //    if (~rst_ni) begin
  //      done         <= '0;
  //      state_keccak <= '0;
  //    end else begin
  //      for (int unsigned i = 0; i < NbInstr; i++) begin
  //        if (xif_issue_if.issue_valid) begin
  //          if (sel[i]) begin
  //            if (CoproInstr[i].resp.insn == OP_KSTART) begin
  //              done         <= 1'b0; // Don't finish yet
  //              state_keccak <= 1'b1;
  //            end else begin
  //              done    = CoproInstr[i].resp.done; // Immediate finish
  //              state_keccak <= '0;
  //            end
  //          end
  //        end else begin
  //          done         <= '0;
  //          state_keccak <= '0;
  //        end
  //      end
  //    end
  //  end
  //endgenerate
  logic done_d, state_keccak_d, state_sphincs_d;

  always_comb begin
    done_d          = 1'b0;
    state_keccak_d  = 1'b0;
    state_sphincs_d = 1'b0;

    if (xif_issue_if.issue_valid) begin
      for (int unsigned i = 0; i < NbInstr; i++) begin
        if (sel[i]) begin
          if (CoproInstr[i].resp.insn == OP_THASH1 ||
              CoproInstr[i].resp.insn == OP_THASH2 ||
              CoproInstr[i].resp.insn == OP_PRF_ADDR ||
              CoproInstr[i].resp.insn == OP_THASH1_192 ||
              CoproInstr[i].resp.insn == OP_THASH2_192 ||
              CoproInstr[i].resp.insn == OP_THASH1_256 ||
              CoproInstr[i].resp.insn == OP_THASH2_256 ||
              CoproInstr[i].resp.insn == OP_PRF_192 ||
              CoproInstr[i].resp.insn == OP_PRF_256) begin
            // SPHINCS+ operations: multi-cycle, uses sphincs_done_i for completion
            done_d         = 1'b0;
            state_sphincs_d = 1'b1;
          end else if (CoproInstr[i].resp.insn == OP_KSTART || CoproInstr[i].resp.insn == OP_KPERM) begin
            // Keccak: multi-cycle, uses keccak_done_i for completion
            done_d         = 1'b0;
            state_keccak_d = 1'b1;
          end else begin
            done_d         = CoproInstr[i].resp.done;
          end
        end
      end
    end
  end



  always @(posedge clk_i or negedge rst_ni) begin : pipe_reg_ID_EX
    if (~rst_ni) begin
      state_q       <= IDLE;
      rs_values_o   <= '0;
      select_insn_o <= none;
      done          <= '0;
      state_keccak  <= '0;
      state_sphincs <= '0;
    end else begin
      //if (sample_in) begin
      state_q       <= state_d;
      rs_values_o   <= rs_values;
      select_insn_o <= select_insn;
      done          <= done_d;
      state_keccak  <= state_keccak_d;
      state_sphincs <= state_sphincs_d;
      //end
    end
  end


  assign save_rd_o   = save_rd;
  // done_o: pulse when instruction completes (immediate or after multi-cycle wait)
  assign done_o      = done | 
                       (state_q == WAIT_SPHINCS && sphincs_done_i) |
                       (state_q == WAIT_KECCAK && keccak_done_i);

endmodule
