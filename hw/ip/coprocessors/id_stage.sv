// Copyright 2025 EDGE Group, @VLSI Lab - Politecnico di Torino                        //
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
//               Two pipelined-instruction depths:                                      //
//                 - WAIT_PIPE alone (1 stall cycle): Barrett reduction, pipelined       //
//                   internally in barrett.sv.                                          //
//                 - WAIT_PIPE2 -> WAIT_PIPE (2 stall cycles): Montgomery NTT/INTT       //
//                   instructions, which now settle a 3-stage shared_multiplication_    //
//                   logic pipeline (full_prod_q, then t_value_q) before commit.         //
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

  typedef enum logic [2:0] {
    IDLE,
    WAIT_KECCAK,
    WAIT_SPHINCS,
    WAIT_PIPE,    // final commit cycle for both 1-stall and 2-stall pipelined instructions
    WAIT_PIPE2    // extra stall cycle before WAIT_PIPE, for the 3-stage Montgomery pipeline
  } state_e;

  state_e state_q, state_d;
  logic state_keccak;
  logic state_sphincs;
  logic state_pipe;     // registered flag: 1-stall pipelined instruction in flight (Barrett)
  logic state_pipe2;    // registered flag: 2-stall pipelined instruction in flight (Montgomery)

  horcrux_pkg::instruction_u       instruction;
  horcrux_pkg::in_t                rs_values;
  horcrux_pkg::horcrux_insn        select_insn;
  logic [NbInstr-1:0] sel;
  logic                          sample_in;
  logic                          save_rd;
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
        if (state_sphincs) begin
          state_d    = WAIT_SPHINCS;
        end else if (state_keccak) begin
          state_d    = WAIT_KECCAK;
        end else if (state_pipe2) begin
          state_d    = WAIT_PIPE2;
        end else if (state_pipe) begin
          state_d    = WAIT_PIPE;
        end else begin
          state_d  = IDLE;
        end
      end

      WAIT_KECCAK: begin
        if (keccak_done_i) begin
          state_d   = IDLE;
        end
      end

      WAIT_SPHINCS: begin
        if (sphincs_done_i) begin
          state_d   = IDLE;
        end
      end

      WAIT_PIPE: begin
        // Final commit cycle (reached directly from IDLE for 1-stall
        // instructions, or via WAIT_PIPE2 for 2-stall instructions):
        // return to IDLE unconditionally next cycle.
        state_d = IDLE;
      end

      WAIT_PIPE2: begin
        // Extra stall cycle for the 3-stage Montgomery pipeline: always
        // proceed to WAIT_PIPE (the final commit cycle) next.
        state_d = WAIT_PIPE;
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
    xif_issue_if.issue_ready          = '0;
    sample_in                         = '0;
    save_rd                           = '0;
    rd_o <= '0;
    id_o <= '0;

    // Retain context during pipeline stall
    if (state_pipe || state_pipe2 || state_q == WAIT_PIPE || state_q == WAIT_PIPE2) begin
      select_insn = select_insn_o;
      rs_values   = rs_values_o;
    end else begin
      select_insn = none;
      rs_values   = '0;
    end

    for (int unsigned i = 0; i < NbInstr; i++) begin
      // Block new issue when a pipelined instruction is in flight.
      // Guards needed:
      //   state_pipe/state_pipe2: cycle(s) AFTER issue (state_q still IDLE
      //     but the shared_mul pipeline is computing and rd_excom must not
      //     be overwritten before commit).
      //   state_q==WAIT_PIPE/WAIT_PIPE2: the stall/commit cycles themselves.
      // Keccak/SPHINCS+ are unaffected because they leave state_pipe(2)=0.
      if (sel[i] && xif_issue_if.issue_valid &&
          (state_q != WAIT_PIPE) && (state_q != WAIT_PIPE2) &&
          !state_pipe && !state_pipe2) begin

        xif_issue_if.issue_ready          = 1'b1;
        xif_issue_if.issue_resp.accept    = CoproInstr[i].resp.accept;
        xif_issue_if.issue_resp.writeback = CoproInstr[i].resp.writeback;
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

  logic done_d, state_keccak_d, state_sphincs_d, state_pipe_d, state_pipe2_d;

  // Gate on sample_in (issue_valid && issue_ready) not just issue_valid.
  // If we used issue_valid alone, the CPU keeping issue_valid=1 while
  // issue_ready=0 (WAIT_PIPE / state_pipe) would re-assert state_pipe_d
  // every cycle, looping IDLE→WAIT_PIPE→IDLE→… forever and preventing
  // issue_ready from ever rising again.
  always_comb begin
    done_d          = 1'b0;
    state_keccak_d  = 1'b0;
    state_sphincs_d = 1'b0;
    state_pipe_d    = 1'b0;
    state_pipe2_d   = 1'b0;

    if (sample_in) begin
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
            done_d          = 1'b0;
            state_sphincs_d = 1'b1;
          end else if (CoproInstr[i].resp.insn == OP_KSTART ||
                       CoproInstr[i].resp.insn == OP_KPERM) begin
            done_d         = 1'b0;
            state_keccak_d = 1'b1;
          end else if (CoproInstr[i].resp.insn == OP_BFNTTD  ||
                       CoproInstr[i].resp.insn == OP_BFINTTD ||
                       CoproInstr[i].resp.insn == OP_BFNTTK  ||
                       CoproInstr[i].resp.insn == OP_BFINTTK ||
                       CoproInstr[i].resp.insn == OP_BFNTTF  ||
                       CoproInstr[i].resp.insn == OP_BFINTTF ||
                       CoproInstr[i].resp.insn == OP_MQMULD  ||
                       CoproInstr[i].resp.insn == OP_MQMULK  ||
                       CoproInstr[i].resp.insn == OP_MQMULF  ||
                       CoproInstr[i].resp.insn == OP_MODP_MONTYMUL) begin
            // Pipelined Montgomery instructions: done fires two cycles later
            // (WAIT_PIPE2 then WAIT_PIPE), after both full_prod_q and
            // t_value_q (the two pipeline registers in shared_mul) are valid.
            done_d        = 1'b0;
            state_pipe2_d = 1'b1;
          end else if (CoproInstr[i].resp.insn == OP_BARRETT ||
                       CoproInstr[i].resp.insn == OP_BARRETT_HQC ||
                       CoproInstr[i].resp.insn == OP_BARRETT_HQC3 ||
                       CoproInstr[i].resp.insn == OP_BARRETT_HQC5) begin
            // Barrett instructions: done fires one cycle later, after the
            // quotient register in barrett.sv is valid.
            done_d       = 1'b0;
            state_pipe_d = 1'b1;
          end else begin
            done_d         = CoproInstr[i].resp.done;
          end
        end
      end
    end
  end


  // pipe_ctx_hold: keep rs_values_o/select_insn_o frozen during every stall
  // cycle that follows a pipelined instruction issue:
  //   Barrett (1-stall):    cycle N+1: state_pipe=1, state_q still IDLE
  //                         cycle N+2: state_q==WAIT_PIPE (commit)
  //   Montgomery (2-stall): cycle N+1: state_pipe2=1, state_q still IDLE
  //                         cycle N+2: state_q==WAIT_PIPE2
  //                         cycle N+3: state_q==WAIT_PIPE (commit)
  // During these cycles the comb always_comb feeds select_insn_o back to
  // select_insn, but the flop must not clobber it with the default none/'0.
  logic pipe_ctx_hold;
  assign pipe_ctx_hold = state_pipe | state_pipe2 |
                          (state_q == WAIT_PIPE) | (state_q == WAIT_PIPE2);

  always @(posedge clk_i or negedge rst_ni) begin : pipe_reg_ID_EX
    if (~rst_ni) begin
      state_q       <= IDLE;
      rs_values_o   <= '0;
      select_insn_o <= none;
      done          <= '0;
      state_keccak  <= '0;
      state_sphincs <= '0;
      state_pipe    <= '0;
      state_pipe2   <= '0;
    end else begin
      state_q       <= state_d;
      // Only update the context registers when a new instruction is being
      // accepted OR when we are not in the pipe-hold window.  This prevents
      // the "none/0" defaults from trampling the saved context between the
      // issue cycle and the WAIT_PIPE commit cycle.
      if (sample_in || !pipe_ctx_hold) begin
        rs_values_o   <= rs_values;
        select_insn_o <= select_insn;
      end
      done          <= done_d;
      state_keccak  <= state_keccak_d;
      state_sphincs <= state_sphincs_d;
      state_pipe    <= state_pipe_d;
      state_pipe2   <= state_pipe2_d;
    end
  end


  assign save_rd_o = save_rd;

  // done_o fires when:
  //  - immediate single-cycle instructions: done register is high
  //  - Keccak multi-cycle: keccak_done pulse while waiting
  //  - SPHINCS+ multi-cycle: sphincs_done pulse while waiting
  //  - Pipelined NTT/INTT: unconditionally in WAIT_PIPE cycle, reached via
  //    WAIT_PIPE2 (full_prod_q and t_value_q now valid)
  //  - Pipelined Barrett: unconditionally in WAIT_PIPE cycle, reached
  //    directly from IDLE (quotient register in barrett.sv now valid)
  assign done_o = done |
                  (state_q == WAIT_SPHINCS && sphincs_done_i) |
                  (state_q == WAIT_KECCAK  && keccak_done_i)  |
                  (state_q == WAIT_PIPE);

endmodule
