//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  Falcon Forward NTT — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the Falcon forward Number Theoretic
//               Transform.
//               Isolates the operation under a GPIO-triggered VCD dump window (see
//               sw/applications/tests-power/README.md) for post-synthesis SW-vs-HW power
//               comparison; SW_TEST_ENABLED selects the software reference or the
//               HORCRUX-accelerated path.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "falcon.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

#define BFNTTF(dest, a, b, c) \
    asm volatile ( \
        "addi t0, %[r2], 0\n" \
        ".insn r 0x6b, 0x06, 0x0, %[rd], %[r1], t0, %[r3] \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a), [r2] "r" (b), [r3] "r" (c) \
        : "cc", "t0" \
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

static void falcon_ntt_sw_compute(uint16_t *poly) {
    for (int i = 0; i < FALCON_N; i++) {
        poly[i] = falcon_ntt_input[i];
    }
    mq_NTT_sw(poly, 4);
}

static int falcon_ntt_sw_verify(const uint16_t *poly) {
    int ok = 1;
    for (int i = 0; i < FALCON_N; i++) {
        ok &= compare_coeffs("F-NTT-SW", i, (int32_t)poly[i], (int32_t)falcon_ntt_output[i]);
    }
    if (ok) printf("PASSED: Falcon NTT (Software)\n");
    return ok;
}

static void falcon_ntt_hw_compute(uint16_t *poly) {
    for (int i = 0; i < FALCON_N; i++) {
        poly[i] = falcon_ntt_input[i];
    }

    unsigned logn = 4;
    size_t n = (size_t)1 << logn;
    size_t t = n;

    for (size_t m = 1; m < n; m <<= 1) {
        size_t ht = t >> 1;
        for (size_t i = 0, j1 = 0; i < m; i++, j1 += t) {
            int16_t s = (int16_t)falcon_GMb_16[m + i];
            size_t j2 = j1 + ht;
            for (size_t j = j1; j < j2; j++) {
                uint32_t rs1 = (uint32_t)poly[j];
                uint32_t rs2 = (uint32_t)poly[j + ht];
                uint32_t out;

                BFNTTF(out, rs1, rs2, (int32_t)s);

                poly[j] = (uint16_t)(out & 0xFFFF);
                poly[j + ht] = (uint16_t)(out >> 16);
            }
        }
        t = ht;
    }
}

static int falcon_ntt_hw_verify(const uint16_t *poly) {
    int ok = 1;
    for (int i = 0; i < FALCON_N; i++) {
        ok &= compare_coeffs("F-NTT-HW", i, (int32_t)poly[i], (int32_t)falcon_ntt_output[i]);
    }
    if (ok) printf("PASSED: Falcon NTT (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;

    uint16_t poly_sw[FALCON_N], poly_hw[FALCON_N];


#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    falcon_ntt_sw_compute(poly_sw);
    vcd_disable();
    
#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    falcon_ntt_hw_compute(poly_hw);
    vcd_disable();


    return 0;
}
