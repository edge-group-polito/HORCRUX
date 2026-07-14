/**
 * @file main.c
 * @brief Dilithium reduce32 Test - Software vs Hardware
 * 
 * Tests the modular reduction operations for Dilithium
 * digital signature algorithm using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "dilithium.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

static int compare_scalar(const char* test_name, int index, int32_t input, 
                          int32_t got, int32_t expected) {
    if (got != expected) {
        printf("FAILED: %s [%d] for input %d: Got %d, Expected %d\n", 
               test_name, index, input, got, expected);
        return 0;
    }
    return 1;
}

static void dilithium_reduce32_sw_compute(int32_t *results) {
    for (int i = 0; i < 20; i++) {
        results[i] = reduce32(dsa_tv_scalars[i].input);
    }
}

static int dilithium_reduce32_sw_verify(const int32_t *results) {
    int ok = 1;
    for (int i = 0; i < 20; i++) {
        ok &= compare_scalar("D-REDUCE32-SW", i, dsa_tv_scalars[i].input, 
                             results[i], dsa_tv_scalars[i].expected_reduce);
    }
    if (ok) printf("PASSED: Dilithium reduce32 (Software)\n");
    return ok;
}

static void dilithium_reduce32_hw_compute(int32_t *results) {
    for (int i = 0; i < 20; i++) {
        bfu_result_t res;
        asm volatile ( 
            "addi t0, %[r1], 0\n"
            ".insn r 0x3b, 0x7, 0x1A, %[rd], t0, x0 \n" 
            : [rd] "=r" (res.rd_lo) 
            : [r1] "r" ((int32_t)dsa_tv_scalars[i].input)
            : "cc", "t0"
        );
        results[i] = (int32_t)res.rd_lo;
    }
}

static int dilithium_reduce32_hw_verify(const int32_t *results) {
    int ok = 1;
    for (int i = 0; i < 20; i++) {
        int32_t expected = dsa_tv_scalars[i].expected_reduce;
        if (results[i] != expected) {
            printf("FAILED: D-RED32-HW [%d] Input %d: Got %d, Expected %d\n", 
                   i, dsa_tv_scalars[i].input, results[i], expected);
            ok = 0;
        }
    }
    if (ok) printf("PASSED: Dilithium reduce32 (Hardware)\n");
    return ok;
}


int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    int32_t results_reduce32_sw[20];
    int32_t results_reduce32_hw[20];

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   DILITHIUM REDUCE32 TEST     \n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    dilithium_reduce32_sw_compute(results_reduce32_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= dilithium_reduce32_sw_verify(results_reduce32_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TESTS SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    dilithium_reduce32_hw_compute(results_reduce32_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= dilithium_reduce32_hw_verify(results_reduce32_hw);
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
