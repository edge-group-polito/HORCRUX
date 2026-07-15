//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  ML-DSA Polyvec Inverse NTT — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the ML-DSA inverse NTT over a K=4
//               polynomial vector.
//               Isolates the operation under a GPIO-triggered VCD dump window (see
//               sw/applications/tests-power/README.md) for post-synthesis SW-vs-HW power
//               comparison; SW_TEST_ENABLED selects the software reference or the
//               HORCRUX-accelerated path.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dilithium.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

/*
 * Deterministic polyvec initialisation.
 * Produces coefficients in [-DSA_Q, DSA_Q) using a one-step LCG per entry.
 */
static void init_fixed_polyvec(dsa_polyvec *v, int32_t salt)
{
    for (int i = 0; i < DSA_K; i++) {
        for (int j = 0; j < DSA_N; j++) {
            uint32_t seed = (uint32_t)(i * 251 + j * 65537 + salt * 1000003);
            seed = seed * 1664525u + 1013904223u; /* LCG step */
            int32_t x = (int32_t)(seed % (uint32_t)(2 * DSA_Q)) - DSA_Q;
            v->vec[i].coeffs[j] = x;
        }
    }
}

static int compare_polyvec(const char *test_name,
                            const dsa_polyvec *got,
                            const dsa_polyvec *exp)
{
    int ok = 1;
    for (int i = 0; i < DSA_K; i++) {
        for (int j = 0; j < DSA_N; j++) {
            int32_t g = got->vec[i].coeffs[j];
            int32_t e = exp->vec[i].coeffs[j];
            if (g != e) {
                ok = 0;
#if DEBUG
                printf("[%s] Mismatch vec=%d idx=%d got=%d exp=%d\n",
                       test_name, i, j, g, e);
#else
                return 0;
#endif
            }
        }
    }
    return ok;
}

int main(void)
{
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    int all_passed = 1;

    dsa_polyvec in_a;
    dsa_polyvec ntt_in;                  /* NTT-domain input for INTT test */
    dsa_polyvec sw_out, hw_out, golden;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    /* Build time-domain input, then NTT it to get a valid NTT-domain vector */
    init_fixed_polyvec(&in_a, 1);
    ntt_in = in_a;
    dilithium_polyvec_ntt_sw(&ntt_in);

#if SW_TEST_ENABLED
    sw_out = ntt_in;
    if (vcd_init() != 0)
        return 1;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    vcd_enable();
    dilithium_polyvec_invntt_sw(&sw_out);
    vcd_disable();
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    golden = sw_out; /* SW output becomes the golden reference */

    if (!compare_polyvec("polyvec_invntt SW->golden", &sw_out, &golden)) {
        all_passed = 0;
    }
#else
    golden = ntt_in; /* No SW golden; skip HW verification */
#endif

    hw_out = ntt_in;
    if (vcd_init() != 0)
        return 1;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    vcd_enable();
    dilithium_polyvec_invntt_hw(&hw_out);
    vcd_disable();
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);

#if SW_TEST_ENABLED
    if (!compare_polyvec("polyvec_invntt HW->golden", &hw_out, &golden)) {
        all_passed = 0;
    }
#endif

    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
