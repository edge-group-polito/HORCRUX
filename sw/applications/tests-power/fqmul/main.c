//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  ML-KEM fqmul — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the ML-KEM (Kyber) fqmul Montgomery
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
#include "kyber.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"


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

    int16_t results_sw[NUM_FQMUL_TESTS], results_hw[NUM_FQMUL_TESTS];


#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    fqmul_sw_compute(results_sw);
    vcd_disable();
#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    fqmul_hw_compute(results_hw);
    vcd_disable();

    return 0;
}
