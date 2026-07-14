/**
 * @file main.c
 * @brief Falcon NTT Test - Software vs Hardware Comparison
 * 
 * Tests the Number Theoretic Transform (NTT) operation for Falcon
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

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

#define BFNTTF(dest, a, b, c) \
    asm volatile ( \
        "addi t0, %[r2], 0\n" \
        ".insn r 0x6b, 0x06, 0x0, %[rd], %[r1], t0, %[r3] \n" \
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

static void falcon_ntt_sw_compute(uint16_t *poly) {
    for (int i = 0; i < FALCON_N; i++) {
        poly[i] = falcon_ntt_input[i];
    }
    mq_NTT_sw(poly, 4);
}

static int falcon_ntt_sw_verify(const uint16_t *poly) {
    int ok = 1;
    for (int i = 0; i < FALCON_N; i++) {
        ok &= compare_coeffs("F-NTT-SW", i, (int32_t)poly[i], (int32_t)falcon_ntt_output[i]);
    }
    if (ok) printf("PASSED: Falcon NTT (Software)\n");
    return ok;
}

static void falcon_ntt_hw_compute(uint16_t *poly) {
    for (int i = 0; i < FALCON_N; i++) {
        poly[i] = falcon_ntt_input[i];
    }

    unsigned logn = 4;
    size_t n = (size_t)1 << logn;
    size_t t = n;

    for (size_t m = 1; m < n; m <<= 1) {
        size_t ht = t >> 1;
        for (size_t i = 0, j1 = 0; i < m; i++, j1 += t) {
            int16_t s = (int16_t)falcon_GMb_16[m + i];
            size_t j2 = j1 + ht;
            for (size_t j = j1; j < j2; j++) {
                uint32_t rs1 = (uint32_t)poly[j];
                uint32_t rs2 = (uint32_t)poly[j + ht];
                uint32_t out;

                BFNTTF(out, rs1, rs2, (int32_t)s);

                poly[j] = (uint16_t)(out & 0xFFFF);
                poly[j + ht] = (uint16_t)(out >> 16);
            }
        }
        t = ht;
    }
}

static int falcon_ntt_hw_verify(const uint16_t *poly) {
    int ok = 1;
    for (int i = 0; i < FALCON_N; i++) {
        ok &= compare_coeffs("F-NTT-HW", i, (int32_t)poly[i], (int32_t)falcon_ntt_output[i]);
    }
    if (ok) printf("PASSED: Falcon NTT (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    uint16_t poly_sw[FALCON_N], poly_hw[FALCON_N];

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("    FALCON NTT TEST                  \n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    falcon_ntt_sw_compute(poly_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= falcon_ntt_sw_verify(poly_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    falcon_ntt_hw_compute(poly_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= falcon_ntt_hw_verify(poly_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
    if (cycles_sw > 0) {
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return EXIT_SUCCESS;
}
