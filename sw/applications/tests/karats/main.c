/**
 * @file main.c
 * @brief HQC Karatsuba 64x64 Multiplication Test - Software vs Hardware
 * 
 * Tests the Karatsuba multiplication for HQC (Hamming Quasi-Cyclic)
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

static void karats_sw_compute(uint64_t results[][2]) {
    for (int i = 0; i < NUM_HQC_TESTS; i++) {
        gf2_mul64_sw(results[i], hqc_a[i], hqc_b[i]);
    }
}

static int karats_sw_verify(const uint64_t results[][2]) {
    int ok = 1;
    for (int i = 0; i < NUM_HQC_TESTS; i++) {
        if (results[i][0] != hqc_out_ref[i].val[0] || results[i][1] != hqc_out_ref[i].val[1]) {
            printf("[KARATS-SW ERROR] Test %d: Mismatch!\n", i);
            ok = 0;
        }
    }
    if (ok) printf("PASSED: HQC Karatsuba 64x64 (Software)\n");
    return ok;
}

typedef struct {
    uint64_t c0;
    uint64_t c1;
} karats_hw_result_t;

static void karats_hw_compute(karats_hw_result_t *results) {
    for (int i = 0; i < NUM_HQC_TESTS; i++) {
        bfu_result_t out1, out2, out3;
        asm volatile ( 
            "addi t0, %[r1], 0\n "
            ".insn r 0x3b, 0x7, 0x13, %[rd1], t0,  %[r2] \n" 
            ".insn r 0x3b, 0x7, 0x14, %[rd2], %[r1b], %[r2b] \n" 
            ".insn r 0x3b, 0x7, 0x15, %[rd3], t0,  %[r2] \n" 
            ".insn r 0x3b, 0x7, 0x19, %[rd4], x0, x0 \n" 
            : [rd1] "=r" (out1.rd_lo), [rd2] "=r" (out2.rd_lo), 
              [rd3] "=r" (out3.rd_lo), [rd4] "=r" (out3.rd_hi)
            : [r1]  "r" ((uint32_t)hqc_a[i]), 
              [r2]  "r" ((uint32_t)hqc_b[i]), 
              [r1b] "r" ((uint32_t)(hqc_a[i] >> 32)), 
              [r2b] "r" ((uint32_t)(hqc_b[i] >> 32))
            : "cc", "t0"
        );

        results[i].c0 = out1.rd_lo | ((uint64_t)out3.rd_lo << 32);
        results[i].c1 = out3.rd_hi | ((uint64_t)out2.rd_lo << 32);
    }
}

static int karats_hw_verify(const karats_hw_result_t *results) {
    int ok = 1;
    for (int i = 0; i < NUM_HQC_TESTS; i++) {
        uint64_t exp_c0 = hqc_out_ref[i].val[0];
        uint64_t exp_c1 = hqc_out_ref[i].val[1];
        int match = (results[i].c0 == exp_c0 && results[i].c1 == exp_c1);
        
        if (!match) {
#if DEBUG
            printf("[KARATS-HW ERROR] Test %d: Mismatch!\n", i);
#endif
            ok = 0;
        }
    }
    if (ok) printf("PASSED: HQC Karatsuba 64x64 (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    uint64_t results_sw[NUM_HQC_TESTS][2];
    karats_hw_result_t results_hw[NUM_HQC_TESTS];

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   HQC KARATSUBA 64x64 TEST          \n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    karats_sw_compute(results_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= karats_sw_verify(results_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    karats_hw_compute(results_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= karats_hw_verify(results_hw);
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
