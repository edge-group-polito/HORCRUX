//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  ML-KEM Forward NTT — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the ML-KEM (Kyber) forward Number
//               Theoretic Transform.
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

static int compare_coeffs(const char* test_name, int idx, int32_t got, int32_t exp) {
    int match = (got == exp);

    return match;
}

static void kyber_ntt_sw_compute(kyber_poly *r) {
    kyber_ntt(r->coeffs);
}

static int kyber_ntt_sw_verify(const kyber_poly *r) {
    int ok = 1;
    for (int i = 0; i < KEM_N; i++) {
        ok &= compare_coeffs("K-NTT-SW", i, r->coeffs[i], kyber_tv_ntt_out.coeffs[i]);
    }
    return ok;
}

static void kyber_ntt_hw_compute(kyber_poly *r) {
    int k = 1;
    
    for (int len = 128; len >= 2; len >>= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int16_t zeta = zetas_KEM[k++];
            bfu_result_t out;
            for (int j = start; j < start + len; j++) {
                uint32_t rs1 = (int32_t)r->coeffs[j];
                uint32_t rs2 = (int32_t)r->coeffs[j+len];

                asm volatile ( 
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x0, %[rd], %[r1], t0, %[r3] \n" 
                    : [rd] "=r" (out.rd_lo) 
                    : [r1] "r" (rs1), 
                      [r2] "r" (rs2), 
                      [r3] "r" (zeta) 
                    : "cc", "t0"
                );
                r->coeffs[j] = (int16_t)(out.rd_lo & 0xFFFF);
                r->coeffs[j+len] = (int16_t)(out.rd_lo >> 16);
            }
        }
    }
}

static int kyber_ntt_hw_verify(const kyber_poly *r) {
    int ok = 1;
    for (int i = 0; i < 256; i++) {
        ok &= compare_coeffs("K-NTT-HW", i, r->coeffs[i], kyber_tv_ntt_out.coeffs[i]);
    }
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    kyber_poly r_sw, r_hw;


#if SW_TEST_ENABLED
    r_sw = kyber_tv_ntt_in;
    if (vcd_init() != 0)
    return 1;

    vcd_enable();

    kyber_ntt_sw_compute(&r_sw);
    vcd_disable();
    all_passed &= kyber_ntt_sw_verify(&r_sw);

#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    r_hw = kyber_tv_ntt_in;
    if (vcd_init() != 0)
    return 1;

    vcd_enable();

    kyber_ntt_hw_compute(&r_hw);

    vcd_disable();
    all_passed &= kyber_ntt_hw_verify(&r_hw);

    return 0;
}
