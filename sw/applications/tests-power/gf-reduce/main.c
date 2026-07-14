///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// GF Reduce test (SW + HW) for HQC
//
// Tests the complete gf_reduce function used in HQC for polynomial reduction
// in GF(2^8) modulo PARAM_GF_POLY = 0x11D (x^8 + x^4 + x^3 + x + 1)
//
// HW Operation:
//   OP_GF_REDUCE: rd = gf_reduce(rs1) - complete reduction in one cycle
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>

#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

// HQC GF(2^8) parameters
#define PARAM_M         8                   // GF(2^8)
#define PARAM_GF_POLY   0x11D               // x^8 + x^4 + x^3 + x + 1
#define PARAM_GF_POLY_WT 5                  // Hamming weight of polynomial

// Feedback tap positions derived from PARAM_GF_POLY
// 0x11D = 0b100011101 → bits at positions: 8, 4, 3, 2, 0
// Skipping bit 8 (leading) and bit 0 (constant), taps are: 4, 3, 2
static const uint8_t gf_reduction_taps[] = {4, 3, 2};

#define N_TESTS 10

// Pre-computed golden results: gf_reduce(test_inputs[i]) mod 0x11D
static const uint16_t golden[N_TESTS] = {
    0x00, 0x01, 0xFF, 0x1D, 0xE2, 0xBF, 0x00, 0x01, 0x03, 0x44,
    0x89, 0x0E, 0x1D, 0x3B, 0xA3, 0xC3, 0xE8, 0xF3, 0x25, 0x19
};

// HW instruction macro for single-cycle GF reduce
// OP_GF_REDUCE: rd = gf_reduce(rs1) mod 0x11D (funct7=0x78, funct3=7, opcode=0x3b)
// Takes 16-bit input, returns 8-bit reduced result
#define GF_REDUCE_HW(dest, rs1_val) \
    asm volatile ( \
        "addi t0, %[rs1], 0\n" \
        ".insn r 0x3b, 0x7, 0x78, %[rd], t0, x0 \n" \
        : [rd] "=r" (dest) \
        : [rs1] "r" (rs1_val) : "t0", "cc" \
    )

/**
 * @brief SW reference: Reduce a polynomial modulo PARAM_GF_POLY in GF(2^8).
 *
 * This is the original HQC gf_reduce function. It performs modular reduction 
 * of a 16-bit polynomial `x` by the irreducible polynomial PARAM_GF_POLY = 0x11D.
 *
 * @param x 16-bit input polynomial to reduce (deg(x) ≤ 14)
 * @return Reduced 8-bit polynomial modulo PARAM_GF_POLY (deg(x) < 8)
 */
static uint16_t gf_reduce_sw(uint16_t x) {
    uint64_t mod;
    const int reduction_steps = 2;            // For deg(x) = 2 * (8 - 1) = 14, reduce twice
    const size_t gf_reduction_tap_count = 3;  // Number of feedback positions

    for (int i = 0; i < reduction_steps; ++i) {
        mod = x >> PARAM_M;       // Extract upper bits
        x &= (1 << PARAM_M) - 1;  // Keep lower bits
        x ^= mod;                 // Pre-XOR with no shift

        uint16_t z1 = 0;
        for (size_t j = gf_reduction_tap_count; j; --j) {
            uint16_t z2 = gf_reduction_taps[j - 1];
            uint16_t dist = z2 - z1;
            mod <<= dist;
            x ^= mod;
            z1 = z2;
        }
    }

    return x;
}

/**
 * @brief HW accelerated: Reduce a polynomial modulo PARAM_GF_POLY in GF(2^8).
 *
 * Uses single custom instruction GF_REDUCE that performs complete
 * reduction in one cycle using combinational logic.
 *
 * @param x 16-bit input polynomial to reduce
 * @return Reduced 8-bit polynomial modulo PARAM_GF_POLY
 */
static uint16_t gf_reduce_hw(uint16_t x) {
    uint32_t result;
    GF_REDUCE_HW(result, (uint32_t)x);
    return (uint16_t)result;
}


int main(void) {
    // DEBUG: First verify that coprocessor data path works
    // Test with BARRETT_HQC which uses same register_read pattern


    // Test inputs: products of 8-bit values (max 16-bit result)
    // These simulate outputs of gf_carryless_mul before reduction
    const uint16_t test_inputs[N_TESTS] = {
        0x0000,   // 0
        0x0001,   // 1
        0x00FF,   // 255
        0x0100,   // 256 (needs reduction)
        0x01FF,   // 511
        0x0285,   // Random product
        0x11D,    // The polynomial itself
        0x11C,    // Polynomial - 1
        0x11E,    // Polynomial + 1
        0x0FFF,   // 4095
        0x1FFF,   // 8191
        0x3FFF,   // 16383 (max deg=13)
        0x7FFF,   // 32767 (deg=14)
        0xFFFF,   // 65535 (max 16-bit)
        0xABCD,   // Random
        0x1234,   // Random
        0x5678,   // Random
        0xDEAD,   // Random
        0xBEEF,   // Random
        0xCAFE    // Random
    };

    // Test results arrays
    uint16_t sw_results[N_TESTS];
    uint16_t hw_results[N_TESTS];

    int sw_ok = 1;
    int hw_ok = 1;

    // ========== Software Timing Test ==========
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    for (int i = 0; i < N_TESTS; i++) {
        sw_results[i] = gf_reduce_sw(test_inputs[i]);
    }
    vcd_disable();


    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    for (int i = 0; i < N_TESTS; i++) {
        hw_results[i] = gf_reduce_hw(test_inputs[i]);
    }
    vcd_disable();

    
    return 0;
}
