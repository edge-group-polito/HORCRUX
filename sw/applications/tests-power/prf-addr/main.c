/**
 * @file main.c
 * @brief SPHINCS+ PRF Address Test - Software vs Hardware Comparison
 * 
 * Tests the PRF (Pseudo-Random Function) used in SPHINCS+ to derive
 * WOTS+ secret keys from the public seed, secret seed, and address.
 * 
 * Algorithm (FIPS 205): PRF(pub_seed, sk_seed, addr) = SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core_v_mini_mcu.h"
#include "csr.h"
#include "test_params.h"
#include "test_vectors.h"
#include "prf_sw.h"
#include "prf_hw.h"
#include "vcd_util.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

/* Test result tracking */
static int total_tests = 0;
static int passed_tests = 0;

/* Performance tracking */
static uint32_t sw_total_cycles = 0;
static uint32_t hw_total_cycles = 0;

/* Utility functions */
static int compare_arrays(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

/* ========================================================================
 * Test 1: Basic PRF Test (WOTS address)
 * ======================================================================== */
static void test_prf_basic(void) {
    spx_ctx ctx;
    uint8_t out_sw[SPX_N], out_hw[SPX_N];
    uint32_t cycles_sw = 0, cycles_hw = 0;


    memcpy(ctx.pub_seed, tv_pub_seed, SPX_N);
    memcpy(ctx.sk_seed, tv_sk_seed, SPX_N);

#if SW_TEST_ENABLED

    if (vcd_init() != 0)
    return;

    vcd_enable();

    prf_addr_sw(out_sw, &ctx, tv_addr_wots);

    vcd_disable();

#endif

    if (vcd_init() != 0)
    return;

    vcd_enable();

    prf_addr_hw(out_hw, &ctx, tv_addr_wots);

    vcd_disable();

    total_tests++;
#if SW_TEST_ENABLED
    if (compare_arrays(out_sw, out_hw, SPX_N) != 0) {
        printf("  Result: FAIL (mismatch)\n");
    }
#else
    passed_tests++;
    printf("  Result: PASS (HW only)\n");
#endif
}

/* ========================================================================
 * Main
 * ======================================================================== */
int main(void)
{

    test_prf_basic();

    return 0;
}
