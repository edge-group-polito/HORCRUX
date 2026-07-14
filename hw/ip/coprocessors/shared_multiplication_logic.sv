// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Shared Multiplication Logic                                            //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Unified Montgomery multiplication for ML-KEM, ML-DSA, and Falcon.      //
//               Resource-optimized with single auxiliary multiplier and muxed Q.      //
///////////////////////////////////////////////////////////////////////////////////////////

module shared_multiplication_logic (
    input  logic [31:0] op_a_i,
    input  logic [31:0] op_b_i,
    input  logic [31:0] modp_p_i,
    input  logic [31:0] modp_p0i_i,
    input  logic [2:0]  mode_i, 
    output logic [31:0] res_lo_o,
    output logic [31:0] res_hi_o  
);

    // =========================================================================
    // CONSTANTS
    // =========================================================================
    localparam logic [31:0] KEM_Q     = 32'd3329;
    localparam logic [15:0] KEM_QINV  = 16'hF301;
    localparam logic [31:0] DSA_Q     = 32'd8380417;
    localparam logic [31:0] DSA_QINV  = 32'd58728449;
    localparam logic [31:0] FALCON_Q  = 32'd12289;
    localparam logic [15:0] FAL_Q0I   = 16'd12287;

    // Mode encodings
    localparam logic [2:0] MODE_DSA_MONT  = 3'd0;
    localparam logic [2:0] MODE_KEM_MONT  = 3'd1;
    localparam logic [2:0] MODE_FAL_MONT  = 3'd2;
    localparam logic [2:0] MODE_HQC_RAW   = 3'd4;
    localparam logic [2:0] MODE_DSA_RED32 = 3'd5;
    localparam logic [2:0] MODE_MODP_MONT = 3'd6;

    // =========================================================================
    // PRIMARY MULTIPLIER: op_a * op_b
    // =========================================================================
    logic [63:0] full_prod;
    
    unified_mul_32x32 u_primary_mul (
        .a_i              (op_a_i),
        .b_i              (op_b_i), 
        .carryless_mode_i (mode_i == MODE_HQC_RAW), 
        .prod_o           (full_prod)
    );

    // =========================================================================
    // SHARED AUXILIARY MULTIPLIER with operand muxing
    // 
    // This single 32x32 multiplier handles ALL secondary multiplications:
    // - Kyber:     prod[15:0] * KEM_QINV, then t * KEM_Q
    // - Dilithium: prod[31:0] * DSA_QINV, then t * DSA_Q  
    // - Falcon:    prod[31:0] * FAL_Q0I,  then t * FALCON_Q
    // - reduce32:  t_reduce * DSA_Q
    //
    // Since these are all in the SAME combinational path (not concurrent),
    // we structure as a 2-stage computation with muxed operands.
    // =========================================================================
    
    // --- Stage A: Compute t = (prod_low * QINV) ---
    logic [31:0] stage_a_op1, stage_a_op2;
    logic [63:0] stage_a_result;
    logic [31:0] t_value;
    
    // Operand 1: product low bits (width depends on mode)
    always_comb begin
        unique case (mode_i)
            MODE_KEM_MONT:  stage_a_op1 = {16'b0, full_prod[15:0]};  // 16-bit
            MODE_DSA_MONT:  stage_a_op1 = full_prod[31:0];           // 32-bit
            MODE_FAL_MONT:  stage_a_op1 = full_prod[31:0];           // 32-bit (masked later)
            MODE_MODP_MONT: stage_a_op1 = {1'b0, full_prod[30:0]};   // low 31 bits of z
            default:        stage_a_op1 = '0;
        endcase
    end
    
    // Operand 2: QINV constant
    always_comb begin
        unique case (mode_i)
            MODE_KEM_MONT:  stage_a_op2 = {16'b0, KEM_QINV};
            MODE_DSA_MONT:  stage_a_op2 = DSA_QINV;
            MODE_FAL_MONT:  stage_a_op2 = {16'b0, FAL_Q0I};
            MODE_MODP_MONT: stage_a_op2 = modp_p0i_i;
            default:        stage_a_op2 = '0;
        endcase
    end
    
    // Multiplier instance (can be inferred or instantiated)
    assign stage_a_result = stage_a_op1 * stage_a_op2;
    
    // Extract and format t value based on mode
    always_comb begin
        unique case (mode_i)
            MODE_KEM_MONT:  t_value = {{16{stage_a_result[15]}}, stage_a_result[15:0]};  // sign-extend
            MODE_DSA_MONT:  t_value = stage_a_result[31:0];                               // truncate
            MODE_FAL_MONT:  t_value = {16'b0, stage_a_result[15:0]};                      // mask & zero-extend
            MODE_MODP_MONT: t_value = {1'b0, stage_a_result[30:0]};                       // keep only 31 bits
            default:        t_value = '0;
        endcase
    end

    // --- Stage B: Compute t * Q ---
    logic [31:0] stage_b_op1, stage_b_op2;
    logic signed [63:0] stage_b_result;
    
    // Operand 1: t value (or reduce32's t)
    logic signed [31:0] reduce32_t;
    assign reduce32_t = ($signed(op_a_i) + 32'sd4194304) >>> 23;
    
    always_comb begin
        unique case (mode_i)
            MODE_DSA_RED32: stage_b_op1 = reduce32_t;
            MODE_FAL_MONT:  stage_b_op1 = t_value;  // Already masked to 16-bit
            MODE_MODP_MONT: stage_b_op1 = t_value;
            default:        stage_b_op1 = t_value;
        endcase
    end
    
    // Operand 2: Q constant
    always_comb begin
        unique case (mode_i)
            MODE_KEM_MONT:   stage_b_op2 = KEM_Q;
            MODE_DSA_MONT:   stage_b_op2 = DSA_Q;
            MODE_FAL_MONT:   stage_b_op2 = FALCON_Q;
            MODE_MODP_MONT:  stage_b_op2 = modp_p_i;
            MODE_DSA_RED32:  stage_b_op2 = DSA_Q;
            default:         stage_b_op2 = '0;
        endcase
    end
    
    // Signed multiplication for t * Q (used by DSA/KEM/Falcon modes)
    assign stage_b_result = $signed({{32{stage_b_op1[31]}}, stage_b_op1}) * 
                            $signed({{32{1'b0}}, stage_b_op2});

    // Unsigned multiplication for modp path: t * p
    // Both t_value and modp_p_i are < 2^31 (Falcon primes), product fits in 62 bits
    logic [63:0] modp_tp;
    assign modp_tp = {32'b0, t_value} * {32'b0, modp_p_i};

    // =========================================================================
    // SHARED SUBTRACTOR: result = input - t*Q
    // =========================================================================
    logic signed [63:0] minuend;      // What we subtract FROM
    logic signed [63:0] subtrahend;   // What we subtract
    logic signed [63:0] difference;
    
    always_comb begin
        unique case (mode_i)
            MODE_DSA_RED32: minuend = $signed({{32{op_a_i[31]}}, op_a_i});
            default:        minuend = $signed(full_prod);
        endcase
    end
    
    assign subtrahend = stage_b_result;
    assign difference = minuend - subtrahend;

    // =========================================================================
    // FALCON SPECIAL PATH
    // (z + ((z * Q0I) & 0xFFFF) * Q) >> 16, then conditional subtract
    // =========================================================================
    logic [31:0] falcon_w;
    logic [31:0] falcon_sum;
    logic [31:0] falcon_shifted;
    logic [31:0] falcon_final;
    logic [63:0] modp_sum;
    logic [31:0] modp_shifted;
    logic [31:0] modp_sub;
    logic [31:0] modp_final;
    
    // w = ((t & 0xFFFF) * Q) - reuses stage_b_result low bits
    assign falcon_w = stage_b_result[31:0];  // t_masked * Q
    assign falcon_sum = full_prod[31:0] + falcon_w;
    assign falcon_shifted = falcon_sum >> 16;
    assign falcon_final = (falcon_shifted >= FALCON_Q) ? 
                          (falcon_shifted - FALCON_Q) : falcon_shifted;

    // z = a*b (both 31-bit unsigned), w = (z_lo31 * p0i & 0x7FFFFFFF) * p
    // Use purely unsigned arithmetic throughout
    logic [63:0] modp_z;
    assign modp_z       = {2'b0, full_prod[61:0]};  // z = a*b, both <=2^31-1, so z < 2^62
    assign modp_sum     = modp_z + modp_tp;
    assign modp_shifted = modp_sum[62:31];           // (z + w) >> 31, result fits in 32 bits (z+w < 2^63)
    assign modp_sub     = modp_shifted[31:0] - modp_p_i;
    assign modp_final   = modp_sub + (modp_p_i & {32{modp_sub[31]}});

    // =========================================================================
    // OUTPUT MUX
    // =========================================================================
    always_comb begin
        res_lo_o = '0;
        res_hi_o = '0;
        
        unique case (mode_i)
            MODE_KEM_MONT: begin
                // (prod - t*Q) >> 16, but need to handle sign correctly
                // difference is 64-bit signed, we want bits [47:16] as signed 16-bit result
                res_lo_o = difference[47:16];
            end
            
            MODE_DSA_MONT: begin
                // (prod - t*Q) >> 32
                res_lo_o = difference[63:32];
            end
            
            MODE_FAL_MONT: begin
                res_lo_o = falcon_final;
            end
            
            MODE_HQC_RAW: begin
                res_lo_o = full_prod[31:0];
                res_hi_o = full_prod[63:32];
            end

            MODE_MODP_MONT: begin
                res_lo_o = modp_final;
                res_hi_o = 32'b0;
            end
            
            MODE_DSA_RED32: begin
                res_lo_o = difference[31:0];
            end
            
            
            default: begin
                res_lo_o = full_prod[31:0];
            end
        endcase
    end

endmodule
