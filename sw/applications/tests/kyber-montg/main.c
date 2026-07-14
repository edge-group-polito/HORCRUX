///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Generic Montgomery reduction tests for ML-DSA (Dilithium) and ML-KEM (Kyber)
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



#define N_TESTS  20
// ---------------------- ML-KEM (Kyber) test --------------------------
#define KYBER_Q     3329
#define KYBER_QINV  62209    // matches Kyber reference


#define MONTG_KYBER(dest, a) \
    asm volatile ( \
        "addi t0, %[r1], 0\n" \
        ".insn r 0x3b, 0x7, 0x11, %[rd], t0, %[q] \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a), [q] "r" (1) \
    );

static inline int16_t montgomery_reduce_kem(int32_t a) {
    int16_t t;
    t = (int16_t)a * KYBER_QINV;
    t = (a - (int32_t)t * KYBER_Q) >> 16;
    return t;
}


int main(void) {

    // ---------------- ML-KEM (Kyber) --------------------
    const int32_t kem_inputs[N_TESTS] = {
    0, 1, -1, 1234567, -1234567, 2147483647, -2147483648, 0x0000FFFF,0xFFFF0000, 3329, 2*3329,  -3329,
    2147483647, -2147483648, 9999999, -9999999, 65535, -65535, 12345, -54321,
    987654321, 4294967295, 305419896, 1234567890, 4000000000, 789654123, 2222222222, 3456789012,
    111111111, 1357913579, 246802468, 3762837462, 987987987, 1029384756, 876543210, 3141592653,
    2718281828, 1618033988, 2020202020, 4242424242, 858993459, 123123123, 999999999, 888888888,
    777777777, 666666666, 555555555, 444444444, 333333333, 222222222, 1111111111, 98765432,
    2109876543, 1300130013, 3856385638, 2468246824, 1233211233, 987001234, 567898765, 4040404040,
    909090909, 808080808, 707070707, 606060606, 505050505, 909192939, 424344454, 565758596,
    98765430, 87654321, 76543210, 65432109, 54321098, 43210987, 32109876, 21098765, 10987654,
    1987654321, 2987654321, 3987654321, 123987456, 987123654, 192837465, 564738291, 102938475,
    918273645, 817263544, 716253443, 615243342, 514233241, 413223140, 312213039, 211202938,
    110192837, 901928374, 801918273, 701908172, 601898071, 501887970, 401877869, 301867768,
    201857667, 101847566  };

    const int16_t kem_golden[N_TESTS] = {
        0, 169, -169, 77, -77, 32599, -32768, -168, -1, 0, 0, 0, 32599, -32768, -309, 309, -168, 168, -978, 1133, 13670, -169, 3848, 17683, -5021, 12941, -32324, -14117, 645, 20110, 3504, -9704, 16622, 17291, 13025, -17564, -22763, 25401, 30798, -1526, 14405, 3144, 15792, 15147, 11173, 10528, 9883, 5909, 5264, 4619, 16606, 1683, 32321, 19216, -5345, -29293, 17538, 14962, 7709, -3940, 14192, 12985, 11778, 10571, 9364, 13042, 5883, 8340, 1345, 2967, 922, 567, 2106, -718, -566, -615, 655, 29631, -19944, -3983, 2230, 14207, 3120, 9008, 629, 13317, 12110, 10903, 9696, 8489, 7282, 6075, 4868, 332, 13454, 12233, 11012, 9791, 8570, 7349
    };


    int16_t got_sw[N_TESTS] = {0};
    int16_t got_hw[N_TESTS] = {0};
    int kem_ok_sw = 1;
    int kem_ok_hw = 1;
    int all_passed = 1;
    unsigned cycles_sw = 0;
    unsigned cycles_hw = 0;


    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        got_sw[i] = montgomery_reduce_kem(kem_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("Software Cycles: %u\n", cycles_sw);

    for (int i = 0; i < N_TESTS; i++) {
        if (got_sw[i] != kem_golden[i]) {
            printf("[KEM][SW] Test %2d FAIL: a=%d  exp=%d  got=%d\n", i, kem_inputs[i], kem_golden[i], got_sw[i]);
            kem_ok_sw = 0;
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (int i = 0; i < N_TESTS; i++) {
        MONTG_KYBER(got_hw[i], kem_inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

    for (int i = 0; i < N_TESTS; i++) {
        if (got_hw[i] != kem_golden[i]) {
            printf("[KEM][HW] Test %2d FAIL: a=%d  exp=%d  got=%d\n", i, kem_inputs[i], kem_golden[i], got_hw[i]);
            kem_ok_hw = 0;
        }
    }

#if SW_TEST_ENABLED
    if (cycles_sw > 0 && cycles_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    all_passed = kem_ok_sw && kem_ok_hw;
    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return 0;
}
