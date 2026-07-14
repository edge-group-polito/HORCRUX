/**
 * @file main.c
 * @brief HQC Karatsuba 64x64 Multiplication Test - Software vs Hardware
 * 
 * Tests the Karatsuba multiplication for HQC (Hamming Quasi-Cyclic)
 * code-based encryption using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hqc.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

static void karats_sw_compute(uint64_t results[][2]) {
    for (int i = 0; i < NUM_HQC_TESTS; i++) {
        gf2_mul64_sw(results[i], hqc_a[i], hqc_b[i]);
    }
}

typedef struct {
    uint64_t c0;
    uint64_t c1;
} karats_hw_result_t;

static void karats_hw_compute(karats_hw_result_t *results) {
    for (int i = 0; i < NUM_HQC_TESTS; i++) {
        bfu_result_t out1, out2, out3;
        asm volatile ( 
            "addi t0, %[r1], 0\n "
            ".insn r 0x3b, 0x7, 0x13, %[rd1], t0,  %[r2] \n" 
            ".insn r 0x3b, 0x7, 0x14, %[rd2], %[r1b], %[r2b] \n" 
            ".insn r 0x3b, 0x7, 0x15, %[rd3], t0,  %[r2] \n" 
            ".insn r 0x3b, 0x7, 0x19, %[rd4], x0, x0 \n" 
            : [rd1] "=r" (out1.rd_lo), [rd2] "=r" (out2.rd_lo), 
              [rd3] "=r" (out3.rd_lo), [rd4] "=r" (out3.rd_hi)
            : [r1]  "r" ((uint32_t)hqc_a[i]), 
              [r2]  "r" ((uint32_t)hqc_b[i]), 
              [r1b] "r" ((uint32_t)(hqc_a[i] >> 32)), 
              [r2b] "r" ((uint32_t)(hqc_b[i] >> 32))
            : "cc", "t0"
        );

        results[i].c0 = out1.rd_lo | ((uint64_t)out3.rd_lo << 32);
        results[i].c1 = out3.rd_hi | ((uint64_t)out2.rd_lo << 32);
    }
}



int main(void) {
    int all_passed = 1;

    uint64_t results_sw[NUM_HQC_TESTS][2];
    karats_hw_result_t results_hw[NUM_HQC_TESTS];



#if SW_TEST_ENABLED

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    karats_sw_compute(results_sw);
    vcd_disable();

#else

#endif
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    karats_hw_compute(results_hw);
    vcd_disable();

    return 0;
}
