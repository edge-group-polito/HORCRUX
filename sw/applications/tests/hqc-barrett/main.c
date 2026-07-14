///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// HQC Barrett reduction test (SW + HW)
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>

#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define N_TESTS 20

#define HQC_PROFILE_1 1u
#define HQC_PROFILE_3 2u
#define HQC_PROFILE_5 3u

#define HQC1_N   17669u
#define HQC1_MU  243079u
#define HQC3_N   35851u
#define HQC3_MU  119800u
#define HQC5_N   57637u
#define HQC5_MU  74517u

#define HQC1_BARRETT(dest, x) \
    asm volatile ( \
        "addi t0, %[vx], 0\n" \
        ".insn r 0x3b, 0x7, 0x58, %[rd], t0, x0 \n" \
        : [rd] "=&r" (dest) \
        : [vx] "r" (x) \
        : "t0", "cc" \
    );

#define HQC3_BARRETT(dest, x) \
    asm volatile ( \
        "addi t0, %[vx], 0\n" \
        ".insn r 0x3b, 0x7, 0x59, %[rd], t0, x0 \n" \
        : [rd] "=&r" (dest) \
        : [vx] "r" (x) \
        : "t0", "cc" \
    );

#define HQC5_BARRETT(dest, x) \
    asm volatile ( \
        "addi t0, %[vx], 0\n" \
        ".insn r 0x3b, 0x7, 0x5a, %[rd], t0, x0 \n" \
        : [rd] "=&r" (dest) \
        : [vx] "r" (x) \
        : "t0", "cc" \
    );

static inline uint32_t hqc_barrett_sw(uint32_t x, uint32_t n, uint32_t mu) {
    uint64_t q = ((uint64_t)x * mu) >> 32;
    uint32_t r = x - (uint32_t)(q * n);
    uint32_t reduce_flag = (((r - n) >> 31) ^ 1u);
    uint32_t mask = (uint32_t)(-(int32_t)reduce_flag);
    r -= mask & n;
    return r;
}

static inline uint32_t get_n(uint32_t profile) {
    if (profile == HQC_PROFILE_1) return HQC1_N;
    if (profile == HQC_PROFILE_3) return HQC3_N;
    return HQC5_N;
}

static inline uint32_t get_mu(uint32_t profile) {
    if (profile == HQC_PROFILE_1) return HQC1_MU;
    if (profile == HQC_PROFILE_3) return HQC3_MU;
    return HQC5_MU;
}

static inline uint32_t hqc_barrett_hw(uint32_t x, uint32_t profile) {
    uint32_t out;
    if (profile == HQC_PROFILE_1) {
        HQC1_BARRETT(out, x);
    } else if (profile == HQC_PROFILE_3) {
        HQC3_BARRETT(out, x);
    } else {
        HQC5_BARRETT(out, x);
    }
    return out;
}

int main(void) {
    const uint32_t inputs[N_TESTS] = {
        0u, 1u, 2u, 17u, 255u, 1024u, 4096u, 65535u,
        16767880u, 16767881u, 16767882u, 0x00FFFFFFu,
        123456u, 654321u, 1000000u, 4000000u,
        8000000u, 12000000u, 16000000u, 20000000u,
        25000000u, 30000000u, 35000000u, 4294967295u
    };

    const uint32_t profiles[3] = {HQC_PROFILE_1, HQC_PROFILE_3, HQC_PROFILE_5};

    uint32_t sw_cycles = 0;
    uint32_t hw_cycles = 0;
    int sw_ok = 1;
    int hw_ok = 1;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("HQC Barrett Reduction Tests (profiles: HQC-1/HQC-3/HQC-5)\n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int p = 0; p < 3; p++) {
        uint32_t profile = profiles[p];
        uint32_t n = get_n(profile);
        uint32_t mu = get_mu(profile);

        for (int i = 0; i < N_TESTS; i++) {
            uint32_t got_sw = hqc_barrett_sw(inputs[i], n, mu);
            uint32_t exp = inputs[i] % n;
            if (got_sw != exp) {
                printf("[HQC][SW][P%u] Test %2d FAIL: x=%u exp=%u got=%u\n",
                       profile, i, inputs[i], exp, got_sw);
                sw_ok = 0;
            }
        }
    }
    CSR_READ(CSR_REG_MCYCLE, &sw_cycles);
    printf("Software Cycles: %u\n", sw_cycles);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int p = 0; p < 3; p++) {
        uint32_t profile = profiles[p];
        uint32_t n = get_n(profile);
        uint32_t mu = get_mu(profile);

        for (int i = 0; i < N_TESTS; i++) {
            uint32_t got_hw;
            uint32_t exp = hqc_barrett_sw(inputs[i], n, mu);
            got_hw = hqc_barrett_hw(inputs[i], profile);
            if (got_hw != exp) {
                printf("[HQC][HW][P%u] Test %2d FAIL: x=%u exp=%u got=%u\n",
                       profile, i, inputs[i], exp, got_hw);
                hw_ok = 0;
            }
        }
    }
    CSR_READ(CSR_REG_MCYCLE, &hw_cycles);
    printf("Hardware Cycles: %u\n", hw_cycles);

#if SW_TEST_ENABLED
    if (sw_cycles > 0 && hw_cycles > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", sw_cycles, hw_cycles);
        printf("Speedup: %u.%02ux\n", sw_cycles / hw_cycles, ((sw_cycles * 100) / hw_cycles) % 100);
    }
#endif

    if (sw_ok && hw_ok) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return 0;
}
