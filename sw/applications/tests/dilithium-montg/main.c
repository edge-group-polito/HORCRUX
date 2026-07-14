///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Generic Montgomery reduction tests for ML-DSA (Dilithium) and ML-KEM (Kyber)
//
///////////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif



#define N_TESTS  20
// ---------------------- ML-DSA (Dilithium) test ----------------------
#define DSA_Q     8380417
#define DSA_QINV  58728449  // matches Dilithium reference


#define MONTG_DIL(dest, a) \
    asm volatile ( \
        "addi t0, %[r1], 0\n" \
        ".insn r 0x3b, 0x7, 0x12, %[rd], t0, %[q] \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a), [q] "r" (1) \
    );

static inline int32_t montgomery_reduce_dsa(int32_t a) {
    int32_t t;
    t = (int32_t)((int64_t)(int32_t)a * (int32_t)DSA_QINV);
    t = (int32_t)(((int64_t)a - (int64_t)t * DSA_Q) >> 32);
    return t;
}



int main(void) {

    // ---------------- ML-DSA (Dilithium) ----------------
    // Inputs are 32-bit: MQMULD computes (rs1 * rs2 * R^-1) mod Q, tested with rs2=1
    const int32_t dsa_inputs[20] = {
        123456789, 0, 1, -1, -123456789, 2147483647, -2147483648, 65535,
        -65536, 8380417, 16760834, -8380417, 2147483647, -2147483648,
        9999999, -9999999, 65535, -65535, 12345, -54321
    };

    const int32_t dsa_golden[20] = {
        -2438631, 0, -114592, 114592, 2438631, -4075616, 4190208, -933088,
        1047680, 0, 0, 0, -4075616, 4190208,
        1574338, -1574338, -933088, 933088, 1652233, -1897799
    };
    

    int32_t got_sw[N_TESTS] = {};
    int32_t got_hw[N_TESTS] = {};
    int dsa_ok_sw = 1;
    int dsa_ok_hw = 1;
    int all_passed = 1;

    unsigned cycles_sw = 0;
    unsigned cycles_hw = 0;


    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("Dilithium Montgomery Reduction Tests.\n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        got_sw[i] = montgomery_reduce_dsa(dsa_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("Software Cycles: %u\n", cycles_sw);

    for (int i = 0; i < N_TESTS; i++) {
        if (got_sw[i] != dsa_golden[i]) {
            printf("[DSA][SW] Test %2d FAIL: a=%d  exp=%d  got=%d\n", i, dsa_inputs[i], dsa_golden[i], got_sw[i]);
            dsa_ok_sw = 0;
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        MONTG_DIL(got_hw[i], dsa_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

    for (int i = 0; i < N_TESTS; i++) {
        if (got_hw[i] != dsa_golden[i]) {
            printf("[DSA][HW] Test %2d FAIL: a=%d  exp=%d  got=%d\n", i, dsa_inputs[i], dsa_golden[i], got_hw[i]);
            dsa_ok_hw = 0;
        }
    }

#if SW_TEST_ENABLED
    if (cycles_sw > 0 && cycles_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    all_passed = dsa_ok_sw && dsa_ok_hw;
    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }


    return 0;
}
