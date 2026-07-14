/**
 * @file main.c
 * @brief HQC GF(2) 8-bit Carry-less Multiplication Test - Software vs Hardware
 * 
 * Tests the carry-less multiplication for HQC (Hamming Quasi-Cyclic)
 * code-based encryption using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hqc.h"
#include "vcd_util.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

static void gf_carryless_sw_compute(uint8_t results[][2]) {
    for (int i = 0; i < NTESTS_8BIT; i++) {
        gf_carryless_mul_sw(results[i], hqc_a_8bit[i], hqc_b_8bit[i]);
    }
}

static void gf_carryless_hw_compute(uint32_t *results) {
    for (int i = 0; i < NTESTS_8BIT; i++) {
        bfu_result_t out;
        asm volatile ( 
            "addi t0, %[r1], 0\n "
            ".insn r 0x3b, 0x7, 0x16, %[rd], t0, %[r2] \n" 
            : [rd] "=r" (out.rd_lo) 
            : [r1] "r" (hqc_a_8bit[i]), [r2] "r" (hqc_b_8bit[i])
            : "cc", "t0"
        );
        results[i] = out.rd_lo;
    }
}


int main(void) {
    int all_passed = 1;

    uint8_t results_sw[NTESTS_8BIT][2];
    uint32_t results_hw[NTESTS_8BIT];


#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    gf_carryless_sw_compute(results_sw);
    vcd_disable();

#else

#endif
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    gf_carryless_hw_compute(results_hw);
    vcd_disable();


    return 0;
}
