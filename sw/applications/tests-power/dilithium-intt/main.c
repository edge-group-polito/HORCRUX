//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  ML-DSA Inverse NTT — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the ML-DSA (Dilithium) inverse Number
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
#include "dilithium.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"



#define SW_TEST_ENABLED 0

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif





static void dilithium_intt_sw_compute(dsa_poly *a) {
    dilithium_invntt(a->coeffs);
}



static void dilithium_intt_hw_compute(dsa_poly *a) {
    int k = 256;
    const int32_t f = 41978; // Scaling factor

    // Inverse NTT GS-Butterfly Stages
    for (int len = 1; len < 256; len <<= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = -zetas_DSA[--k];
            bfu_result_t out;
            for (int j = start; j < start + len; ++j) {
                uint32_t rs1 = (uint32_t)a->coeffs[j];
                uint32_t rs2 = (uint32_t)a->coeffs[j+len];
                
                asm volatile ( 
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x3,  %[rd1], %[r1], t0, %[r3] \n" 
                    ".insn r 0x3b, 0x7, 0x18, %[rd2], x0, x0 \n"
                    : [rd1] "=&r" (out.rd_lo), [rd2] "=&r" (out.rd_hi)
                    : [r1] "r" (rs1), 
                      [r2] "r" (rs2), 
                      [r3] "r" (zeta)
                    : "cc", "t0" 
                );

                a->coeffs[j]     = (int32_t)out.rd_lo;
                a->coeffs[j+len] = (int32_t)out.rd_hi;
            }
        }
    }

    // Final Scaling Stage
    for (int j = 0; j < 256; ++j) {
        asm volatile ( 
            "addi t0, %[r1], 0\n "
            ".insn r 0x3b, 0x7, 0x12, %[rd], t0, %[r2] \n" 
            : [rd] "=&r" ((uint32_t)a->coeffs[j]) 
            : [r1] "r" ((uint32_t)a->coeffs[j]), [r2] "r" (f)
            : "cc", "t0"
        );

    }
}



int main(void) {

    dsa_poly a_sw, a_hw;


#if SW_TEST_ENABLED
    a_sw = dsa_tv_intt_in;
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    dilithium_intt_sw_compute(&a_sw);
    vcd_disable();

#else

#endif

    a_hw = dsa_tv_intt_in;

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    dilithium_intt_hw_compute(&a_hw);
    vcd_disable();

    return 0;
}
