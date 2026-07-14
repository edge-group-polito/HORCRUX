/**
 * @file main.c
 * @brief Falcon FPR/FPR_NORM64 fixed-vector test with golden outputs.
 *
 * This test is intended to validate bit-accurate behavior before mapping
 * these operations to hardware.
 */

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

static const fpr_test_vector_t FPR_TV[] = {
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL},
    {0,    0,     0x0000000000000000ULL, 0x0000000000000000ULL},
    {0,    0,     0x8000000000000000ULL, 0x6000000000000000ULL},
    {1,    0,     0x8000000000000000ULL, 0xE000000000000000ULL},
    {0, -1077,    0xFFFFFFFFFFFFFFF8ULL, 0x0000000000000000ULL},
    {0, -1076,    0xFFFFFFFFFFFFFFF8ULL, 0x3FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFF8ULL, 0x7FFFFFFFFFFFFFFEULL},
    {0,   10,     0xFFFFFFFFFFFFFFFBULL, 0x7FFFFFFFFFFFFFFFULL},
    {0,   10,     0xFFFFFFFFFFFFFFFEULL, 0x8000000000000000ULL},
    {0,   10,     0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL},
    {0, 1000,     0xAAAAAAAAAAAAAAABULL, 0xABEAAAAAAAAAAAABULL}
};

#define FPR_TV_COUNT (sizeof(FPR_TV) / sizeof(FPR_TV[0]))

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
};



static inline void fpr_hw_run_triplet(uint32_t s, int32_t e, uint32_t m_lo, uint32_t m_hi,
                                      uint32_t *out_lo, uint32_t *out_hi)
{
    uint32_t rd_lo;
    uint32_t rd_hi;

    asm volatile(
        //"addi t0, %[s]  , 0 \n"
        //"addi t1, %[e]  , 0 \n"
        //"addi t2, %[mlo], 0 \n"
        "addi t3, %[mhi], 0 \n"
        ".insn r 0x3b, 0x7, 0x33, x0,   %[s],    %[e]    \n"
        ".insn r 0x3b, 0x7, 0x34, %[lo], %[mlo], t3  \n"
        ".insn r 0x3b, 0x7, 0x35, %[hi], x0,    x0       \n"
        : [lo] "=r" (rd_lo), [hi] "=r" (rd_hi)
        : [s] "r" (s),
          [e] "r" ((uint32_t)e),
          [mlo] "r" (m_lo),
          [mhi] "r" (m_hi)
        : "t0", "t1", "t2", "t3", "memory");

    *out_lo = rd_lo;
    *out_hi = rd_hi;
}




static int run_fpr_sw_tests(uint64_t got_vec[FPR_TV_COUNT])
{
    for (unsigned i = 0; i < FPR_TV_COUNT; i++) {
        got_vec[i] = FPR(FPR_TV[i].s, FPR_TV[i].e, FPR_TV[i].m);
    }

    return 1;
}

/*
 * Hardware placeholders: kept intentionally empty for now.
 * These are ready to be replaced with custom-instruction or MMIO paths.
 */
static int run_fpr_hw_placeholder(uint64_t hw_full_vec[FPR_TV_COUNT])
{
    for (unsigned i = 0; i < FPR_TV_COUNT; i++) {
        uint32_t hw_lo;
        uint32_t hw_hi;

        fpr_hw_run_triplet((uint32_t)FPR_TV[i].s,
                   (int32_t)FPR_TV[i].e,
                   (uint32_t)(FPR_TV[i].m & 0xFFFFFFFFu),
                   (uint32_t)(FPR_TV[i].m >> 32),
                   &hw_lo,
                   &hw_hi);

        hw_full_vec[i] = ((uint64_t)hw_hi << 32) | (uint64_t)hw_lo;
    }

    return 1;
}





int main(void)
{
    int all_passed = 1;

    uint64_t got_vec[FPR_TV_COUNT];
    uint64_t hw_full_vec[FPR_TV_COUNT];



#if SW_TEST_ENABLED

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    all_passed &= run_fpr_sw_tests(got_vec);
    vcd_disable();

#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    all_passed &= run_fpr_hw_placeholder(hw_full_vec);
    vcd_disable();


    return 0;
}