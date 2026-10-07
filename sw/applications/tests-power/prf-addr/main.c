//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 EDGE Group, @VLSI Lab - Politecnico di Torino
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  SLH-DSA PRF Address — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for SLH-DSA PRF secret-key derivation from
//               the address structure.
//               Isolates the operation under a GPIO-triggered VCD dump window (see
//               sw/applications/tests-power/README.md) for post-synthesis SW-vs-HW power
//               comparison; SW_TEST_ENABLED selects the software reference or the
//               HORCRUX-accelerated path.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core_v_mini_mcu.h"
#include "csr.h"
#include "test_params.h"
#include "test_vectors.h"
#include "prf_sw.h"
#include "prf_hw.h"
#include "vcd_util.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

/* Test result tracking */
static int total_tests = 0;
static int passed_tests = 0;

/* SW-vs-HW comparison outcome (0 = match, 1 = mismatch). volatile so the
 * store survives optimization and can be read from a waveform or debugger. */
volatile uint32_t prf_mismatch = 0;

/* Performance tracking */
static uint32_t sw_total_cycles = 0;
static uint32_t hw_total_cycles = 0;

/* Utility functions */
static int compare_arrays(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

/* ========================================================================
 * Test 1: Basic PRF Test (WOTS address)
 * ======================================================================== */
static void test_prf_basic(void) {
    spx_ctx ctx;
    uint8_t out_sw[SPX_N], out_hw[SPX_N];
    uint32_t cycles_sw = 0, cycles_hw = 0;


    memcpy(ctx.pub_seed, tv_pub_seed, SPX_N);
    memcpy(ctx.sk_seed, tv_sk_seed, SPX_N);

#if SW_TEST_ENABLED

    if (vcd_init() != 0)
    return;

    vcd_enable();

    prf_addr_sw(out_sw, &ctx, tv_addr_wots);

    vcd_disable();

#endif

    if (vcd_init() != 0)
    return;

    vcd_enable();

    prf_addr_hw(out_hw, &ctx, tv_addr_wots);

    vcd_disable();

    total_tests++;

    /* No printf: a UART write here stalls the post-synthesis gate-level
     * simulation long past the measured region, so the testbench never reaches
     * exit_valid. Functional checking belongs in sw/applications/tests/prf-addr. */
#if SW_TEST_ENABLED
    prf_mismatch = (compare_arrays(out_sw, out_hw, SPX_N) != 0) ? 1u : 0u;
    if (prf_mismatch == 0u) passed_tests++;
#else
    passed_tests++;
#endif
}

/* ========================================================================
 * Main
 * ======================================================================== */
int main(void)
{

    test_prf_basic();

    return 0;
}
