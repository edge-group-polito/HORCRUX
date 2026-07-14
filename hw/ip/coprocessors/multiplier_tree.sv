//////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Unified Multiplier Tree - NTT Butterflies & Montgomery                //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Cooley-Tukey and Gentleman-Sande NTT butterflies for ML-KEM/ML-DSA,   //
//               Falcon NTT operations, Montgomery multiplication, Karatsuba shifts.    //
//////////////////////////////////////////////////////////////////////////////////////////



module multiplier_tree (
    input  logic clk_i,
    input  logic rst_ni,
    input  logic [31:0] multiplier_tree1_i,  // rs1 (A)
    input  logic [31:0] multiplier_tree2_i,  // rs2 (B)
    input  logic [31:0] multiplier_tree3_i,  // zeta
    input  horcrux_pkg::horcrux_insn insn_i,
    output logic [31:0] result_o,
    output logic [31:0] result_hi_o
);
    import horcrux_pkg::*;

    // Internal registers for HQC Karatsuba
    logic [31:0] a1_reg, b1_reg;  // Store a_hi, b_hi from KARATS_2
    logic [31:0] reg_A, reg_B;    // Accumulator registers for intermediate XOR results
    
    // Internal wires
    logic [31:0] mul_op_a, mul_op_b, mul_res_lo, mul_res_hi;
    logic [2:0]  mul_mode;
    logic [31:0] modp_p0i;

    // Barrett reduction for Kyber INTT
    logic [31:0] barrett_input;
    logic [31:0] barrett_result;
    
    // GF reduce - full reduction mod 0x11D in one cycle
    logic [31:0] gf_reduce_result;
    logic [11:0] gf_pass1, gf_pass2;
    logic [7:0] gf_mod1, gf_mod2;

    // compare_u32 - constant-time equality comparison
    // Returns 1 if v1 == v2, 0 otherwise: 1 ^ (((v1-v2) | (v2-v1)) >> 31)
    logic [31:0] compare_u32_result;
    logic [31:0] cmp_v1_minus_v2, cmp_v2_minus_v1;

    function automatic logic [31:0] modp_p0i_lookup(input logic [31:0] p);
        begin
            unique case (p)
                32'd2147473409: modp_p0i_lookup = 32'd2042615807;
                32'd2147389441: modp_p0i_lookup = 32'd1862176767;
                32'd2147387393: modp_p0i_lookup = 32'd1472104447;
                32'd2147377153: modp_p0i_lookup = 32'd1543397375;
                default:        modp_p0i_lookup = 32'd0;
            endcase
        end
    endfunction
    
    // Count trailing zeros function (CTZ)
    function automatic logic [5:0] ctz32(input logic [31:0] x);
        logic [5:0] cnt;
        begin
            cnt = 6'd32;  // Return 32 if input is 0
            for (int i = 31; i >= 0; i--) begin
                if (x[i])
                    cnt = 6'(i);
            end
            ctz32 = cnt;
        end
    endfunction

    // --- 1. Multiplier Operand Selection Mux ---
    always_comb begin
        mul_op_a = multiplier_tree3_i; // Default: Zeta
        mul_op_b = multiplier_tree2_i; // Default: rs2 (B)
        mul_mode = 3'd0;               // Default: DSA mode
        barrett_input = '0;

        case (insn_i)
            OP_MODP_MONTYMUL: begin
                mul_mode = 3'd6;
                mul_op_a = multiplier_tree1_i;
                mul_op_b = multiplier_tree2_i;
            end

            // Kyber Butterflies (SISO)
            OP_BFNTTK: begin
                mul_mode = 3'd1;
                mul_op_b = multiplier_tree2_i;
            end
            OP_BFINTTK: begin
                mul_mode = 3'd1;
                barrett_input = {{16{multiplier_tree1_i[15]}}, multiplier_tree1_i[15:0]} + 
                    {{16{multiplier_tree2_i[15]}}, multiplier_tree2_i[15:0]};
                mul_op_b = $signed(multiplier_tree2_i[15:0]) - $signed(multiplier_tree1_i[15:0]);
            end
            OP_BFNTTF: begin
                mul_mode = 3'd2; // Falcon Montgomery
                mul_op_b = multiplier_tree2_i;
            end
            OP_BFINTTF: begin
                mul_mode = 3'd2; // Falcon Montgomery
                // Modular subtraction: (a - b) mod Q for 16-bit Falcon coefficients
                mul_op_b = (multiplier_tree1_i[15:0] >= multiplier_tree2_i[15:0]) ?
                           {16'b0, multiplier_tree1_i[15:0] - multiplier_tree2_i[15:0]} :
                           {16'b0, multiplier_tree1_i[15:0] - multiplier_tree2_i[15:0] + 16'd12289};
            end
            OP_BARRETT: begin
                barrett_input = multiplier_tree1_i;
            end
            OP_BARRETT_HQC, OP_BARRETT_HQC3, OP_BARRETT_HQC5: begin
                barrett_input = multiplier_tree1_i;
            end
            // Dilithium Butterflies
            OP_BFNTTD: begin
                mul_mode = 3'd0; 
            end
            OP_BFINTTD: begin
                mul_mode = 3'd0;
                mul_op_b = multiplier_tree1_i - multiplier_tree2_i;
            end
            // Standalone Multiplications
            OP_MQMULF, OP_MQMULK, OP_MQMULD: begin
                mul_mode = (insn_i == OP_MQMULD) ? 3'd0 : 
                           (insn_i == OP_MQMULK) ? 3'd1 : 3'd2;
                mul_op_a = multiplier_tree1_i;
                mul_op_b = multiplier_tree2_i;
            end
            // HQC Karatsuba
            OP_KARATS_1: begin
                // P0 = a_lo * b_lo (carryless)
                mul_mode = 3'd4;
                mul_op_a = multiplier_tree1_i;  // a_lo
                mul_op_b = multiplier_tree2_i;  // b_lo
            end
            OP_KARATS_2: begin
                // P2 = a_hi * b_hi (carryless)
                mul_mode = 3'd4;
                mul_op_a = multiplier_tree1_i;  // a_hi
                mul_op_b = multiplier_tree2_i;  // b_hi
            end
            OP_KARATS_3: begin
                // P1 = (a_lo ^ a_hi) * (b_lo ^ b_hi) (carryless)
                mul_mode = 3'd4;
                mul_op_a = multiplier_tree1_i ^ a1_reg;  // a_lo ^ a_hi
                mul_op_b = multiplier_tree2_i ^ b1_reg;  // b_lo ^ b_hi
            end
            OP_GFMUL8: begin
                mul_mode = 3'd4;
                mul_op_a = {24'b0, multiplier_tree1_i[7:0]};
                mul_op_b = {24'b0, multiplier_tree2_i[7:0]};
            end
            OP_RED32: begin
                mul_mode = 3'd5; // Custom mode for conditional addition of Q
                mul_op_a = multiplier_tree1_i; // Input to conditionally reduce
            end
            default: begin
                mul_mode = 3'd0;
            end
        endcase
    end

    assign modp_p0i = modp_p0i_lookup(multiplier_tree3_i);

    barrett kyber_barrett_inst (
        .a(barrett_input),
        .b(multiplier_tree2_i),
        .insn_i(insn_i),
        .result(barrett_result)
    );
    
    // GF Reduce: Full GF(2^8) reduction mod 0x11D = x^8 + x^4 + x^3 + x^2 + 1
    // Pass 1: reduce bits [15:8] using polynomial taps at positions 4,3,2
    assign gf_mod1 = multiplier_tree1_i[15:8];
    assign gf_pass1 = {4'b0, multiplier_tree1_i[7:0]} ^ 
                      {4'b0, gf_mod1} ^                    // x^0 tap (mod itself)
                      {2'b0, gf_mod1, 2'b0} ^              // x^2 tap (mod << 2)
                      {1'b0, gf_mod1, 3'b0} ^              // x^3 tap (mod << 3)
                      {gf_mod1, 4'b0};                      // x^4 tap (mod << 4)
    // Pass 2: reduce any remaining bits [11:8] from pass 1
    assign gf_mod2 = {4'b0, gf_pass1[11:8]};
    assign gf_pass2 = {4'b0, gf_pass1[7:0]} ^ 
                      {4'b0, gf_mod2} ^                    // x^0 tap
                      {2'b0, gf_mod2, 2'b0} ^              // x^2 tap
                      {1'b0, gf_mod2, 3'b0} ^              // x^3 tap
                      {gf_mod2, 4'b0};                      // x^4 tap
    assign gf_reduce_result = {24'b0, gf_pass2[7:0]};

    // compare_u32: Constant-time equality check
    // Returns 1 if rs1 == rs2, 0 otherwise
    // Formula: 1 ^ (((v1-v2) | (v2-v1)) >> 31)
    assign cmp_v1_minus_v2 = multiplier_tree1_i - multiplier_tree2_i;
    assign cmp_v2_minus_v1 = multiplier_tree2_i - multiplier_tree1_i;
    assign compare_u32_result = 32'd1 ^ {31'b0, (cmp_v1_minus_v2 | cmp_v2_minus_v1) >> 31};

    shared_multiplication_logic u_shared_mul (
        .op_a_i(mul_op_a),
        .op_b_i(mul_op_b),
        .modp_p_i(multiplier_tree3_i),
        .modp_p0i_i(modp_p0i),
        .mode_i(mul_mode),
        .res_lo_o(mul_res_lo),
        .res_hi_o(mul_res_hi)
    );



    // Falcon modular reduction constants and signals
    localparam logic [16:0] FALCON_Q = 17'd12289;
    
    logic [15:0] a, b, t;
    logic [15:0] temp_result_a, temp_result_b;
    
    // Falcon butterfly intermediate signals (need 17 bits for overflow handling)
    logic signed [16:0] falcon_a_plus_t, falcon_a_minus_t;
    logic signed [16:0] falcon_a_plus_b, falcon_a_minus_b;
    logic [15:0] falcon_res_a, falcon_res_b;
    logic [15:0] falcon_intt_res_a, falcon_intt_res_b;

    // --- 2. Butterfly ALU and Output Mux ---
    // 
    // KARATSUBA OUTPUT MAPPING (critical!):
    // 
    // For c = a * b where a,b are 64-bit and c is 128-bit:
    //   c0 = c[63:0],  c1 = c[127:64]
    //
    // Test code assembles results as:
    //   c0 = KARATS_1.rd | (KARATS_3.rd << 32)   => c0 = {c0_hi, c0_lo}
    //   c1 = KARATS_4.rd | (KARATS_2.rd << 32)   => c1 = {c1_hi, c1_lo}
    //
    // Therefore:
    //   KARATS_1.rd must output: c0_lo = P0_lo
    //   KARATS_2.rd must output: c1_hi = P2_hi
    //   KARATS_3.rd must output: c0_hi
    //   KARATS_4.rd must output: c1_lo
    //
    always_comb begin
        result_o    = mul_res_lo;
        result_hi_o = mul_res_hi;
        a           = 16'(0);
        b           = 16'(0);
        t           = 16'(0);
        temp_result_a = 16'(0);
        temp_result_b = 16'(0);
        falcon_a_plus_t = 17'(0);
        falcon_a_minus_t = 17'(0);
        falcon_a_plus_b = 17'(0);
        falcon_a_minus_b = 17'(0);
        falcon_res_a = 16'(0);
        falcon_res_b = 16'(0);
        falcon_intt_res_a = 16'(0);
        falcon_intt_res_b = 16'(0);

        case (insn_i)
            OP_BFNTTK: begin
                a = multiplier_tree1_i[15:0];
                t = mul_res_lo[15:0];
                temp_result_a = a + t;
                temp_result_b = a - t;
                result_o = {temp_result_b, temp_result_a};
            end
            OP_BFINTTK: begin
                result_o = {mul_res_lo[15:0], 16'(barrett_result)};
            end
            OP_BFNTTF: begin
                // Falcon Forward NTT: a' = (a + t) mod q, b' = (a - t) mod q
                a = multiplier_tree1_i[15:0];
                t = mul_res_lo[15:0];
                
                // Compute a + t - q (17-bit signed to detect underflow)
                falcon_a_plus_t = $signed({1'b0, a}) + $signed({1'b0, t}) - $signed(FALCON_Q);
                // If negative, add q back
                falcon_res_a = falcon_a_plus_t[16] ? (falcon_a_plus_t[15:0] + FALCON_Q[15:0]) : falcon_a_plus_t[15:0];
                
                // Compute a - t (17-bit signed to detect underflow)
                falcon_a_minus_t = $signed({1'b0, a}) - $signed({1'b0, t});
                // If negative, add q back
                falcon_res_b = falcon_a_minus_t[16] ? (falcon_a_minus_t[15:0] + FALCON_Q[15:0]) : falcon_a_minus_t[15:0];
                
                result_o = {falcon_res_b, falcon_res_a};
            end
            OP_BFINTTF: begin
                // Falcon Inverse NTT: a' = (a + b) mod q, b' = (a - b) * zeta mod q
                a = multiplier_tree1_i[15:0];
                b = multiplier_tree2_i[15:0];
                
                // Compute a + b - q
                falcon_a_plus_b = $signed({1'b0, a}) + $signed({1'b0, b}) - $signed(FALCON_Q);
                falcon_intt_res_a = falcon_a_plus_b[16] ? (falcon_a_plus_b[15:0] + FALCON_Q[15:0]) : falcon_a_plus_b[15:0];
                
                // b' comes from Montgomery multiplication (already reduced)
                falcon_intt_res_b = mul_res_lo[15:0];
                
                result_o = {falcon_intt_res_b, falcon_intt_res_a};
            end
            OP_BARRETT, OP_BARRETT_HQC, OP_BARRETT_HQC3, OP_BARRETT_HQC5: begin
                result_o = barrett_result;
            end
            OP_BFNTTD: begin
                result_o    = multiplier_tree1_i + mul_res_lo;
                result_hi_o = multiplier_tree1_i - mul_res_lo;
            end
            OP_BFNTTDH: begin
                result_o = reg_A;
            end
            OP_BFINTTD: begin
                result_o    = multiplier_tree1_i + multiplier_tree2_i;
                result_hi_o = mul_res_lo; 
            end
            OP_BFINTTDH: begin
                result_o = reg_A;
            end

            OP_MQMULF, OP_MQMULK, OP_MQMULD: begin
                result_o    = mul_res_lo;
                result_hi_o = mul_res_hi;
            end

            OP_MODP_MONTYMUL: begin
                result_o    = mul_res_lo;
                result_hi_o = 32'b0;
            end
            
            // --- KARATSUBA INSTRUCTIONS ---
            OP_KARATS_1: begin
                // Output: rd = P0_lo (which becomes c0_lo)
                // P0 = a_lo * b_lo
                // We output P0_lo, and store P0_lo, P0_hi in registers for later
                result_o    = mul_res_lo;   // c0_lo = P0_lo
                result_hi_o = mul_res_hi;   // Not used by test, but available
            end
            
            OP_KARATS_2: begin
                // Output: rd = P2_hi (which becomes c1_hi)
                // P2 = a_hi * b_hi
                // CRITICAL: Test expects rd = P2_hi, not P2_lo!
                result_o    = mul_res_hi;   // c1_hi = P2_hi
                result_hi_o = mul_res_lo;   // P2_lo (stored for KARATS_3 computation)
            end
            
            OP_KARATS_3: begin
                // Output: rd = c0_hi
                // P1 = (a_lo ^ a_hi) * (b_lo ^ b_hi)
                // c0_hi = P0_lo ^ P0_hi ^ P2_lo ^ P1_lo = reg_A ^ P1_lo
                result_o    = mul_res_lo ^ reg_A;   // c0_hi
                result_hi_o = mul_res_hi ^ reg_B;   // c1_lo (stored for KARATS_4)
            end
            
            OP_KARATS_4: begin
                // Output: rd = c1_lo (read from register)
                result_o = reg_A;   // c1_lo was stored here after KARATS_3
            end
            
            OP_RED32: begin
                result_o = mul_res_lo;
            end

            OP_GF_REDUCE: begin
                // DEBUG: return raw input to verify data path works
                // result_o = multiplier_tree1_i;  // Uncomment to debug
                result_o = gf_reduce_result;
            end

            OP_COMPARE_U32: begin
                result_o = compare_u32_result;
            end

            default: begin
                result_o    = mul_res_lo;
                result_hi_o = mul_res_hi;
            end
        endcase
    end

    // --- 3. Sequential State Updates ---
    //
    // After KARATS_1: 
    //   reg_A = P0_lo ^ P0_hi  (partial sum for c0_hi)
    //   reg_B = P0_hi          (partial sum for c1_lo)
    //
    // After KARATS_2:
    //   a1_reg = a_hi, b1_reg = b_hi  (for KARATS_3 XOR inputs)
    //   reg_A = P0_lo ^ P0_hi ^ P2_lo  (updated partial for c0_hi)
    //   reg_B = P0_hi ^ P2_lo ^ P2_hi  (updated partial for c1_lo)
    //
    // After KARATS_3:
    //   reg_A = c1_lo = P1_hi ^ reg_B  (stored for KARATS_4 to read)
    //
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            a1_reg <= '0;
            b1_reg <= '0;
            reg_A  <= '0;
            reg_B  <= '0;
        end else begin
            case (insn_i)
                OP_KARATS_1: begin
                    // P0 = a_lo * b_lo computed
                    // Prepare partial sums:
                    reg_A <= mul_res_lo ^ mul_res_hi;  // P0_lo ^ P0_hi
                    reg_B <= mul_res_hi;               // P0_hi
                end
                
                OP_KARATS_2: begin
                    // P2 = a_hi * b_hi computed
                    // Save inputs for KARATS_3:
                    a1_reg <= multiplier_tree1_i;  // a_hi
                    b1_reg <= multiplier_tree2_i;  // b_hi
                    // Update partial sums:
                    reg_A <= mul_res_lo ^ reg_A;              // P2_lo ^ P0_lo ^ P0_hi
                    reg_B <= mul_res_lo ^ mul_res_hi ^ reg_B; // P2_lo ^ P2_hi ^ P0_hi
                end
                
                OP_KARATS_3: begin
                    // P1 = (a_lo ^ a_hi) * (b_lo ^ b_hi) computed
                    // Store c1_lo for KARATS_4 to read:
                    // c1_lo = P1_hi ^ reg_B = P1_hi ^ P2_lo ^ P2_hi ^ P0_hi
                    reg_A <= mul_res_hi ^ reg_B;
                end
                
                OP_BFNTTD: begin
                    reg_A <= result_hi_o;
                end
                
                OP_BFINTTD: begin
                    reg_A <= result_hi_o;
                end

                default: begin
                    // Keep registers stable
                end
            endcase
        end
    end



endmodule
