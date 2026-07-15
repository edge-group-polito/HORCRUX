//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  SHA3-256 Single-Block Absorption — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for SHA3-256 single-block absorption and
//               squeeze.
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
    uint8_t sw_hash[32];
    uint8_t hw_hash[32];
    const uint8_t msg[] = "abc";
    const uint8_t expected[32] = {
        0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2,
        0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd,
        0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b,
        0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32
    };

    int all_passed = 1;


#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    sha3_256(sw_hash, msg, sizeof(msg) - 1);
    vcd_disable();
#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    sha3_256_hw(hw_hash, msg, sizeof(msg) - 1);
    vcd_disable();

    return 0;
}
