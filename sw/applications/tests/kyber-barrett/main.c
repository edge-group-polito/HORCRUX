///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Auth: Alessandra Dolmeta, Valeria Piscopo
// Email: alessandra.dolmeta@polito.it, valeria.piscopo@polito.it
// Affiliation: Politecnico di Torino - @VLSI Lab
// Date: October 2025
//
///////////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif


// Placeholder values; replace these with actual values from Kyber and Dilithium specifications
#define QINV_KYBER       62209
#define KYBER_Q          3329
#define QINV_DILITHIUM   58728449
#define DILITHIUM_Q      8380417


#define NTESTS_BARRETT   20


//#define BARRET_KYBER(dest, a) \
//    asm volatile ( \
//        "addi t0, %[r1], 0\n" \
//        ".insn r 0x3b, 0x7, 0x5, %[rd], t0, x0 \n" \
//        : [rd] "=r" (dest) \
//        : [r1] "r" (a) : "cc" \
//    );

static inline __attribute__((always_inline))
int32_t barrett_reduce_asm(int32_t a)
{
    int32_t out;
    asm volatile (
        "addi t0, %1, 0\n\t"                // t0 := rs1 copy
        ".insn r 0x3b, 0x7, 5, %0, t0, x0\n\t"
        : "=&r"(out)                        // rd (early-clobber)
        : "r"(a)                            // rs1 (via t0)
        : "t0", "cc"                        // we modify t0
    );
    return out;
}

// Kyber Barrett reduction (software)
static int32_t KYBER_barrett_reduce(int32_t a)
{
    int32_t t;
    const int32_t v = ((1 << 26) + KYBER_Q / 2) / KYBER_Q;
    t = ((int32_t)v * a + (1 << 25)) >> 26;
    t *= KYBER_Q;
    return a - t;
}


