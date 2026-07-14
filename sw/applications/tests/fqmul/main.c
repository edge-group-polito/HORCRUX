/**
 * @file main.c
 * @brief Kyber fqmul (Montgomery Reduction) Test - Software vs Hardware
 * 
 * Tests the Montgomery reduction multiplication for Kyber
 * key encapsulation mechanism using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "kyber.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

#define NUM_FQMUL_TESTS 10

static const fqmul_test_vector fqmul_tests[] = {
    {-758, -2, -129},
    {-758,  0,    0},
    {-758, -1, 1600},
    {-758,  1, -1600},
    {-758,  2,  129},
    {-758, -3, 1471},
    {-758,  3, -1471},
    {-758,  1, -1600},
    {-758,  2,  129},
    {-758, -3, 1471}
};

static void fqmul_sw_compute(int16_t *results) {
    for (int i = 0; i < NUM_FQMUL_TESTS; i++) {
        results[i] = fqmul(fqmul_tests[i].a, fqmul_tests[i].b);
    }
}

static void fqmul_hw_compute(int16_t *results) {

    for (int i = 0; i < NUM_FQMUL_TESTS; i++) {
        int32_t a = (int32_t)fqmul_tests[i].a;
        int32_t b = (int32_t)fqmul_tests[i].b;
        bfu_result_t out;
        asm volatile ( 
            "addi t0, %[r1], 0\n "
            ".insn r 0x3b, 0x7, 0x11, %[rd], t0, %[r2] \n" 
            : [rd] "=r" (out.rd_lo) 
            : [r1] "r" (a), [r2] "r" (b)
            : "cc", "t0"
        );
        results[i] = (int16_t)out.rd_lo;
    }
}

static int fqmul_verify(const int16_t *results, const char *name) {
    int ok = 1;
    for (int i = 0; i < NUM_FQMUL_TESTS; i++) {
        if (results[i] != fqmul_tests[i].expected) {
            printf("  [FAIL] Test %d: fqmul(%d, %d)\n", i, fqmul_tests[i].a, fqmul_tests[i].b);
            printf("         Expected: %d, Got: %d\n", fqmul_tests[i].expected, results[i]);
            ok = 0;
        }
    }
    if (ok) printf("PASSED: Kyber fqmul (%s) (%d cases)\n", name, NUM_FQMUL_TESTS);
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    int16_t results_sw[NUM_FQMUL_TESTS], results_hw[NUM_FQMUL_TESTS];

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("    KYBER FQMUL TEST                 \n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    fqmul_sw_compute(results_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= fqmul_verify(results_sw, "Software");
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    fqmul_hw_compute(results_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= fqmul_verify(results_hw, "Hardware");
    printf("Hardware Cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
    if (cycles_sw > 0 && cycles_hw > 0) {
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
