//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  SLH-DSA THASH1 — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the SLH-DSA tweakable hash on a 1-block
//               input (THASH1).
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
#include "thash_sw.h"
#include "thash_hw.h"
#include "vcd_util.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

/* Test result tracking */
static int total_tests = 0;
static int passed_tests = 0;

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
 * Test 1: thash with 1 block input (THASH1)
 * ======================================================================== */
static void test_thash1_basic(void) {
    spx_ctx ctx;
    uint8_t out_sw[SPX_N];
    uint8_t out_hw[SPX_N];
    uint8_t addr[SPX_ADDR_BYTES];
    uint32_t cycles_sw = 0, cycles_hw = 0;


    spx_ctx_init(&ctx, tv_pub_seed, tv_sk_seed);
    memcpy(addr, tv_addr_bytes, SPX_ADDR_BYTES);

#if SW_TEST_ENABLED
    /* Run SW reference */
    if (vcd_init() != 0)
    return;

    vcd_enable();
    thash_sw(out_sw, tv_thash1_input, 1, &ctx, addr);
    vcd_disable();
#endif

    /* Run HW implementation */
    if (vcd_init() != 0)
    return;

    vcd_enable();

    thash_hw(out_hw, tv_thash1_input, 1, &ctx, addr);

    vcd_disable();

    total_tests++;
#if SW_TEST_ENABLED
    if (compare_arrays(out_sw, out_hw, SPX_N) != 0) {
        printf("  Result: FAIL (SW vs HW mismatch)\n");
    }
#else
    passed_tests++;  // Just verify HW completes
    printf("  Result: PASS (HW only)\n");
#endif
}


/* ========================================================================
 * Test 4: Full WOTS chain (15 steps - SPX_WOTS_W-1)
 * ======================================================================== */
static void test_thash1_full_chain(void) {
    spx_ctx ctx;
    uint8_t state_sw[SPX_N], state_hw[SPX_N];
    uint8_t addr[SPX_ADDR_BYTES];
    uint32_t cycles_sw = 0, cycles_hw = 0;
    const unsigned int chain_len = SPX_WOTS_W - 1;  /* 15 steps */


    spx_ctx_init(&ctx, tv_pub_seed, tv_sk_seed);

#if SW_TEST_ENABLED
    memcpy(addr, tv_addr_bytes, SPX_ADDR_BYTES);
    memcpy(state_sw, tv_thash1_input, SPX_N);

    if (vcd_init() != 0)
    return;

    vcd_enable();

    for (unsigned int i = 0; i < chain_len; i++) {
        addr[SPX_OFFSET_HASH_ADDR] = (uint8_t)i;
        thash_sw(state_sw, state_sw, 1, &ctx, addr);
    }
    vcd_disable();
#endif

    memcpy(addr, tv_addr_bytes, SPX_ADDR_BYTES);
    memcpy(state_hw, tv_thash1_input, SPX_N);

    if (vcd_init() != 0)
    return;

    vcd_enable();

    for (unsigned int i = 0; i < chain_len; i++) {
        addr[SPX_OFFSET_HASH_ADDR] = (uint8_t)i;
        thash_hw(state_hw, state_hw, 1, &ctx, addr);
    }
    vcd_disable();

    total_tests++;
#if SW_TEST_ENABLED
    if (compare_arrays(state_sw, state_hw, SPX_N) != 0) {
        printf("  Result: FAIL (chain mismatch)\n");
    }
#endif
}

/* ========================================================================
 * Main
 * ======================================================================== */
int main(void)
{
    // Initialize cycle counter

    test_thash1_basic();

    return 0;
}
