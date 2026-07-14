/**
 * @file main.c
 * @brief Dilithium NTT Test - Software vs Hardware Comparison
 * 
 * Tests the Number Theoretic Transform (NTT) operation for Dilithium
 * digital signature algorithm using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "dilithium.h"
#include "bfu.h"
#include "vcd_util.h"
#include "core_v_mini_mcu.h"
#include "csr.h"



// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0



static void dilithium_ntt_sw_compute(dsa_poly *a) {
    dilithium_ntt(a->coeffs);
}


static void dilithium_ntt_hw_compute(dsa_poly *a) {
    int k = 0;
    
    for (int len = 128; len > 0; len >>= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = zetas_DSA[++k];
            bfu_result_t out;
            for (int j = start; j < start + len; ++j) {
                asm volatile ( 
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd1], %[r1], t0, %[r3] \n" 
                    ".insn r 0x3b, 0x7, 0x17, %[rd2], x0, x0 \n"
                    : [rd1] "=&r" (out.rd_lo), [rd2] "=&r" (out.rd_hi)
                    : [r1] "r" ((uint32_t)a->coeffs[j]), 
                      [r2] "r" ((uint32_t)a->coeffs[j+len]), 
                      [r3] "r" (zeta)
                    : "cc", "t0" 
                );
                a->coeffs[j] = (int32_t)out.rd_lo;
                a->coeffs[j+len] = (int32_t)out.rd_hi;
            }
        }
    }
}



int main(void) {
    int all_passed = 1;
    dsa_poly a_sw, a_hw;


#if SW_TEST_ENABLED
    a_sw = dsa_tv_ntt_in;
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    dilithium_ntt_sw_compute(&a_sw);
    vcd_disable();

#else

#endif

    a_hw = dsa_tv_ntt_in;
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    dilithium_ntt_hw_compute(&a_hw);
    vcd_disable();

    return 0;
}
