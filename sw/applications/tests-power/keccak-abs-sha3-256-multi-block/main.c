//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  SHA3-256 Multi-Block Absorption — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for SHA3-256 multi-block absorption.
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

#include "fips202.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

int main(void) {
    uint8_t msg[200];
    uint8_t sw_hash[32];
    uint8_t hw_hash[32];

    int all_passed = 1;

    for (int i = 0; i < 200; i++) {
        msg[i] = (uint8_t)(i & 0xFF);
    }



#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    sha3_256(sw_hash, msg, sizeof(msg));
    vcd_disable();
#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    sha3_256_hw(hw_hash, msg, sizeof(msg));
    vcd_disable();


    return 0;
}
