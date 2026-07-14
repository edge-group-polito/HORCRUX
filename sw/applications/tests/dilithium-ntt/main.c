/**
 * @file main.c
 * @brief Dilithium NTT Test - Software vs Hardware Comparison
 * 
 * Tests the Number Theoretic Transform (NTT) operation for Dilithium
 * digital signature algorithm using both software and hardware implementations.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "dilithium.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"


// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
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

static void dilithium_ntt_sw_compute(dsa_poly *a) {
    dilithium_ntt(a->coeffs);
}

static int dilithium_ntt_sw_verify(const dsa_poly *a) {
    int ok = 1;
    for (int i = 0; i < DSA_N; i++) {
        ok &= compare_coeffs("D-NTT-SW", i, a->coeffs[i], dsa_tv_ntt_out.coeffs[i]);
    }
    if (ok) printf("PASSED: Dilithium NTT (Software)\n");
    return ok;
}

static void dilithium_ntt_hw_compute(dsa_poly *a) {
    int k = 0;

    for (int len = 128; len > 0; len >>= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = zetas_DSA[++k];
            bfu_result_t out0, out1;
            int j;
            for (j = start; j < start + len - 1; j += 2) {
                asm volatile ( 
                    "addi t0, %[b0], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd0_lo], %[a0], t0, %[z] \n"
                    ".insn r 0x3b, 0x7, 0x17, %[rd0_hi], x0, x0 \n"
                    "addi t0, %[b1], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd1_lo], %[a1], t0, %[z] \n"
                    ".insn r 0x3b, 0x7, 0x17, %[rd1_hi], x0, x0 \n"
                    : [rd0_lo] "=&r" (out0.rd_lo), [rd0_hi] "=&r" (out0.rd_hi),
                        [rd1_lo] "=&r" (out1.rd_lo), [rd1_hi] "=&r" (out1.rd_hi)
                    : [a0] "r" ((uint32_t)a->coeffs[j]),       [b0] "r" ((uint32_t)a->coeffs[j+len]),
                        [a1] "r" ((uint32_t)a->coeffs[j+1]),     [b1] "r" ((uint32_t)a->coeffs[j+1+len]),
                        [z]  "r" (zeta)
                    : "cc", "t0" 
                );
                a->coeffs[j]       = (int32_t)out0.rd_lo;
                a->coeffs[j+len]   = (int32_t)out0.rd_hi;
                a->coeffs[j+1]     = (int32_t)out1.rd_lo;
                a->coeffs[j+1+len] = (int32_t)out1.rd_hi;
            }

            for (; j < start + len; ++j) {
            asm volatile (
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd1], %[r1], t0, %[r3] \n"
                    ".insn r 0x3b, 0x7, 0x17, %[rd2], x0, x0 \n"
                    : [rd1] "=&r" (out0.rd_lo), [rd2] "=&r" (out0.rd_hi)
                    : [r1] "r" ((uint32_t)a->coeffs[j]),
                        [r2] "r" ((uint32_t)a->coeffs[j+len]),
                        [r3] "r" (zeta)
                    : "cc", "t0"
            );
            a->coeffs[j]     = (int32_t)out0.rd_lo;
            a->coeffs[j+len] = (int32_t)out0.rd_hi;
    }
        }
    }
}

static int dilithium_ntt_hw_verify(const dsa_poly *a) {
    int ok = 1;
    for (int i = 0; i < 256; i++) {
        ok &= compare_coeffs("D-NTT-HW", i, a->coeffs[i], dsa_tv_ntt_out.coeffs[i]);
    }
    if (ok) printf("PASSED: Dilithium NTT (Hardware)\n");
    return ok;
}

int main(void) {
    int all_passed = 1;
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    dsa_poly a_sw, a_hw;

    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("    DILITHIUM NTT TEST               \n");

#if SW_TEST_ENABLED
    a_sw = dsa_tv_ntt_in;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    dilithium_ntt_sw_compute(&a_sw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    all_passed &= dilithium_ntt_sw_verify(&a_sw);
    printf("Software Cycles: %u\n\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    a_hw = dsa_tv_ntt_in;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    dilithium_ntt_hw_compute(&a_hw);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    all_passed &= dilithium_ntt_hw_verify(&a_hw);
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
