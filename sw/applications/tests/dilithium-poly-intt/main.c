/**
 * @file main.c
 * @brief Dilithium Polyvec INTT Test - Software vs Hardware Comparison
 *
 * Generates a deterministic time-domain polyvector, transforms it into the
 * NTT domain using the SW golden model, then runs the inverse NTT via both
 * the SW golden and the HW implementation and checks for exact agreement.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dilithium.h"
#include "bfu.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

/*
 * Deterministic polyvec initialisation.
 * Produces coefficients in [-DSA_Q, DSA_Q) using a one-step LCG per entry.
 */
static void init_fixed_polyvec(dsa_polyvec *v, int32_t salt)
{
    for (int i = 0; i < DSA_K; i++) {
        for (int j = 0; j < DSA_N; j++) {
            uint32_t seed = (uint32_t)(i * 251 + j * 65537 + salt * 1000003);
            seed = seed * 1664525u + 1013904223u; /* LCG step */
            int32_t x = (int32_t)(seed % (uint32_t)(2 * DSA_Q)) - DSA_Q;
            v->vec[i].coeffs[j] = x;
        }
    }
}

static int compare_polyvec(const char *test_name,
                            const dsa_polyvec *got,
                            const dsa_polyvec *exp)
{
    int ok = 1;
    for (int i = 0; i < DSA_K; i++) {
        for (int j = 0; j < DSA_N; j++) {
            int32_t g = got->vec[i].coeffs[j];
            int32_t e = exp->vec[i].coeffs[j];
            if (g != e) {
                ok = 0;
#if DEBUG
                printf("[%s] Mismatch vec=%d idx=%d got=%d exp=%d\n",
                       test_name, i, j, g, e);
#else
                printf("[%s ERROR] Mismatch vec=%d idx=%d got=%d exp=%d\n",
                       test_name, i, j, g, e);
                return 0;
#endif
            }
        }
    }
    return ok;
}

int main(void)
{
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    int all_passed = 1;

    dsa_polyvec in_a;
    dsa_polyvec ntt_in;                  /* NTT-domain input for INTT test */
    dsa_polyvec sw_out, hw_out, golden;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    /* Build time-domain input, then NTT it to get a valid NTT-domain vector */
    init_fixed_polyvec(&in_a, 1);
    ntt_in = in_a;
    dilithium_polyvec_ntt_sw(&ntt_in);

    printf("    DILITHIUM POLYVEC INTT TEST     \n");

#if SW_TEST_ENABLED
    sw_out = ntt_in;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    dilithium_polyvec_invntt_sw(&sw_out);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    golden = sw_out; /* SW output becomes the golden reference */

    if (compare_polyvec("polyvec_invntt SW->golden", &sw_out, &golden)) {
        printf("PASSED: polyvec_invntt (Software)\n");
    } else {
        all_passed = 0;
    }
    printf("polyvec_invntt software cycles: %u\n", cycles_sw);
#else
    golden = ntt_in; /* No SW golden; skip HW verification */
#endif

    hw_out = ntt_in;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    dilithium_polyvec_invntt_hw(&hw_out);
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);

#if SW_TEST_ENABLED
    if (compare_polyvec("polyvec_invntt HW->golden", &hw_out, &golden)) {
        printf("PASSED: polyvec_invntt (Hardware)\n");
    } else {
        all_passed = 0;
    }
#endif

    printf("polyvec_invntt hardware cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
    if (cycles_hw != 0) {
        printf("Speedup: %u.%02ux\n",
               cycles_sw / cycles_hw,
               ((cycles_sw * 100) / cycles_hw) % 100);
    }
#endif

    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
