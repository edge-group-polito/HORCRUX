/**
 * @file main.c
 * @brief Falcon INTT Test - Software vs Hardware Comparison
 * 
 * Tests the Inverse Number Theoretic Transform (INTT) operation for Falcon
 * digital signature algorithm using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "falcon.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

#define MQMULF(dest, a, b) \
    asm volatile ( \
        "addi t0, %[r1], 0\n" \
        ".insn r 0x3b, 0x7, 0x10, %[rd], t0, %[r2] \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a), [r2] "r" (b) \
    );

#define BFINTTF(dest, a, b, c) \
    asm volatile ( \
        "addi t0, %[r2], 0\n" \
        ".insn r 0x6b, 0x06, 0x1, %[rd], %[r1], t0, %[r3] \n" \
        : [rd] "=&r" (dest) \
        : [r1] "r" (a), [r2] "r" (b), [r3] "r" (c) \
        : "cc", "t0" \
    );

static int compare_coeffs(const char* test_name, int idx, int32_t got, int32_t exp) {
    int match = (got == exp);
#if DEBUG
    printf("%s [%3d] -> Got: %8d | Exp: %8d %s\n", 
           test_name, idx, got, exp, match ? " " : "[MISMATCH]");
#endif
    if (!match && !DEBUG) {
        printf("[%s ERROR] Mismatch @ idx %3d: Got %8d, Expected %8d\n", 
               test_name, idx, got, exp);
    }
    return match;
}

static void falcon_intt_sw_compute(uint16_t *poly) {
    for (int i = 0; i < FALCON_N; i++) {
        poly[i] = falcon_intt_input[i];
    }
    mq_iNTT_sw(poly, 4);
}

static int falcon_intt_sw_verify(const uint16_t *poly) {
    int ok = 1;
    for (int i = 0; i < FALCON_N; i++) {
        ok &= compare_coeffs("F-INTT-SW", i, (int32_t)poly[i], (int32_t)falcon_intt_output[i]);
    }
    if (ok) printf("PASSED: Falcon iNTT (Software)\n");
    return ok;
}

static void falcon_intt_hw_compute(uint16_t *poly) {
    for (int i = 0; i < FALCON_N; i++) {
        poly[i] = falcon_intt_input[i];
    }

    unsigned logn = 4;
    size_t n = (size_t)1 << logn;
    size_t t = 1;
    size_t m = n;

    while (m > 1) {
        size_t hm = m >> 1;
        size_t dt = t << 1;
        for (size_t i = 0, j1 = 0; i < hm; i++, j1 += dt) {
            int16_t s = (int16_t)falcon_iGMb_16[hm + i];
            size_t j2 = j1 + t;
            for (size_t j = j1; j < j2; j++) {
                uint32_t rs1 = (uint32_t)poly[j];
                uint32_t rs2 = (uint32_t)poly[j + t];
                uint32_t out;

                BFINTTF(out, rs1, rs2, (int32_t)s);

                poly[j] = (uint16_t)(out & 0xFFFF);
                poly[j + t] = (uint16_t)(out >> 16);
            }
        }
        t = dt;
        m = hm;
    }

    // Divide by n
    uint32_t ni = FALCON_R;
    for (size_t i = n; i > 1; i >>= 1) {
        ni += FALCON_Q & -(ni & 1);
        ni = ni >> 1;
    }

    for (size_t i = 0; i < n; i++) {
        uint32_t out;
        MQMULF(out, (uint32_t)poly[i], ni);
        poly[i] = (uint16_t)out;
    }
}

static int falcon_intt_hw_verify(const uint16_t *poly) {
    int ok = 1;
    for (int i = 0; i < FALCON_N; i++) {
        ok &= compare_coeffs("F-INTT-HW", i, (int32_t)poly[i], (int32_t)falcon_intt_output[i]);
    }
    if (ok) printf("PASSED: Falcon iNTT (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;

    uint16_t poly_sw[FALCON_N], poly_hw[FALCON_N];


#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();    
    falcon_intt_sw_compute(poly_sw);
    vcd_disable();
    
#else

#endif
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    falcon_intt_hw_compute(poly_hw);
    vcd_disable();


    return 0;
}
