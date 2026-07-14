///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// compare_u32 test (SW + HW) for HQC
//
// Tests the constant-time comparison function used in HQC
// Returns 1 if v1 == v2, 0 otherwise
//
// HW Operation:
//   OP_COMPARE_U32: rd = (rs1 == rs2) ? 1 : 0
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>

#include "core_v_mini_mcu.h"
#include "csr.h"

#define N_TESTS 20

// Pre-computed golden results: 1 if equal, 0 if different
static const uint32_t cmp_golden[N_TESTS] = {
    1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

// HW instruction macro for constant-time comparison
// OP_COMPARE_U32: rd = (rs1 == rs2) ? 1 : 0 (funct7=0x79, funct3=7, opcode=0x3b)
#define COMPARE_U32_HW(dest, rs1_val, rs2_val) \
    asm volatile ( \
        "addi t0, %[rs1], 0\n\t" \
        ".insn r 0x3b, 0x7, 0x79, %[rd], t0, %[rs2] \n" \
        : [rd] "=r" (dest) \
        : [rs1] "r" (rs1_val), [rs2] "r" (rs2_val) : "cc", "t0" \
    )

/**
 * @brief SW reference: Constant-time comparison of two integers v1 and v2
 *
 * Returns 1 if v1 is equal to v2 and 0 otherwise
 * https://gist.github.com/sneves/10845247
 *
 * @param[in] v1 First value
 * @param[in] v2 Second value
 * @return 1 if v1 == v2, 0 otherwise
 */
static inline uint32_t compare_u32_sw(const uint32_t v1, const uint32_t v2) {
    return 1 ^ (((v1 - v2) | (v2 - v1)) >> 31);
}

/**
 * @brief HW accelerated: Constant-time comparison of two integers
 *
 * Uses single custom instruction COMPARE_U32 that performs
 * constant-time comparison in one cycle.
 *
 * @param[in] v1 First value
 * @param[in] v2 Second value
 * @return 1 if v1 == v2, 0 otherwise
 */
static inline uint32_t compare_u32_hw(const uint32_t v1, const uint32_t v2) {
    uint32_t result;
    COMPARE_U32_HW(result, v1, v2);
    return result;
}

int main(void) {
    // Test inputs: pairs of values to compare
    // Mix of equal and unequal pairs, edge cases
    const uint32_t test_v1[N_TESTS] = {
        0x00000000,   // Equal: 0 == 0
        0x00000001,   // Equal: 1 == 1
        0xFFFFFFFF,   // Equal: max == max
        0x12345678,   // Equal: random
        0x00000000,   // Unequal: 0 vs 1
        0x00000001,   // Unequal: 1 vs 0
        0xFFFFFFFF,   // Unequal: max vs max-1
        0xFFFFFFFE,   // Unequal: max-1 vs max
        0x80000000,   // Edge: MSB set
        0x7FFFFFFF,   // Edge: max positive signed
        0x80000000,   // Unequal: sign bit diff
        0x7FFFFFFF,   // Unequal: sign bit diff
        0x00000001,   // Unequal: 1 vs 2
        0x00000002,   // Unequal: 2 vs 1
        0xDEADBEEF,   // Random unequal
        0xCAFEBABE,   // Random unequal
        0xAAAAAAAA,   // Pattern test
        0x55555555,   // Pattern test
        0x00000100,   // Small difference
        0x00000101,   // Small difference
        0x10000000,   // Large values
        0x10000001,   // Large values
        0xABCDEF01,   // Equal: complex pattern
        0x01234567    // Equal: another pattern
    };

    const uint32_t test_v2[N_TESTS] = {
        0x00000000,   // Equal: 0 == 0
        0x00000001,   // Equal: 1 == 1
        0xFFFFFFFF,   // Equal: max == max
        0x12345678,   // Equal: random
        0x00000001,   // Unequal: 0 vs 1
        0x00000000,   // Unequal: 1 vs 0
        0xFFFFFFFE,   // Unequal: max vs max-1
        0xFFFFFFFF,   // Unequal: max-1 vs max
        0x80000000,   // Edge: MSB set (equal)
        0x7FFFFFFF,   // Edge: max positive signed (equal)
        0x7FFFFFFF,   // Unequal: sign bit diff
        0x80000000,   // Unequal: sign bit diff
        0x00000002,   // Unequal: 1 vs 2
        0x00000001,   // Unequal: 2 vs 1
        0xBEEFDEAD,   // Random unequal
        0xBABECAFE,   // Random unequal
        0x55555555,   // Pattern test (unequal)
        0xAAAAAAAA,   // Pattern test (unequal)
        0x00000101,   // Small difference (unequal)
        0x00000100,   // Small difference (unequal)
        0x10000001,   // Large values (unequal)
        0x10000000,   // Large values (unequal)
        0xABCDEF01,   // Equal: complex pattern
        0x01234567    // Equal: another pattern
    };

    // Golden reference results (computed once by SW)
    uint32_t sw_results[N_TESTS];
    uint32_t hw_results[N_TESTS];

    uint32_t sw_cycles = 0;
    uint32_t hw_cycles = 0;
    int sw_ok = 1;
    int hw_ok = 1;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("compare_u32 Test (Constant-time equality comparison)\n");

    // Golden reference is pre-computed in cmp_golden[]

    // ========== Software Timing Test ==========
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        sw_results[i] = compare_u32_sw(test_v1[i], test_v2[i]);
    }
    CSR_READ(CSR_REG_MCYCLE, &sw_cycles);
    printf("[SW] Cycles for %d comparisons: %u\n", N_TESTS, sw_cycles);

    // ========== Hardware Accelerated Test ==========
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        hw_results[i] = compare_u32_hw(test_v1[i], test_v2[i]);
    }
    CSR_READ(CSR_REG_MCYCLE, &hw_cycles);
    printf("[HW] Cycles for %d comparisons: %u\n", N_TESTS, hw_cycles);

    // ========== Verify SW vs Golden ==========
    for (int i = 0; i < N_TESTS; i++) {
        if (sw_results[i] != cmp_golden[i]) {
            printf("[SW] Test %2d FAIL: v1=0x%08lX v2=0x%08lX golden=%lu sw=%lu\n",
                   i, test_v1[i], test_v2[i], cmp_golden[i], sw_results[i]);
            sw_ok = 0;
        }
    }
    if (sw_ok) {
        printf("[SW] All %d tests match golden.\n", N_TESTS);
    }

    // ========== Verify HW vs Golden ==========
    for (int i = 0; i < N_TESTS; i++) {
        if (hw_results[i] != cmp_golden[i]) {
            printf("[HW] Test %2d FAIL: v1=0x%08lX v2=0x%08lX golden=%lu hw=%lu\n",
                   i, test_v1[i], test_v2[i], cmp_golden[i], hw_results[i]);
            hw_ok = 0;
        }
    }
    if (hw_ok) {
        printf("[HW] All %d tests match golden.\n", N_TESTS);
    }

    // ========== Performance Summary ==========
    printf("\n======================================\n");
    printf("SW Cycles: %u | HW Cycles: %u\n", sw_cycles, hw_cycles);
    if (hw_cycles > 0) {
        printf("Speedup: %u.%02ux\n", sw_cycles / hw_cycles, 
               ((sw_cycles * 100) / hw_cycles) % 100);
    }

    if (sw_ok && hw_ok) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return 0;
}
