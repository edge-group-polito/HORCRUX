//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  Falcon Montgomery Multiplication — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the Falcon mq_montymul Montgomery
//               multiplication.
//               Isolates the operation under a GPIO-triggered VCD dump window (see
//               sw/applications/tests-power/README.md) for post-synthesis SW-vs-HW power
//               comparison; SW_TEST_ENABLED selects the software reference or the
//               HORCRUX-accelerated path.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "falcon.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

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

    return success;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    uint32_t results_sw[100], results_hw[100];


#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;
    
    vcd_enable();

    mq_montymul_sw_compute(results_sw);
    all_passed &= mq_montymul_sw_verify(results_sw);

    vcd_disable();

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();

    mq_montymul_hw_compute(results_hw);

    vcd_disable();
    all_passed &= mq_montymul_hw_verify(results_hw);

    return 0;
}
