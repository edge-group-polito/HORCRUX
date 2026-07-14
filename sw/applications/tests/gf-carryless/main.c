/**
 * @file main.c
 * @brief HQC GF(2) 8-bit Carry-less Multiplication Test - Software vs Hardware
 * 
 * Tests the carry-less multiplication for HQC (Hamming Quasi-Cyclic)
 * code-based encryption using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hqc.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

static void gf_carryless_sw_compute(uint8_t results[][2]) {
    for (int i = 0; i < NTESTS_8BIT; i++) {
        gf_carryless_mul_sw(results[i], hqc_a_8bit[i], hqc_b_8bit[i]);
    }
}

static int gf_carryless_sw_verify(const uint8_t results[][2]) {
    int ok = 1;
    for (int i = 0; i < NTESTS_8BIT; i++) {
        if (results[i][0] != hqc_c0_expected[i] || results[i][1] != hqc_c1_expected[i]) {
#if DEBUG
            printf("[GF-SW ERROR] Test %d: Got [%02x, %02x], Exp [%02x, %02x]\n",
                   i, results[i][0], results[i][1], hqc_c0_expected[i], hqc_c1_expected[i]);
#endif
            ok = 0;
        }
    }
    if (ok) printf("PASSED: HQC 8-bit Carry-less (Software)\n");
    return ok;
}

static void gf_carryless_hw_compute(uint32_t *results) {
    for (int i = 0; i < NTESTS_8BIT; i++) {
        bfu_result_t out;
        asm volatile ( 
            "addi t0, %[r1], 0\n "
            ".insn r 0x3b, 0x7, 0x16, %[rd], t0, %[r2] \n" 
            : [rd] "=r" (out.rd_lo) 
            : [r1] "r" (hqc_a_8bit[i]), [r2] "r" (hqc_b_8bit[i])
            : "cc", "t0"
        );
        results[i] = out.rd_lo;
    }
}

static int gf_carryless_hw_verify(const uint32_t *results) {
    int ok = 1;
    for (int i = 0; i < NTESTS_8BIT; i++) {
        if ((uint8_t)results[i] != hqc_c0_expected[i] || 
            (uint8_t)(results[i] >> 8) != hqc_c1_expected[i]) {
#if DEBUG
            printf("[GF-HW ERROR] Test %d: Got [%02x, %02x], Exp [%02x, %02x]\n",
                   i, (uint8_t)results[i], (uint8_t)(results[i] >> 8),
                   hqc_c0_expected[i], hqc_c1_expected[i]);
#endif
            ok = 0;
        }
    }
    if (ok) printf("PASSED: HQC 8-bit Carry-less (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    uint8_t results_sw[NTESTS_8BIT][2];
    uint32_t results_hw[NTESTS_8BIT];

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("  HQC GF(2) CARRY-LESS MUL TEST      \n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gf_carryless_sw_compute(results_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= gf_carryless_sw_verify(results_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gf_carryless_hw_compute(results_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= gf_carryless_hw_verify(results_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
    if (cycles_sw > 0) {
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return EXIT_SUCCESS;
}
