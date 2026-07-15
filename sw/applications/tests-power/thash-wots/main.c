//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  SLH-DSA THASH + WOTS Chain — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the SLH-DSA THASH function combined with
//               the full WOTS+ chain (gen_chain).
//               Isolates the operation under a GPIO-triggered VCD dump window (see
//               sw/applications/tests-power/README.md) for post-synthesis SW-vs-HW power
//               comparison; SW_TEST_ENABLED selects the software reference or the
//               HORCRUX-accelerated path.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "core_v_mini_mcu.h"
#include "csr.h"

#include "test_params.h"
#include "test_vectors.h"
#include "thash_sw.h"
#include "wots_chain_sw.h"
#include "vcd_util.h"

//#define TEST_SW_ONLY 


#define SW_TEST_ENABLED 1


#include "thash_hw.h"
#include "wots_chain_hw.h"


/* ========================================================================
 * Utility functions
 * ======================================================================== */

static void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

static int compare_arrays(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}


/* ========================================================================
 * Test: Multiple WOTS chains (simulates sign/verify)
 * ======================================================================== */

static int test_multiple_chains(void) {
    spx_ctx ctx;
    uint8_t inputs[SPX_WOTS_LEN * SPX_N];
    uint8_t out_sw[SPX_WOTS_LEN * SPX_N];
    uint32_t starts[SPX_WOTS_LEN];
    uint32_t steps[SPX_WOTS_LEN];
    uint8_t addr[SPX_ADDR_BYTES];
    unsigned int cycles_sw = 0, total_thash_sw = 0;
#ifndef TEST_SW_ONLY
    uint8_t out_hw[SPX_WOTS_LEN * SPX_N];
    unsigned int cycles_hw = 0, total_thash_hw = 0;
#endif

    spx_ctx_init(&ctx, tv_pub_seed, tv_sk_seed);
    memcpy(addr, tv_addr_bytes, SPX_ADDR_BYTES);

    for (unsigned int i = 0; i < SPX_WOTS_LEN; i++) {
        for (unsigned int j = 0; j < SPX_N; j++) {
            inputs[i * SPX_N + j] = (uint8_t)((i * SPX_N + j) ^ 0xA5);
        }
        starts[i] = 0;
        steps[i] = (i % (SPX_WOTS_W - 1)) + 1;
    }

    if (vcd_init() != 0)
    return 1;

    vcd_enable();

    total_thash_sw = run_multiple_chains_sw(out_sw, inputs, SPX_WOTS_LEN,
                                            starts, steps, &ctx, addr);
    vcd_disable();

    if (vcd_init() != 0)
    return 1;

    vcd_enable();

    total_thash_hw = run_multiple_chains_hw(out_hw, inputs, SPX_WOTS_LEN,
                                            starts, steps, &ctx, addr);
    vcd_disable();

    if (compare_arrays(out_sw, out_hw, SPX_WOTS_LEN * SPX_N) != 0) {
        printf("multi_chain: FAIL\n");
        return 1;
    }


    return 0;
}

/* ========================================================================
 * Main
 * ======================================================================== */

int main(void) {
    int errors = 0;

    errors += test_multiple_chains();

    return errors;
}
