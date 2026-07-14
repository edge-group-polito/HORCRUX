///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Generic Montgomery reduction tests for ML-DSA (Dilithium) and ML-KEM (Kyber)
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>


#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif


#define N_TESTS  20

#define FALCON_Q     12289
#define FALCON_QINV  12287    // -q^{-1} mod 2^16

#define MONTG_FALCON(dest, a) \
    asm volatile ( \
        "addi t0, %[r1], 0\n" \
        ".insn r 0x3b, 0x7, 0x2, %[rd], t0, x0 \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a) \
    );

static inline int16_t montgomery_reduce_falcon(int32_t a) {
    int16_t t;
    t = (int16_t)a * FALCON_QINV;
    t = (a - (int32_t)t * FALCON_Q) >> 16;
    return t;
}


int main(void) {

    // ---------------- Falcon  ----------------
    const int32_t fal_inputs[20] = {
        0, 1, -1, 1234567, -1234567, 2147483647, -2147483648, 0x0000FFFF, 0xFFFF0000, 12289, 2*12289, -12289, 2147483647, -2147483648, 9999999, -9999999, 65535, -65535, 12345, -54321
    };
    const int16_t fal_golden[20] = { 
        0, -2304, 2303, -5813, 5812, -30465, -32768, 2304, -1, 0, 0, -1, -30465, -32768, -2608, 2607, 2304, -2305, -6134, 4406
    };


    int16_t got_sw[N_TESTS] = {0};
    int16_t got_hw[N_TESTS] = {0};
    int fal_ok_sw = 1;
    int fal_ok_hw = 1;
    int all_passed = 1;
    unsigned cycles_sw = 0;
    unsigned cycles_hw = 0;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        got_sw[i] = montgomery_reduce_falcon(fal_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("Software Cycles: %u\n", cycles_sw);

    for (int i = 0; i < N_TESTS; i++) {
        if (got_sw[i] != fal_golden[i]) {
            printf("[FALCON][SW] Test %2d FAIL: a=%d  exp=%d  got=%d\n", i, fal_inputs[i], fal_golden[i], got_sw[i]);
            fal_ok_sw = 0;
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        MONTG_FALCON(got_hw[i], fal_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

    for (int i = 0; i < N_TESTS; i++) {
        if (got_hw[i] != fal_golden[i]) {
            printf("[FALCON][HW] Test %2d FAIL: a=%d  exp=%d  got=%d\n", i, fal_inputs[i], fal_golden[i], got_hw[i]);
            fal_ok_hw = 0;
        }
    }

#if SW_TEST_ENABLED
    if (cycles_sw > 0 && cycles_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    all_passed = fal_ok_sw && fal_ok_hw;
    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }


    return 0;
}
