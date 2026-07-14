//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Unified Sampler Unit - ML-KEM/ML-DSA sampling operations               //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  CBD, rejection sampling, and coefficient unpacking for ML-KEM/ML-DSA. //
//               Supports REJ_UNIFORM, REJ_ETA, and UNPACK_Z operations.               //
//////////////////////////////////////////////////////////////////////////////////////////


module sampler (
    input  logic [31:0] data_i,     // Primary data input (rs1)
    input  logic [31:0] index_i,    // Index/selector input (rs2)
    input  horcrux_pkg::horcrux_insn insn_i,
    output logic [31:0] result_o    // Result output (may include valid bit in MSB)
);

    import horcrux_pkg::*;

    //==========================================================================
    // ML-DSA Parameters
    //==========================================================================
    localparam logic [22:0] Q_MLDSA = 23'd8380417;  // ML-DSA modulus
    localparam logic [17:0] GAMMA1  = 18'd131072;   // 2^17 for ML-DSA-44

    //==========================================================================
    // CBD (Centered Binomial Distribution) - Existing functionality
    //==========================================================================
    // Supports eta = 1, 2, 3, 4 for ML-KEM and ML-DSA
    
    logic [2:0] eta;
    logic [5:0] stride;
    logic [31:0] mask;
    logic [31:0] sh_a, sh_b;
    logic [31:0] a_bits, b_bits;
    logic signed [7:0] cbd_diff;
    logic [4:0] j_masked;
    logic [31:0] cbd_result;

    // Decode eta from instruction
    always_comb begin
        unique case (insn_i)
            OP_CBD1: eta = 3'd1;
            OP_CBD2: eta = 3'd2;
            OP_CBD3: eta = 3'd3;
            OP_CBD4: eta = 3'd4;
            default: eta = 3'd0;
        endcase
    end

    // CBD computation
    always_comb begin
        j_masked = index_i[4:0];
        stride   = {eta, 1'b0};                    // Stride = 2*eta
        mask     = (32'h1 << eta) - 32'h1;         // Mask = 2^eta - 1
        
        sh_a     = j_masked * stride;
        sh_b     = sh_a + {29'b0, eta};
        
        a_bits   = (data_i >> sh_a) & mask;
        b_bits   = (data_i >> sh_b) & mask;
        
        cbd_diff = $signed(a_bits[7:0]) - $signed(b_bits[7:0]);
        cbd_result = $signed(cbd_diff);
    end

    //==========================================================================
    // REJ_UNIFORM - Rejection Sampling for Matrix A (ML-DSA)
    //==========================================================================
    // Input:  data_i[23:0] = 3 bytes from SHAKE128 stream
    // Output: result_o = {valid[31], 8'b0, coeff[22:0]}
    //         valid = 1 if coefficient < Q, else 0
    
    logic [22:0] rej_uniform_t;
    logic        rej_uniform_valid;
    logic [31:0] rej_uniform_result;

    assign rej_uniform_t     = data_i[22:0];  // Mask to 23 bits
    assign rej_uniform_valid = (rej_uniform_t < Q_MLDSA);
    // When rejected (valid=0), output 0; when accepted, output {1, 8'b0, coeff}
    assign rej_uniform_result = rej_uniform_valid ? {1'b1, 8'b0, rej_uniform_t} : 32'd0;

    //==========================================================================
    // REJ_ETA - Nibble-based Rejection for Secret Vectors (ML-DSA)
    //==========================================================================
    // Input:  data_i[7:0] = byte containing two 4-bit nibbles
    //         index_i[0]  = nibble selector (0=low, 1=high)
    // Output: result_o = {valid[31], sign_extended_coeff[30:0]}
    //
    // For ETA=2: reject if nibble >= 15, output = 2 - (nibble mod 5)
    // For ETA=4: reject if nibble >= 9,  output = 4 - nibble
    
    logic [3:0]  rej_eta_nibble;
    logic        rej_eta_nibble_sel;
    logic        rej_eta2_valid, rej_eta4_valid;
    logic [31:0] rej_eta2_result, rej_eta4_result;
    
    // Nibble computation for mod 5: t - ((205*t) >> 10) * 5
    // This approximates t mod 5 for t in [0, 14]
    logic [13:0] rej_eta2_mult;      // 205 * nibble (max 205*14 = 2870)
    logic [3:0]  rej_eta2_div5;      // (205*nibble) >> 10 (max 2)
    logic [4:0]  rej_eta2_mod5_sub;  // div5 * 5 (max 10)
    logic [3:0]  rej_eta2_mod5;      // nibble - mod5_sub = nibble mod 5
    logic signed [2:0] rej_eta2_coeff;  // 2 - mod5 in [-2, +2]
    
    logic signed [3:0] rej_eta4_coeff;  // 4 - nibble in [-4, +4]

    assign rej_eta_nibble_sel = index_i[0];
    assign rej_eta_nibble = rej_eta_nibble_sel ? data_i[7:4] : data_i[3:0];

    // ETA=2 computation: coeff = 2 - (nibble mod 5)
    assign rej_eta2_mult     = 12'd205 * {8'b0, rej_eta_nibble};
    assign rej_eta2_div5     = rej_eta2_mult[13:10];  // >> 10
    assign rej_eta2_mod5_sub = rej_eta2_div5 * 4'd5;
    assign rej_eta2_mod5     = rej_eta_nibble - rej_eta2_mod5_sub[3:0];
    assign rej_eta2_coeff    = 3'sd2 - $signed({1'b0, rej_eta2_mod5[2:0]});
    assign rej_eta2_valid    = (rej_eta_nibble < 4'd15);
    // When rejected (valid=0), output 0; when accepted, output {1, sign_ext, coeff}
    assign rej_eta2_result   = rej_eta2_valid ? {1'b1, {28{rej_eta2_coeff[2]}}, rej_eta2_coeff} : 32'd0;

    // ETA=4 computation: coeff = 4 - nibble
    assign rej_eta4_coeff  = 4'sd4 - $signed({1'b0, rej_eta_nibble[2:0]});
    assign rej_eta4_valid  = (rej_eta_nibble < 4'd9);
    // When rejected (valid=0), output 0; when accepted, output {1, sign_ext, coeff}
    assign rej_eta4_result = rej_eta4_valid ? {1'b1, {27{rej_eta4_coeff[3]}}, rej_eta4_coeff} : 32'd0;

    //==========================================================================
    // UNPACK_Z - γ1-range Coefficient Unpacking (ML-DSA ExpandMask)
    //==========================================================================
    // Input:  data_i  = 32-bit word from packed 9-byte buffer
    //         index_i = extraction mode/offset
    //
    // For GAMMA1 = 2^17: 4 coefficients in 9 bytes (18 bits each)
    // Bytes layout: [b0 b1 b2 b3 b4 b5 b6 b7 b8] → 4 coefficients
    //   coeff0 = b0 | (b1 << 8) | ((b2 & 0x03) << 16)
    //   coeff1 = (b2 >> 2) | (b3 << 6) | ((b4 & 0x0F) << 14)
    //   coeff2 = (b4 >> 4) | (b5 << 4) | ((b6 & 0x3F) << 12)
    //   coeff3 = (b6 >> 6) | (b7 << 2) | (b8 << 10)
    //
    // Simplified: input word + extraction selector → one 18-bit coefficient
    // Then transform: output = GAMMA1 - extracted_value
    
    logic [17:0] unpack_z_raw;
    logic signed [31:0] unpack_z_result;
    logic [1:0]  unpack_z_sel;

    assign unpack_z_sel = index_i[1:0];

    // Extract 18-bit value based on selector
    // Assumes data_i contains pre-aligned 18-bit coefficient
    // (software must load appropriate bytes)
    always_comb begin
        unique case (unpack_z_sel)
            2'd0: unpack_z_raw = data_i[17:0];                    // First 18 bits
            2'd1: unpack_z_raw = data_i[19:2];                    // Bits [19:2]
            2'd2: unpack_z_raw = data_i[21:4];                    // Bits [21:4]
            2'd3: unpack_z_raw = data_i[23:6];                    // Bits [23:6]
            default: unpack_z_raw = 18'd0;
        endcase
    end

    // Transform: coeff = GAMMA1 - raw_value
    // GAMMA1 = 131072 (2^17), raw in [0, 2^18-1]
    // Result in [-(GAMMA1-1), GAMMA1] = [-131071, 131072]
    assign unpack_z_result = $signed({14'b0, GAMMA1}) - $signed({14'b0, unpack_z_raw});

    //==========================================================================
    // Output Multiplexer
    //==========================================================================
    always_comb begin
        result_o = 32'd0;
        
        unique case (insn_i)
            // CBD operations (existing)
            OP_CBD1, OP_CBD2, OP_CBD3, OP_CBD4: begin
                result_o = cbd_result;
            end
            
            // Rejection sampling for matrix A
            OP_REJ_UNIFORM: begin
                result_o = rej_uniform_result;
            end
            
            // Rejection sampling for secret vectors
            OP_REJ_ETA2: begin
                result_o = rej_eta2_result;
            end
            
            OP_REJ_ETA4: begin
                result_o = rej_eta4_result;
            end
            
            // Gamma1 unpacking for mask y
            OP_UNPACK_Z: begin
                result_o = unpack_z_result;
            end
            
            default: begin
                result_o = 32'd0;
            end
        endcase
    end

endmodule
