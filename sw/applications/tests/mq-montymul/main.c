/**
 * @file main.c
 * @brief Falcon Montgomery Multiplication Test - Software vs Hardware
 * 
 * Tests the Montgomery multiplication (mq_montymul) operation for Falcon
 * digital signature algorithm using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "falcon.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define NTEST 10

#define DEBUG 0

#define MQMULF(dest, a, b) \
    asm volatile ( \
        "addi t0, %[r1], 0\n" \
        ".insn r 0x3b, 0x7, 0x10, %[rd], t0, %[r2] \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a), [r2] "r" (b) \
    );

static int compare_coeffs(const char* test_name, int idx, int32_t got, int32_t exp) {
    int match = (got == exp);
#if DEBUG
    printf("%s [%3d] -> Got: %8d | Exp: %8d %s\n", 
           test_name, idx, got, exp, match ? " " : "[MISMATCH]");
#endif
    if (!match && !DEBUG) {
        printf("[%s ERROR] Mismatch @ idx %3d: Got %8d, Expected %8d\n", 
               test_name, idx, got, exp);
    }
    return match;
}

static void mq_montymul_sw_compute(uint32_t *results) {
    for (int i = 0; i < NTEST; i++) {
        results[i] = mq_montymul_sw(falcon_source1[i], falcon_source0[i]);
    }
}

static int mq_montymul_sw_verify(const uint32_t *results) {
    int ok = 1;
    for (int i = 0; i < NTEST; i++) {
        ok &= compare_coeffs("F-MUL-SW", i, (int32_t)results[i], (int32_t)falcon_golden[i]);
    }
    if (ok) printf("PASSED: Falcon mq_montymul (Software)\n");
    return ok;
}

static void mq_montymul_hw_compute(uint32_t *results) {
    bfu_result_t out;
    for (int i = 0; i < NTEST; i++) {
        MQMULF(out.rd_lo, falcon_source1[i], falcon_source0[i]);
        results[i] = out.rd_lo;
    }
}

static int mq_montymul_hw_verify(const uint32_t *results) {
    int success = 1;
    for (int i = 0; i < NTEST; i++) {
        success &= compare_coeffs("F-MUL-HW", i, (int32_t)results[i], (int32_t)falcon_golden[i]);
    }
    if (success) printf("PASSED: Falcon mq_montymul (Hardware)\n");
    return success;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    uint32_t results_sw[100], results_hw[100];

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("  FALCON MQ_MONTYMUL TEST            \n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    mq_montymul_sw_compute(results_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= mq_montymul_sw_verify(results_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    mq_montymul_hw_compute(results_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= mq_montymul_hw_verify(results_hw);
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
