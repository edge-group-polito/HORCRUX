// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  SPHINCS+ Operations Unit                                               //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Unified SPHINCS+ acceleration with THASH1/2 and PRF_ADDR operations.   //
//               Shares Keccak permutation core with stateful mux for data paths.      //
///////////////////////////////////////////////////////////////////////////////////////////

module sphincs_ops
  import horcrux_pkg::*;
#(
  parameter int unsigned NUM_VARIABLES  = 50,
  parameter int unsigned VARIABLE_WIDTH = 32
) (
  input  logic                                          clk_i,
  input  logic                                          rst_ni,
  input  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0]  register_array_i,
  input  horcrux_pkg::horcrux_insn                      insn_i,
  input  logic                                          simple_mode_i,
  input  logic                                          keccak_done_i,
  input  logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0]  keccak_result_i,
  
  // Keccak interface
  output logic                                          sphincs_start_keccak_o,
  output logic [1599:0]                                 sphincs_keccak_input_o,
  
  // Register file writeback
  output logic                                          sphincs_wb_enable_o,
  output logic [0:NUM_VARIABLES-1][VARIABLE_WIDTH-1:0]  sphincs_wb_data_o,
  
  // Status signals
  output logic                                          sphincs_active_o,
  output logic                                          sphincs_done_o
);

  // ============================================================================
  // Constants
  // ============================================================================
  
  localparam int unsigned SPX_ADDR_BYTES = 32;  // Address size in bytes (all levels)
  localparam int unsigned SHAKE256_RATE = 136;  // SHAKE256 rate in bytes
  
  // ============================================================================
  // Security level encoding
  // ============================================================================
  
  typedef enum logic [1:0] {
    SEC_128 = 2'b00,
    SEC_192 = 2'b01,
    SEC_256 = 2'b10
  } sec_level_t;
  
  // ============================================================================
  // Operation type encoding (latched at start)
  // ============================================================================
  
  typedef enum logic [1:0] {
    OP_TYPE_NONE    = 2'b00,
    OP_TYPE_PRF     = 2'b01,  // prf_addr: 1 permutation
    OP_TYPE_THASH1  = 2'b10,  // thash1: N-byte input
    OP_TYPE_THASH2  = 2'b11   // thash2: 2N-byte input
  } op_type_t;
  
  op_type_t   op_type_q, op_type_d;
  sec_level_t sec_level_q, sec_level_d;
  logic       simple_q, simple_d;
  
  // ============================================================================
  // State machine (unified for all operations)
  // ============================================================================
  // 
  // PRF:            IDLE -> ABSORB_1 -> WAIT_1 -> STORE_RESULT
  // THASH robust:   IDLE -> ABSORB_1 -> WAIT_1 -> ABSORB_2 -> WAIT_2 -> STORE_RESULT
  // THASH simple:   IDLE -> ABSORB_1 -> WAIT_1 -> STORE_RESULT
  
  typedef enum logic [2:0] {
    IDLE,
    ABSORB_1,
    WAIT_1,
    ABSORB_2,
    WAIT_2,
    STORE_RESULT
  } state_t;
  
  state_t state_q, state_d;
  
  // ============================================================================
  // Internal signals
  // ============================================================================
  
  // Extracted data from register file (max widths for 256-level)
  logic [255:0] pub_seed;
  logic [255:0] addr;
  logic [255:0] input_block1;
  logic [255:0] input_block2;
  
  // Bitmask and masked input (max: thash2 @ 256 = 512 bits)
  logic [511:0] bitmask_reg;
  logic [511:0] masked_input;
  
  // Keccak state
  logic [1599:0] keccak_state;
  
  // Signals for operation detection
  logic is_any_launch;
  logic is_thash;  // Either thash1 or thash2
  
  // ============================================================================
  // Extract data from register file (parameterized by security level)
  // ============================================================================
  //  128: pub_seed=reg[0:3],  addr=reg[4:11],  input1=reg[12:15], input2=reg[16:19]
  //  192: pub_seed=reg[0:5],  addr=reg[6:13],  input1=reg[14:19], input2=reg[20:25]
  //  256: pub_seed=reg[0:7],  addr=reg[8:15],  input1=reg[16:23], input2=reg[24:31]
  
  always_comb begin
    pub_seed = '0;
    addr = '0;
    input_block1 = '0;
    input_block2 = '0;
    
    case (sec_level_q)
      SEC_128: begin
        pub_seed[127:0] = {register_array_i[3],  register_array_i[2],
                           register_array_i[1],  register_array_i[0]};
        addr = {register_array_i[11], register_array_i[10], register_array_i[9], register_array_i[8],
                register_array_i[7],  register_array_i[6],  register_array_i[5], register_array_i[4]};
        input_block1[127:0] = {register_array_i[15], register_array_i[14],
                               register_array_i[13], register_array_i[12]};
        input_block2[127:0] = {register_array_i[19], register_array_i[18],
                               register_array_i[17], register_array_i[16]};
      end
      SEC_192: begin
        pub_seed[191:0] = {register_array_i[5],  register_array_i[4],  register_array_i[3],
                           register_array_i[2],  register_array_i[1],  register_array_i[0]};
        addr = {register_array_i[13], register_array_i[12], register_array_i[11], register_array_i[10],
                register_array_i[9],  register_array_i[8],  register_array_i[7],  register_array_i[6]};
        input_block1[191:0] = {register_array_i[19], register_array_i[18], register_array_i[17],
                               register_array_i[16], register_array_i[15], register_array_i[14]};
        input_block2[191:0] = {register_array_i[25], register_array_i[24], register_array_i[23],
                               register_array_i[22], register_array_i[21], register_array_i[20]};
      end
      SEC_256: begin
        pub_seed = {register_array_i[7],  register_array_i[6],  register_array_i[5],  register_array_i[4],
                    register_array_i[3],  register_array_i[2],  register_array_i[1],  register_array_i[0]};
        addr = {register_array_i[15], register_array_i[14], register_array_i[13], register_array_i[12],
                register_array_i[11], register_array_i[10], register_array_i[9],  register_array_i[8]};
        input_block1 = {register_array_i[23], register_array_i[22], register_array_i[21], register_array_i[20],
                        register_array_i[19], register_array_i[18], register_array_i[17], register_array_i[16]};
        input_block2 = {register_array_i[31], register_array_i[30], register_array_i[29], register_array_i[28],
                        register_array_i[27], register_array_i[26], register_array_i[25], register_array_i[24]};
      end
      default: ;
    endcase
  end
  
  // ============================================================================
  // Operation type detection and latching
  // ============================================================================
  
  assign is_thash = (op_type_q == OP_TYPE_THASH1) || (op_type_q == OP_TYPE_THASH2);
  
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      op_type_q   <= OP_TYPE_NONE;
      sec_level_q <= SEC_128;
      simple_q    <= 1'b0;
    end else begin
      op_type_q   <= op_type_d;
      sec_level_q <= sec_level_d;
      simple_q    <= simple_d;
    end
  end
  
  always_comb begin
    op_type_d   = op_type_q;
    sec_level_d = sec_level_q;
    simple_d    = simple_q;
    is_any_launch = 1'b0;
    
    if (state_q == IDLE) begin
      case (insn_i)
        OP_THASH1: begin
          op_type_d = OP_TYPE_THASH1; sec_level_d = SEC_128;
          simple_d = simple_mode_i; is_any_launch = 1'b1;
        end
        OP_THASH2: begin
          op_type_d = OP_TYPE_THASH2; sec_level_d = SEC_128;
          simple_d = simple_mode_i; is_any_launch = 1'b1;
        end
        OP_PRF_ADDR: begin
          op_type_d = OP_TYPE_PRF; sec_level_d = SEC_128;
          simple_d = 1'b0; is_any_launch = 1'b1;
        end
        OP_THASH1_192: begin
          op_type_d = OP_TYPE_THASH1; sec_level_d = SEC_192;
          simple_d = simple_mode_i; is_any_launch = 1'b1;
        end
        OP_THASH2_192: begin
          op_type_d = OP_TYPE_THASH2; sec_level_d = SEC_192;
          simple_d = simple_mode_i; is_any_launch = 1'b1;
        end
        OP_THASH1_256: begin
          op_type_d = OP_TYPE_THASH1; sec_level_d = SEC_256;
          simple_d = simple_mode_i; is_any_launch = 1'b1;
        end
        OP_THASH2_256: begin
          op_type_d = OP_TYPE_THASH2; sec_level_d = SEC_256;
          simple_d = simple_mode_i; is_any_launch = 1'b1;
        end
        OP_PRF_192: begin
          op_type_d = OP_TYPE_PRF; sec_level_d = SEC_192;
          simple_d = 1'b0; is_any_launch = 1'b1;
        end
        OP_PRF_256: begin
          op_type_d = OP_TYPE_PRF; sec_level_d = SEC_256;
          simple_d = 1'b0; is_any_launch = 1'b1;
        end
        default: begin
          op_type_d = OP_TYPE_NONE; is_any_launch = 1'b0;
        end
      endcase
    end else if (state_d == IDLE) begin
      op_type_d = OP_TYPE_NONE;
      simple_d  = 1'b0;
    end
  end
  
  // ============================================================================
  // Bitmask register - save first permutation output (robust thash only)
  // ============================================================================
  //  128 thash1: 128-bit bitmask  |  128 thash2: 256-bit bitmask
  //  192 thash1: 192-bit bitmask  |  192 thash2: 384-bit bitmask
  //  256 thash1: 256-bit bitmask  |  256 thash2: 512-bit bitmask
  
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      bitmask_reg <= '0;
    end else if (state_q == WAIT_1 && keccak_done_i && is_thash && !simple_q) begin
      bitmask_reg <= '0;
      case (sec_level_q)
        SEC_128: begin
          if (op_type_q == OP_TYPE_THASH2) begin
            for (int i = 0; i < 8; i++)
              bitmask_reg[i*32 +: 32] <= keccak_result_i[i];
          end else begin
            for (int i = 0; i < 4; i++)
              bitmask_reg[i*32 +: 32] <= keccak_result_i[i];
          end
        end
        SEC_192: begin
          if (op_type_q == OP_TYPE_THASH2) begin
            for (int i = 0; i < 12; i++)
              bitmask_reg[i*32 +: 32] <= keccak_result_i[i];
          end else begin
            for (int i = 0; i < 6; i++)
              bitmask_reg[i*32 +: 32] <= keccak_result_i[i];
          end
        end
        SEC_256: begin
          if (op_type_q == OP_TYPE_THASH2) begin
            for (int i = 0; i < 16; i++)
              bitmask_reg[i*32 +: 32] <= keccak_result_i[i];
          end else begin
            for (int i = 0; i < 8; i++)
              bitmask_reg[i*32 +: 32] <= keccak_result_i[i];
          end
        end
        default: ;
      endcase
    end
  end
  
  // Compute masked input (for robust state_2).
  // Pack only the active N-byte portions to avoid pulling zero-padded upper bits
  // from 128/192 containers into the 2N-byte THASH2 payload.
  always_comb begin
    logic [511:0] payload_packed;

    payload_packed = '0;
    if (op_type_q == OP_TYPE_THASH2) begin
      case (sec_level_q)
        SEC_128: payload_packed[255:0]  = {input_block2[127:0], input_block1[127:0]};
        SEC_192: payload_packed[383:0]  = {input_block2[191:0], input_block1[191:0]};
        SEC_256: payload_packed[511:0]  = {input_block2[255:0], input_block1[255:0]};
        default: payload_packed[255:0]  = {input_block2[127:0], input_block1[127:0]};
      endcase
    end else begin
      case (sec_level_q)
        SEC_128: payload_packed[127:0]  = input_block1[127:0];
        SEC_192: payload_packed[191:0]  = input_block1[191:0];
        SEC_256: payload_packed[255:0]  = input_block1[255:0];
        default: payload_packed[127:0]  = input_block1[127:0];
      endcase
    end

    masked_input = payload_packed ^ bitmask_reg;
  end
  
  // ============================================================================
  // Keccak state construction (per security level)
  // ============================================================================
  //
  // SHAKE256 rate = 136 bytes (1088 bits). Capacity = 64 bytes (512 bits).
  // Full state = 1600 bits = rate_portion[1087:0] || capacity[511:0]
  //
  // State 1 cases:
  //   Robust thash bitmask: pub_seed(N) || addr(32) || 0x1F || ... || 0x80
  //   Simple thash / PRF:   pub_seed(N) || addr(32) || payload || 0x1F || ... || 0x80
  //     payload = input1 (thash1), input1||input2 (thash2), sk_seed (PRF)
  //
  // State 2 (robust thash only):
  //   pub_seed(N) || addr(32) || masked_input || 0x1F || ... || 0x80
  //
  // Bit layout: byte 0 at bits [7:0], byte k at bits [8k+7:8k]
  // 0x80 padding always at byte 135 = bits [1087:1080]
  
  logic [1087:0] rate_1_128, rate_1_192, rate_1_256;
  logic [1087:0] rate_2_128, rate_2_192, rate_2_256;
  logic [1087:0] rate_portion_1, rate_portion_2;
  logic [1599:0] state_1, state_2;
  
  // ---- State 1: 128f ----
  // pub_seed[127:0] at bytes 0-15, addr at bytes 16-47
  // Robust bitmask pad: byte 48 (bit 384)
  // PRF/simple thash1 payload: bytes 48-63, pad at byte 64 (bit 512)
  // Simple thash2 payload: bytes 48-79, pad at byte 80 (bit 640)
  always_comb begin
    rate_1_128 = '0;
    rate_1_128[127:0]   = pub_seed[127:0];
    rate_1_128[383:128] = addr;
    
    if (!simple_q && op_type_q != OP_TYPE_PRF) begin
      // Robust bitmask generation
      rate_1_128[391:384] = 8'h1F;
    end else if (op_type_q == OP_TYPE_THASH2) begin
      // Simple thash2: 2 input blocks
      rate_1_128[511:384] = input_block1[127:0];
      rate_1_128[639:512] = input_block2[127:0];
      rate_1_128[647:640] = 8'h1F;
    end else begin
      // PRF or simple thash1: 1 input block (sk_seed or input1)
      rate_1_128[511:384] = input_block1[127:0];
      rate_1_128[519:512] = 8'h1F;
    end
    rate_1_128[1087:1080] = 8'h80;
  end
  
  // ---- State 1: 192f ----
  // pub_seed[191:0] at bytes 0-23, addr at bytes 24-55
  // Robust bitmask pad: byte 56 (bit 448)
  // PRF/simple thash1 payload: bytes 56-79, pad at byte 80 (bit 640)
  // Simple thash2 payload: bytes 56-103, pad at byte 104 (bit 832)
  always_comb begin
    rate_1_192 = '0;
    rate_1_192[191:0]   = pub_seed[191:0];
    rate_1_192[447:192] = addr;
    
    if (!simple_q && op_type_q != OP_TYPE_PRF) begin
      rate_1_192[455:448] = 8'h1F;
    end else if (op_type_q == OP_TYPE_THASH2) begin
      rate_1_192[639:448] = input_block1[191:0];
      rate_1_192[831:640] = input_block2[191:0];
      rate_1_192[839:832] = 8'h1F;
    end else begin
      rate_1_192[639:448] = input_block1[191:0];
      rate_1_192[647:640] = 8'h1F;
    end
    rate_1_192[1087:1080] = 8'h80;
  end
  
  // ---- State 1: 256f ----
  // pub_seed[255:0] at bytes 0-31, addr at bytes 32-63
  // Robust bitmask pad: byte 64 (bit 512)
  // PRF/simple thash1 payload: bytes 64-95, pad at byte 96 (bit 768)
  // Simple thash2 payload: bytes 64-127, pad at byte 128 (bit 1024)
  always_comb begin
    rate_1_256 = '0;
    rate_1_256[255:0]   = pub_seed;
    rate_1_256[511:256] = addr;
    
    if (!simple_q && op_type_q != OP_TYPE_PRF) begin
      rate_1_256[519:512] = 8'h1F;
    end else if (op_type_q == OP_TYPE_THASH2) begin
      rate_1_256[767:512]   = input_block1;
      rate_1_256[1023:768]  = input_block2;
      rate_1_256[1031:1024] = 8'h1F;
    end else begin
      rate_1_256[767:512] = input_block1;
      rate_1_256[775:768] = 8'h1F;
    end
    rate_1_256[1087:1080] = 8'h80;
  end
  
  // State 1 mux
  always_comb begin
    case (sec_level_q)
      SEC_128: rate_portion_1 = rate_1_128;
      SEC_192: rate_portion_1 = rate_1_192;
      SEC_256: rate_portion_1 = rate_1_256;
      default: rate_portion_1 = rate_1_128;
    endcase
  end
  
  // ---- State 2: robust thash only (pub_seed || addr || masked_input || pad) ----
  
  // State 2: 128f
  // thash1: masked[127:0] at bytes 48-63, pad at byte 64
  // thash2: masked[255:0] at bytes 48-79, pad at byte 80
  always_comb begin
    rate_2_128 = '0;
    rate_2_128[127:0]   = pub_seed[127:0];
    rate_2_128[383:128] = addr;
    if (op_type_q == OP_TYPE_THASH2) begin
      rate_2_128[639:384] = masked_input[255:0];
      rate_2_128[647:640] = 8'h1F;
    end else begin
      rate_2_128[511:384] = masked_input[127:0];
      rate_2_128[519:512] = 8'h1F;
    end
    rate_2_128[1087:1080] = 8'h80;
  end
  
  // State 2: 192f
  // thash1: masked[191:0] at bytes 56-79, pad at byte 80
  // thash2: masked[383:0] at bytes 56-103, pad at byte 104
  always_comb begin
    rate_2_192 = '0;
    rate_2_192[191:0]   = pub_seed[191:0];
    rate_2_192[447:192] = addr;
    if (op_type_q == OP_TYPE_THASH2) begin
      rate_2_192[831:448] = masked_input[383:0];
      rate_2_192[839:832] = 8'h1F;
    end else begin
      rate_2_192[639:448] = masked_input[191:0];
      rate_2_192[647:640] = 8'h1F;
    end
    rate_2_192[1087:1080] = 8'h80;
  end
  
  // State 2: 256f
  // thash1: masked[255:0] at bytes 64-95, pad at byte 96
  // thash2: masked[511:0] at bytes 64-127, pad at byte 128
  always_comb begin
    rate_2_256 = '0;
    rate_2_256[255:0]   = pub_seed;
    rate_2_256[511:256] = addr;
    if (op_type_q == OP_TYPE_THASH2) begin
      rate_2_256[1023:512]  = masked_input[511:0];
      rate_2_256[1031:1024] = 8'h1F;
    end else begin
      rate_2_256[767:512] = masked_input[255:0];
      rate_2_256[775:768] = 8'h1F;
    end
    rate_2_256[1087:1080] = 8'h80;
  end
  
  // State 2 mux
  always_comb begin
    case (sec_level_q)
      SEC_128: rate_portion_2 = rate_2_128;
      SEC_192: rate_portion_2 = rate_2_192;
      SEC_256: rate_portion_2 = rate_2_256;
      default: rate_portion_2 = rate_2_128;
    endcase
  end
  
  // Assemble full 1600-bit states
  assign state_1 = {512'b0, rate_portion_1};
  assign state_2 = {512'b0, rate_portion_2};
  
  // Select keccak state based on FSM phase
  // IMPORTANT: Must hold state valid during WAIT phases for keccak_dp timing
  always_comb begin
    case (state_q)
      ABSORB_1, WAIT_1: keccak_state = state_1;
      ABSORB_2, WAIT_2: keccak_state = state_2;
      default:          keccak_state = '0;
    endcase
  end
  
  // ============================================================================
  // State machine
  // ============================================================================
  
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      state_q <= IDLE;
    end else begin
      state_q <= state_d;
    end
  end
  
  always_comb begin
    state_d = state_q;
    
    case (state_q)
      IDLE: begin
        if (is_any_launch) begin
          state_d = ABSORB_1;
        end
      end
      
      ABSORB_1: begin
        state_d = WAIT_1;
      end
      
      WAIT_1: begin
        if (keccak_done_i) begin
          // PRF or simple thash: done after 1 permutation
          // Robust thash: need second permutation for final hash
          if (op_type_q == OP_TYPE_PRF || simple_q) begin
            state_d = STORE_RESULT;
          end else begin
            state_d = ABSORB_2;
          end
        end
      end
      
      ABSORB_2: begin
        state_d = WAIT_2;
      end
      
      WAIT_2: begin
        if (keccak_done_i) begin
          state_d = STORE_RESULT;
        end
      end
      
      STORE_RESULT: begin
        state_d = IDLE;
      end
      
      default: state_d = IDLE;
    endcase
  end
  
  // ============================================================================
  // Output signals
  // ============================================================================
  
  assign sphincs_active_o = (state_q != IDLE);
  
  // Start keccak when entering absorb states
  assign sphincs_start_keccak_o = (state_q == ABSORB_1) || (state_q == ABSORB_2);
  
  // Keccak input state
  assign sphincs_keccak_input_o = keccak_state;
  
  // Done signal - single pulse at end of operation
  assign sphincs_done_o = (state_q == STORE_RESULT);
  
  // Writeback enable
  assign sphincs_wb_enable_o = (state_q == STORE_RESULT);
  
  // Writeback data: first N/4 words from keccak result (16/24/32 bytes)
  always_comb begin
    sphincs_wb_data_o = register_array_i;  // Preserve other registers
    case (sec_level_q)
      SEC_128: begin
        for (int i = 0; i < 4; i++)
          sphincs_wb_data_o[i] = keccak_result_i[i];
      end
      SEC_192: begin
        for (int i = 0; i < 6; i++)
          sphincs_wb_data_o[i] = keccak_result_i[i];
      end
      SEC_256: begin
        for (int i = 0; i < 8; i++)
          sphincs_wb_data_o[i] = keccak_result_i[i];
      end
      default: begin
        for (int i = 0; i < 4; i++)
          sphincs_wb_data_o[i] = keccak_result_i[i];
      end
    endcase
  end

endmodule
