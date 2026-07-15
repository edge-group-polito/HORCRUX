//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  Falcon FPR_NORM64 — Power Characterization
// Language:     C
// Date:         April 2026
//
// Description:  Power-characterization test for the Falcon FPR_NORM64 mantissa
//               normalisation routine.
//               Isolates the operation under a GPIO-triggered VCD dump window (see
//               sw/applications/tests-power/README.md) for post-synthesis SW-vs-HW power
//               comparison; SW_TEST_ENABLED selects the software reference or the
//               HORCRUX-accelerated path.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "vcd_util.h"

#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

typedef uint64_t fpr;

static void print_u64_hex(uint64_t x)
{
    printf("0x%08lX%08lX",
           (unsigned long)(uint32_t)(x >> 32),
           (unsigned long)(uint32_t)x);
}

static inline fpr
FPR(int s, int e, uint64_t m)
{
    fpr x;
    uint32_t t;
    unsigned f;

    /*
     * If e >= -1076, then the value is "normal"; otherwise, it
     * should be a subnormal, which we clamp down to zero.
     */
    e += 1076;
    t = (uint32_t)e >> 31;
    m &= (uint64_t)t - 1;

    /*
     * If m = 0 then we want a zero; make e = 0 too, but conserve
     * the sign.
     */
    t = (uint32_t)(m >> 54);
    e &= -(int)t;

    /*
     * The 52 mantissa bits come from m.
     */
    x = (((uint64_t)s << 63) | (m >> 2)) + ((uint64_t)(uint32_t)e << 52);

    /*
     * Round-to-nearest, ties-to-even.
     */
    f = (unsigned)m & 7U;
    x += (0xC8U >> f) & 1;
    return x;
}

#define FPR_NORM64(m, e)   do { \
        uint32_t nt; \
 \
        (e) -= 63; \
 \
        nt = (uint32_t)((m) >> 32); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) << 32)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 5); \
 \
        nt = (uint32_t)((m) >> 48); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) << 16)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 4); \
 \
        nt = (uint32_t)((m) >> 56); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) <<  8)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 3); \
 \
        nt = (uint32_t)((m) >> 60); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) <<  4)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 2); \
 \
        nt = (uint32_t)((m) >> 62); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) <<  2)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 1); \
 \
        nt = (uint32_t)((m) >> 63); \
        (m) ^= ((m) ^ ((m) <<  1)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt); \
    } while (0)

typedef struct {
    int s;
    int e;
    uint64_t m;
    uint64_t expected;
} fpr_test_vector_t;

typedef struct {
    uint64_t m;
    int e;
    uint64_t expected_m;
    int expected_e;
} norm64_test_vector_t;



static const norm64_test_vector_t NORM64_TV[] = {
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100},
    {0x0000000000000000ULL,   9, 0x0000000000000000ULL,  -54},
    {0x0000000000000001ULL,   9, 0x8000000000000000ULL,  -54},
    {0x0000000000000002ULL,   9, 0x8000000000000000ULL,  -53},
    {0x0000000000000003ULL,   9, 0xC000000000000000ULL,  -53},
    {0x0000000100000000ULL,   9, 0x8000000000000000ULL,  -22},
    {0x0001000000000000ULL,   9, 0x8000000000000000ULL,   -6},
    {0x4000000000000000ULL,   9, 0x8000000000000000ULL,    8},
    {0x7FFFFFFFFFFFFFFFULL,   9, 0xFFFFFFFFFFFFFFFEULL,    8},
    {0x8000000000000000ULL,   9, 0x8000000000000000ULL,    9},
    {0xF000000000000000ULL, -100, 0xF000000000000000ULL, -100}
};

#define NORM64_TV_COUNT (sizeof(NORM64_TV) / sizeof(NORM64_TV[0]))


static inline void norm64_hw_run_triplet(uint32_t m_lo, uint32_t m_hi, int32_t e,
                                         uint32_t *out_m_lo, uint32_t *out_m_hi, int32_t *out_e)
{
    uint32_t rd_m_lo;
    uint32_t rd_m_hi;
    uint32_t rd_e;
    uint32_t bubble;

    asm volatile(
        "addi t0, %[in_mlo], 0 \n"
        "addi t1, %[in_mhi], 0 \n"
        "addi t2, %[in_e],   0 \n"
        ".insn r 0x6b, 0x6, 0x3, %[mlo], %[in_mlo], %[in_mhi], t2 \n"
        ".insn r 0x3b, 0x7, 0x36, %[mhi], x0, x0 \n"
        "addi %[b], x0, 0 \n"
        ".insn r 0x3b, 0x7, 0x37, %[eout], x0, x0 \n"
        : [mlo] "=&r" (rd_m_lo), [mhi] "=&r" (rd_m_hi), [eout] "=&r" (rd_e), [b] "=&r" (bubble)
        : [in_mlo] "r" (m_lo),
          [in_mhi] "r" (m_hi),
          [in_e] "r" ((uint32_t)e)
        : "t0", "t1", "t2", "memory");

    *out_m_lo = rd_m_lo;
    *out_m_hi = rd_m_hi;
    *out_e = (int32_t)rd_e;
}


static int run_norm64_sw_tests(uint64_t got_m_vec[NORM64_TV_COUNT], int32_t got_e_vec[NORM64_TV_COUNT])
{
    for (unsigned i = 0; i < NORM64_TV_COUNT; i++) {
        uint64_t m = NORM64_TV[i].m;
        int e = NORM64_TV[i].e;
        FPR_NORM64(m, e);
        got_m_vec[i] = m;
        got_e_vec[i] = (int32_t)e;
    }

    return 1;
}


static int run_norm64_hw_placeholder(uint64_t hw_m_vec[NORM64_TV_COUNT], int32_t hw_e_vec[NORM64_TV_COUNT])
{
    for (unsigned i = 0; i < NORM64_TV_COUNT; i++) {
        uint32_t hw_m_lo;
        uint32_t hw_m_hi;
        int32_t hw_e;

        norm64_hw_run_triplet((uint32_t)(NORM64_TV[i].m & 0xFFFFFFFFu),
                              (uint32_t)(NORM64_TV[i].m >> 32),
                              (int32_t)NORM64_TV[i].e,
                              &hw_m_lo,
                              &hw_m_hi,
                              &hw_e);

        hw_m_vec[i] = ((uint64_t)hw_m_hi << 32) | (uint64_t)hw_m_lo;
        hw_e_vec[i] = hw_e;
    }

    return 1;
}

int main(void)
{
    int all_passed = 1;


    uint64_t got_m_vec[NORM64_TV_COUNT];
    int32_t got_e_vec[NORM64_TV_COUNT];
    uint64_t hw_m_vec[NORM64_TV_COUNT];
    int32_t hw_e_vec[NORM64_TV_COUNT];

#if SW_TEST_ENABLED

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    all_passed &= run_norm64_sw_tests(got_m_vec, got_e_vec);
    vcd_disable();

#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    all_passed &= run_norm64_hw_placeholder(hw_m_vec, hw_e_vec);
    vcd_disable();

    return 0;
}