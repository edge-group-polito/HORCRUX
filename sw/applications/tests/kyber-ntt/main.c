/**
 * @file main.c
 * @brief Kyber NTT Test - Software vs Hardware Comparison
 * 
 * Tests the Number Theoretic Transform (NTT) operation for Kyber
 * key encapsulation mechanism using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "kyber.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"


#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

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

static void kyber_ntt_sw_compute(kyber_poly *r) {
    kyber_ntt(r->coeffs);
}

static int kyber_ntt_sw_verify(const kyber_poly *r) {
    int ok = 1;
    for (int i = 0; i < KEM_N; i++) {
        ok &= compare_coeffs("K-NTT-SW", i, r->coeffs[i], kyber_tv_ntt_out.coeffs[i]);
    }
    if (ok) printf("PASSED: Kyber NTT (Software)\n");
    return ok;
}

static void kyber_ntt_hw_compute(kyber_poly *r) {
    int k = 1;

    for (int len = 128; len >= 2; len >>= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = (int32_t)zetas_KEM[k++];
            uint32_t out0, out1;
            int j;

            for (j = start; j < start + len - 1; j += 2) {
                asm volatile (
                    ".insn r 0x6b, 0x7, 0x0, %[o0], %[a0], %[b0], %[z]\n"
                    ".insn r 0x6b, 0x7, 0x0, %[o1], %[a1], %[b1], %[z]\n"
                    : [o0] "=r" (out0), [o1] "=r" (out1)
                    : [a0] "r" ((int32_t)r->coeffs[j]),       [b0] "r" ((int32_t)r->coeffs[j+len]),
                      [a1] "r" ((int32_t)r->coeffs[j+1]),     [b1] "r" ((int32_t)r->coeffs[j+1+len]),
                      [z]  "r" (zeta)
                    : "cc"
                );

                r->coeffs[j]       = (int16_t)(out0 & 0xFFFF);
                r->coeffs[j+len]   = (int16_t)(out0 >> 16);
                r->coeffs[j+1]     = (int16_t)(out1 & 0xFFFF);
                r->coeffs[j+1+len] = (int16_t)(out1 >> 16);
            }

            for (; j < start + len; j++) {
                asm volatile (
                    ".insn r 0x6b, 0x7, 0x0, %[o0], %[a0], %[b0], %[z]\n"
                    : [o0] "=r" (out0)
                    : [a0] "r" ((int32_t)r->coeffs[j]), [b0] "r" ((int32_t)r->coeffs[j+len]),
                      [z]  "r" (zeta)
                    : "cc"
                );

                r->coeffs[j]     = (int16_t)(out0 & 0xFFFF);
                r->coeffs[j+len] = (int16_t)(out0 >> 16);
            }
        }
    }
}


static int kyber_ntt_hw_verify(const kyber_poly *r) {
    int ok = 1;
    for (int i = 0; i < 256; i++) {
        ok &= compare_coeffs("K-NTT-HW", i, r->coeffs[i], kyber_tv_ntt_out.coeffs[i]);
    }
    if (ok) printf("PASSED: Kyber NTT (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    kyber_poly r_sw, r_hw, r_hw_x4;

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    
#if SW_TEST_ENABLED
    r_sw = kyber_tv_ntt_in;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    kyber_ntt_sw_compute(&r_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= kyber_ntt_sw_verify(&r_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    r_hw = kyber_tv_ntt_in;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    kyber_ntt_hw_compute(&r_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= kyber_ntt_hw_verify(&r_hw);
    printf("Hardware (x2) Cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    printf("SW Cycles: %u | HW x2 Cycles: %u\n", cycles_sw, cycles_hw);
    if (cycles_sw > 0) {
        printf("Speedup x2: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return EXIT_SUCCESS;
}
