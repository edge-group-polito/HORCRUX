//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  SLH-DSA THASH2 — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the SLH-DSA tweakable hash on a 2-block
//               input (THASH2).
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
#include "thash_sw.h"
#include "vcd_util.h"

/* Uncomment to test software only (useful for generating golden outputs) */
/* #define TEST_SW_ONLY */


#define SW_TEST_ENABLED 1


#include "thash_hw.h"

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
 * Test 3: Simulated compute_root iteration
 * ======================================================================== */

static int test_merkle_tree_level(void) {
    spx_ctx ctx;
    uint8_t nodes[4][SPX_N];  /* 4 leaf nodes */
    uint8_t parents[2][SPX_N]; /* 2 parent nodes */
    uint8_t root[SPX_N];
    uint8_t addr[SPX_ADDR_BYTES];
    unsigned int cycles_sw = 0;
#ifndef TEST_SW_ONLY
    uint8_t root_hw[SPX_N];
    unsigned int cycles_hw = 0;
#endif

    spx_ctx_init(&ctx, tv_pub_seed, tv_sk_seed);
    memcpy(addr, tv_addr_tree, SPX_ADDR_BYTES);
    
    /* Initialize 4 leaf nodes with deterministic data */
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < SPX_N; j++) {
            nodes[i][j] = (uint8_t)(i * 16 + j);
        }
    }

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    
    set_tree_height(addr, 0);
    set_tree_index(addr, 0);
    thash_sw(parents[0], (uint8_t*)&nodes[0], 2, &ctx, addr);
    
    set_tree_index(addr, 1);
    thash_sw(parents[1], (uint8_t*)&nodes[2], 2, &ctx, addr);
    
    set_tree_height(addr, 1);
    set_tree_index(addr, 0);
    thash_sw(root, (uint8_t*)parents, 2, &ctx, addr);
    
    vcd_disable();

#ifndef TEST_SW_ONLY
    /* HW: Same computation */
    memcpy(addr, tv_addr_tree, SPX_ADDR_BYTES);

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    
    set_tree_height(addr, 0);
    set_tree_index(addr, 0);
    thash_hw(parents[0], (uint8_t*)&nodes[0], 2, &ctx, addr);
    
    set_tree_index(addr, 1);
    thash_hw(parents[1], (uint8_t*)&nodes[2], 2, &ctx, addr);
    
    set_tree_height(addr, 1);
    set_tree_index(addr, 0);
    thash_hw(root_hw, (uint8_t*)parents, 2, &ctx, addr);
    
    vcd_disable();


    if (compare_arrays(root, root_hw, SPX_N) != 0) {
        printf("Test 3: FAIL (root mismatch)\n");
        return 1;
    }
    
    //printf("Test 3: PASS\n");
#else
    printf("Test 3: SW only\n");
#endif

    return 0;
}

/* ========================================================================
 * Main
 * ======================================================================== */

int main(void) {
    int failures = 0;

    failures += test_merkle_tree_level();

    return failures;
}
