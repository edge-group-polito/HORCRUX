//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  WOTS+ Chain Lengths — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for SLH-DSA WOTS+ base-w chain-length and
//               checksum computation.
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

#include "test_vectors.h"
#include "chain_lengths_sw.h"
#include "chain_lengths_hw.h"
#include "vcd_util.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif


#define NUM_RUNS 10


/**
 * Test software implementation for a given variant
 */
static int test_sw_variant(const test_vector_t *tv) {
    uint8_t result[67];  // Max length is 67 for 256f

    chain_lengths_sw(result, tv->len1, tv->len2, tv->msg);

    return 0; // Skip comparison
}

/**
 * Test hardware implementation for a given variant
 */
static int test_hw_variant(const test_vector_t *tv) {
    uint8_t result[67];  // Max length is 67 for 256f

    const uint32_t *msg32 = (const uint32_t *)tv->msg;
    
    chain_lengths_hw_128f(result, msg32);

    return 0; // Skip comparison
}

int main(void) {
    int total_errors = 0;

    const test_vector_t *tv = &test_vector_128f_simple;

    if (vcd_init() != 0)
    return 1;
    vcd_enable();
    for (uint32_t i = 0; i < NUM_RUNS; i++) {
        total_errors += test_sw_variant(tv);
    }
    vcd_disable();


    if (vcd_init() != 0)
    return 1;
    vcd_enable();
    for (uint32_t i = 0; i < NUM_RUNS; i++) {
        total_errors += test_hw_variant(tv);
    }
    vcd_disable();

    
    return total_errors;
}
