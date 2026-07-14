// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Chain Lengths Unit                                                     //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  SPHINCS+ WOTS+ chain lengths accelerator. Computes base_w and          //
//               checksum in hardware, extracts nibbles and packs results efficiently. //
///////////////////////////////////////////////////////////////////////////////////////////

module chain_lengths
  import horcrux_pkg::*;
#(
  parameter int unsigned NUM_VARIABLES  = 50,
  parameter int unsigned VARIABLE_WIDTH = 32
) (
  input  logic                                          clk_i,
  input  logic                                          rst_ni,
  input  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0]  register_array_i,
  input  horcrux_pkg::horcrux_insn                      insn_i,
  input  logic [3:0]                                    store_index_i,
  output logic [31:0]                                   store_result_o,
  output logic                                          cl_active_o
);

  // ============================================================================
  // Internal signals
  // ============================================================================

  // Result registers (max 9 words for 256f: 64 msg nibbles + 3 csum nibbles = 67 -> ceil(67/8) = 9)
  logic [31:0] result_regs [0:8];
  logic        cl_active_reg;

  // ============================================================================
  // Nibble extraction from register array (little-endian byte order)
  // ============================================================================
  // RISC-V is little-endian: word loaded from &msg[4r] has:
  //   reg[r][7:0]   = msg[4r+0]
  //   reg[r][15:8]  = msg[4r+1]
  //   reg[r][23:16] = msg[4r+2]
  //   reg[r][31:24] = msg[4r+3]
  //
  // base_w extracts high nibble first, then low nibble per byte:
  //   nibble[8r+0] = msg[4r+0][7:4] = reg[r][7:4]
  //   nibble[8r+1] = msg[4r+0][3:0] = reg[r][3:0]
  //   nibble[8r+2] = msg[4r+1][7:4] = reg[r][15:12]
  //   nibble[8r+3] = msg[4r+1][3:0] = reg[r][11:8]
  //   nibble[8r+4] = msg[4r+2][7:4] = reg[r][23:20]
  //   nibble[8r+5] = msg[4r+2][3:0] = reg[r][19:16]
  //   nibble[8r+6] = msg[4r+3][7:4] = reg[r][31:28]
  //   nibble[8r+7] = msg[4r+3][3:0] = reg[r][27:24]

  logic [3:0] nibbles [0:63];

  always_comb begin
    for (int r = 0; r < 8; r++) begin
      nibbles[8*r + 0] = register_array_i[r][7:4];
      nibbles[8*r + 1] = register_array_i[r][3:0];
      nibbles[8*r + 2] = register_array_i[r][15:12];
      nibbles[8*r + 3] = register_array_i[r][11:8];
      nibbles[8*r + 4] = register_array_i[r][23:20];
      nibbles[8*r + 5] = register_array_i[r][19:16];
      nibbles[8*r + 6] = register_array_i[r][31:28];
      nibbles[8*r + 7] = register_array_i[r][27:24];
    end
  end

  // ============================================================================
  // Checksum computation (adder trees for each variant)
  // ============================================================================
  // csum = sum(W-1 - nibble[i]) = sum(15 - nibble[i]) for i=0..LEN1-1
  // Max values: 32*15=480, 48*15=720, 64*15=960 -> all fit in 10 bits

  logic [9:0] csum_128f;  // sum of 32 nibbles
  logic [9:0] csum_192f;  // sum of 48 nibbles
  logic [9:0] csum_256f;  // sum of 64 nibbles

  always_comb begin
    csum_128f = '0;
    for (int i = 0; i < 32; i++) begin
      csum_128f = csum_128f + (10'(4'd15) - 10'(nibbles[i]));
    end

    csum_192f = '0;
    for (int i = 0; i < 48; i++) begin
      csum_192f = csum_192f + (10'(4'd15) - 10'(nibbles[i]));
    end

    csum_256f = '0;
    for (int i = 0; i < 64; i++) begin
      csum_256f = csum_256f + (10'(4'd15) - 10'(nibbles[i]));
    end
  end

  // ============================================================================
  // Checksum shift and nibble extraction
  // ============================================================================
  // For LEN2=3 (all variants): shift = (8 - (3*4 % 8)) % 8 = 4
  //
  // Shifted checksum is 16-bit, extract 3 nibbles MSB-first:
  //   csum_nibble[0] = shifted[15:12]
  //   csum_nibble[1] = shifted[11:8]
  //   csum_nibble[2] = shifted[7:4]

  logic [15:0] csum_shifted_128f;
  logic [15:0] csum_shifted_192f;
  logic [15:0] csum_shifted_256f;

  assign csum_shifted_128f = {6'b0, csum_128f} << 4;
  assign csum_shifted_192f = {6'b0, csum_192f} << 4;
  assign csum_shifted_256f = {6'b0, csum_256f} << 4;

  logic [3:0] csum_nib_128f [0:2];
  logic [3:0] csum_nib_192f [0:2];
  logic [3:0] csum_nib_256f [0:2];

  assign csum_nib_128f[0] = csum_shifted_128f[15:12];
  assign csum_nib_128f[1] = csum_shifted_128f[11:8];
  assign csum_nib_128f[2] = csum_shifted_128f[7:4];

  assign csum_nib_192f[0] = csum_shifted_192f[15:12];
  assign csum_nib_192f[1] = csum_shifted_192f[11:8];
  assign csum_nib_192f[2] = csum_shifted_192f[7:4];

  assign csum_nib_256f[0] = csum_shifted_256f[15:12];
  assign csum_nib_256f[1] = csum_shifted_256f[11:8];
  assign csum_nib_256f[2] = csum_shifted_256f[7:4];

  // ============================================================================
  // Pack message nibbles into 32-bit words (shared across variants)
  // ============================================================================
  // packed_msg[w] = {nibbles[8w+0], nibbles[8w+1], ..., nibbles[8w+7]}
  // Each nibble occupies 4 bits, MSB-first packing.

  logic [31:0] packed_msg [0:7];

  always_comb begin
    for (int w = 0; w < 8; w++) begin
      packed_msg[w] = {nibbles[8*w+0], nibbles[8*w+1], nibbles[8*w+2], nibbles[8*w+3],
                       nibbles[8*w+4], nibbles[8*w+5], nibbles[8*w+6], nibbles[8*w+7]};
    end
  end

  // ============================================================================
  // Pack checksum nibbles into the final word per variant
  // ============================================================================
  // 128f: LEN1=32 -> 4 msg words (0-3), csum word = packed[4] with 3 nibbles + 5 zeros
  // 192f: LEN1=48 -> 6 msg words (0-5), csum word = packed[6] with 3 nibbles + 5 zeros
  // 256f: LEN1=64 -> 8 msg words (0-7), csum word = packed[8] with 3 nibbles + 5 zeros

  logic [31:0] csum_word_128f;
  logic [31:0] csum_word_192f;
  logic [31:0] csum_word_256f;

  assign csum_word_128f = {csum_nib_128f[0], csum_nib_128f[1], csum_nib_128f[2], 20'b0};
  assign csum_word_192f = {csum_nib_192f[0], csum_nib_192f[1], csum_nib_192f[2], 20'b0};
  assign csum_word_256f = {csum_nib_256f[0], csum_nib_256f[1], csum_nib_256f[2], 20'b0};

  // ============================================================================
  // Register results on compute instruction, manage cl_active flag
  // ============================================================================

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 9; i++) begin
        result_regs[i] <= '0;
      end
      cl_active_reg <= 1'b0;
    end else begin
      case (insn_i)
        OP_CL_128F: begin
          // 128f: 32 msg nibbles (4 words) + 3 csum nibbles (1 word) = 5 words
          result_regs[0] <= packed_msg[0];
          result_regs[1] <= packed_msg[1];
          result_regs[2] <= packed_msg[2];
          result_regs[3] <= packed_msg[3];
          result_regs[4] <= csum_word_128f;
          // Clear unused
          result_regs[5] <= '0;
          result_regs[6] <= '0;
          result_regs[7] <= '0;
          result_regs[8] <= '0;
          cl_active_reg  <= 1'b1;
        end

        OP_CL_192F: begin
          // 192f: 48 msg nibbles (6 words) + 3 csum nibbles (1 word) = 7 words
          result_regs[0] <= packed_msg[0];
          result_regs[1] <= packed_msg[1];
          result_regs[2] <= packed_msg[2];
          result_regs[3] <= packed_msg[3];
          result_regs[4] <= packed_msg[4];
          result_regs[5] <= packed_msg[5];
          result_regs[6] <= csum_word_192f;
          // Clear unused
          result_regs[7] <= '0;
          result_regs[8] <= '0;
          cl_active_reg  <= 1'b1;
        end

        OP_CL_256F: begin
          // 256f: 64 msg nibbles (8 words) + 3 csum nibbles (1 word) = 9 words
          result_regs[0] <= packed_msg[0];
          result_regs[1] <= packed_msg[1];
          result_regs[2] <= packed_msg[2];
          result_regs[3] <= packed_msg[3];
          result_regs[4] <= packed_msg[4];
          result_regs[5] <= packed_msg[5];
          result_regs[6] <= packed_msg[6];
          result_regs[7] <= packed_msg[7];
          result_regs[8] <= csum_word_256f;
          cl_active_reg  <= 1'b1;
        end

        OP_KSTART,
        OP_INIT,
        OP_THASH1, OP_THASH2, OP_PRF_ADDR,
        OP_THASH1_192, OP_THASH2_192,
        OP_THASH1_256, OP_THASH2_256,
        OP_PRF_192, OP_PRF_256: begin
          // Clear cl_active when another coprocessor operation starts
          cl_active_reg <= 1'b0;
        end

        default: ; // Hold state
      endcase
    end
  end

  // ============================================================================
  // Output: store read and cl_active flag
  // ============================================================================

  //assign store_result_o = result_regs[store_index_i];
  assign store_result_o = cl_active_reg ? result_regs[store_index_i] : 32'h0;
  assign cl_active_o    = cl_active_reg;

endmodule