int main() {
    int i;

    int16_t barrett_inputs[300] = {
        -10904, -9291, -4864, -3945, -2106, -3003, -1134, 2015, -5203, -9168, 
        -1555, -2820, -2298, -1698, 1568, 822, -4740, -5621, -3516, -3461, 
        -6631, -2311, -1745, 25, -1790, -2832, -388, 1342, 2983, 3602, 
        7267, 4920, -10502, -7937, -7532, -6685, -3220, -3963, -1698, -3315, 
        -5910, -4132, -1666, -750, 877, 435, 2699, 5859, -1252, -3575, 
        1168, 727, -2321, -778, 3569, 3302, -3406, -3492, -86, -1824, 
        -172, 2426, 5052, 2678, -11788, -6393, -8102, -4733, -9432, -4272, 
        -5834, 1070, -8942, -4046, -5568, -3200, -2086, -2146, 784, -2104, 
        -5694, -8081, -1714, -4769, -3331, -5889, 1903, -5289, -4771, -3773, 
        -1363, 2669, -839, -2038, 5033, 4290, -9590, -4399, -5912, -1451, 
        -1766, -3909, -1596, -833, -1275, -4358, 51, 608, -1120, 3434, 
        2976, 5748, -4187, -4940, -683, -2788, 2946, -442, 3304, 3390, 
        3858, -444, 5660, 680, 2462, 2777, 7160, 5743, -5060, -8586, 
        -4474, -6320, -1714, -4862, 4404, 1688, -481, -3763, 4793, -2241, 
        5608, -627, 8988, 4927, -4915, -7744, 155, -5768, 4009, -3878, 
        4195, 702, 366, -3436, 4638, -26, 2560, 2869, 3344, 5785, 
        -2656, -4131, -2438, -3243, -77, 109, 5091, 2053, 187, -881, 
        303, 2039, 486, 3677, 2216, 4385, -208, 1948, 3550, 3560, 
        422, 4010, 6088, 10210, -366, 2919, 5032, 8295, 6692, 11126, 
        8718, 12468, -6312, -1703, -452, -963, -3973, -828, 549, 474, 
        -6255, -1014, -141, 3904, 413, 1336, 1875, 5754, -2106, -580, 
        1122, 2518, -651, 1370, 1591, 6036, -2744, -2625, 1314, 4029, 
        3246, 4879, 3932, 9957, -355, -4301, 3461, 857, -681, 2524, 
        5995, 6308, 656, -2333, 3566, 3787, 2452, 5554, 5914, 8884, 
        -1624, -2497, 3480, -167, 4098, 115, 5598, 1597, 7764, 1814, 
        9126, 5726, 9509, 4383, 9089, 10405, -13401, -12388, -7083, -8242, 
        -8035, -9187, -4381, -2883, -6884, -4491, -1638, -4305, -3649, -4879, 
        2463, 791, -5536, -9329, -2588, -7183, -4290, -2779, -2258, -1969, 
        -6590, -6709, -2738, -5415, -1213, -3342, 4653, 2406, -10255, -6518, 
        -7073, -6382, -6269, -9043, -6039, -3201, -4208, -4931, -2082, -3877
    };

    int16_t barrett_exp[150] = {
        -917, 696,  -1535,   -616,   1223, 326,  -1134,  -1314,   1455, 819,  -1555, 509,   1031,   1631,   1568, 822,  -1411,   1037,   -187,   -132,  27,   1018,   1584,  25,   1539, 497,   -388,   1342,   -346, 273, 609,   1591,   -515,  -1279,   -874, -27, 109,   -634,   1631,  14, 748,   -803,   1663,   -750, 877, 435,   -630,   -799,  -1252,   -246,   1168, 727,   1008,   -778, 240, -27, -77,   -163, -86,   1505,   -172,   -903,  -1606,   -651,   1528, 265,  -1444,  -1404, 555,   -943, 824,   1070,   1045,   -717,   1090, 129,   1243,   1183, 784,   1225, 964,  -1423,   1615,  -1440,  -2, 769,  -1426,   1369,  -1442,   -444,  -1363,   -660,   -839,   1291,  -1625, 961, 397,  -1070, 746,  -1451,   1563,   -580,  -1596,   -833,  -1275,  -1029,  51, 608,  -1120, 105,   -353,   -910,   -858,  -1611,   -683, 541,   -383,   -442, -25,  61, 529,   -444,   -998, 680,   -867,   -552, 502,   -915,   1598,   1401,  -1145, 338,   1615,  -1533,   1075,  -1641,   -481,   -434,   1464,   1088,  -1050,   -627,   -999,   1598,  -1586,  -1086, 155, 890, 680,   -549
    };

    int16_t barrett_res_sw[NTESTS_BARRETT];
    int16_t barrett_res_hw[NTESTS_BARRETT];
    int all_passed = 1;
    int all_pass_sw = 1;
    int all_pass_hw = 1;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    unsigned cycles_barrett_sw = 0;
    unsigned cycles_barrett_hw = 0;

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (i = 0; i < NTESTS_BARRETT; i++) {
        barrett_res_sw[i] = KYBER_barrett_reduce(barrett_inputs[i]);
    }
    CSR_READ(CSR_REG_MCYCLE, &cycles_barrett_sw);
    printf("Software Cycles: %u\n", cycles_barrett_sw);

    for (i = 0; i < NTESTS_BARRETT; i++) {
        if (barrett_res_sw[i] != barrett_exp[i]) {
            printf("  [SW][%2d] FAIL: d=%d, exp=%d, got=%d\n", i, barrett_inputs[i], barrett_exp[i], barrett_res_sw[i]);
            all_pass_sw = 0;
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (i = 0; i < NTESTS_BARRETT; i++) {
        barrett_res_hw[i] = barrett_reduce_asm(barrett_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_barrett_hw);
    printf("Hardware Cycles: %u\n", cycles_barrett_hw);

    for (i = 0; i < NTESTS_BARRETT; i++) {
        if (barrett_res_hw[i] != barrett_exp[i]) {
            printf("  [HW][%2d] FAIL: d=%d, exp=%d, got=%d\n", i, barrett_inputs[i], barrett_exp[i], barrett_res_hw[i]);
            all_pass_hw = 0;
        }
    }

#if SW_TEST_ENABLED
    if (cycles_barrett_sw > 0 && cycles_barrett_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_barrett_sw, cycles_barrett_hw);
        printf("Speedup: %u.%02ux\n", cycles_barrett_sw / cycles_barrett_hw, ((cycles_barrett_sw * 100) / cycles_barrett_hw) % 100);
    }
#endif

    all_passed = all_pass_sw && all_pass_hw;
    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return 0;
}