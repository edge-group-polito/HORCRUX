/**
 * @file main.c
 * @brief Kyber gen_matrix Test - Software vs Hardware Comparison
 * 
 * Tests Kyber matrix generation (gen_a and gen_at) using both software
 * and hardware (Keccak coprocessor) implementations.
 * gen_matrix uses SHAKE128 XOF for rejection sampling.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fips202.h"
#include "kyber_params.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

/* Forward declarations */
extern void gen_matrix(polyvec *a, const uint8_t seed[KYBER_SYMBYTES], int transposed);
extern void gen_a(polyvec *a, const uint8_t seed[KYBER_SYMBYTES]);
extern void gen_at(polyvec *a, const uint8_t seed[KYBER_SYMBYTES]);

/* Software implementations (pure SW Keccak) */
extern void gen_a_sw(polyvec *a, const uint8_t seed[KYBER_SYMBYTES]);
extern void gen_at_sw(polyvec *a, const uint8_t seed[KYBER_SYMBYTES]);

/* Test result tracking */
static int total_tests = 0;
static int passed_tests = 0;

/* Performance tracking */
static uint32_t sw_cycles = 0;
static uint32_t hw_cycles = 0;

/*
 * Helper: Compare two matrix arrays (each with KYBER_K polyvecs)
 */
static int compare_polyvec(const polyvec *a, const polyvec *b)
{
    for(int k = 0; k < KYBER_K; k++) {
        for(int i = 0; i < KYBER_K; i++) {
            for(int j = 0; j < KYBER_N; j++) {
                if(a[k].vec[i].coeffs[j] != b[k].vec[i].coeffs[j]) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/*
 * Fixed test seed: standard NIST test vector pattern
 */
static const uint8_t TEST_SEED[KYBER_SYMBYTES] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

/*
 * Golden reference values for TEST_SEED
 */
typedef struct {
    int16_t a00_first4[4];
    int16_t a01_first4[4];
    int16_t a10_first4[4];
    int16_t a11_first4[4];
} golden_matrix_t;

static const golden_matrix_t GOLDEN_GEN_A = {
    .a00_first4 = {1364, 199, 712, 506},
    .a01_first4 = {483, 1189, 1089, 2233},
    .a10_first4 = {536, 843, 1994, 2664},
    .a11_first4 = {314, 1867, 3116, 2581}
};

static const golden_matrix_t GOLDEN_GEN_AT = {
    .a00_first4 = {1364, 199, 712, 506},
    .a01_first4 = {536, 843, 1994, 2664},
    .a10_first4 = {483, 1189, 1089, 2233},
    .a11_first4 = {314, 1867, 3116, 2581}
};

/*
 * Helper: Verify polynomial vector against golden reference
 */
static int verify_polyvec_golden(const polyvec *a, const golden_matrix_t *golden)
{
    for(int i = 0; i < 4; i++) {
        if(a[0].vec[0].coeffs[i] != golden->a00_first4[i]) return 0;
    }
    for(int i = 0; i < 4; i++) {
        if(a[0].vec[1].coeffs[i] != golden->a01_first4[i]) return 0;
    }
    for(int i = 0; i < 4; i++) {
        if(a[1].vec[0].coeffs[i] != golden->a10_first4[i]) return 0;
    }
    for(int i = 0; i < 4; i++) {
        if(a[1].vec[1].coeffs[i] != golden->a11_first4[i]) return 0;
    }
    return 1;
}

#if SW_TEST_ENABLED
/*
 * SOFTWARE TESTS
 */
void test_gen_a_sw(void)
{
    polyvec a[KYBER_K];
    uint32_t cycles = 0;

    printf("Test: gen_a (SW)\n");
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gen_a_sw(a, TEST_SEED);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    sw_cycles = cycles;
    printf("  Cycles: %u\n", cycles);

    total_tests++;
    if(verify_polyvec_golden(a, &GOLDEN_GEN_A)) {
        passed_tests++;
        printf("  gen_a SW: PASS\n");
    } else {
        printf("  gen_a SW: FAIL\n");
        printf("  Got: ");
        for(int i = 0; i < 4; i++) printf("%d ", a[0].vec[0].coeffs[i]);
        printf("\n  Expected: 1364 199 712 506\n");
    }
}

void test_gen_at_sw(void)
{
    polyvec at[KYBER_K];
    uint32_t cycles = 0;

    printf("Test: gen_at (SW)\n");
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gen_at_sw(at, TEST_SEED);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);

    total_tests++;
    if(verify_polyvec_golden(at, &GOLDEN_GEN_AT)) {
        passed_tests++;
        printf("  gen_at SW: PASS\n");
    } else {
        printf("  gen_at SW: FAIL\n");
    }
}
#endif

/*
 * HARDWARE TESTS
 */
void test_gen_a_hw(void)
{
    polyvec a[KYBER_K];
    uint32_t cycles = 0;

    printf("Test: gen_a (HW)\n");
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gen_a(a, TEST_SEED);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    hw_cycles = cycles;
    printf("  Cycles: %u\n", cycles);

    total_tests++;
    if(verify_polyvec_golden(a, &GOLDEN_GEN_A)) {
        passed_tests++;
        printf("  gen_a HW: PASS\n");
    } else {
        printf("  gen_a HW: FAIL\n");
        printf("  Got: ");
        for(int i = 0; i < 4; i++) printf("%d ", a[0].vec[0].coeffs[i]);
        printf("\n  Expected: 1364 199 712 506\n");
    }
}

void test_gen_at_hw(void)
{
    polyvec at[KYBER_K];
    uint32_t cycles = 0;

    printf("Test: gen_at (HW)\n");
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gen_at(at, TEST_SEED);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);

    total_tests++;
    if(verify_polyvec_golden(at, &GOLDEN_GEN_AT)) {
        passed_tests++;
        printf("  gen_at HW: PASS\n");
    } else {
        printf("  gen_at HW: FAIL\n");
    }
}

void test_deterministic_hw(void)
{
    polyvec a1[KYBER_K], a2[KYBER_K], a3[KYBER_K];

    printf("Test: Deterministic seeding (HW)\n");
    gen_a(a1, TEST_SEED);
    gen_a(a2, TEST_SEED);
    gen_a(a3, TEST_SEED);

    total_tests++;
    if(compare_polyvec(a1, a2) && compare_polyvec(a2, a3)) {
        passed_tests++;
        printf("  Deterministic: PASS\n");
    } else {
        printf("  Deterministic: FAIL\n");
    }
}

int main(void)
{
    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   KYBER GEN_MATRIX TEST             \n");
    printf("KYBER_K=%d, KYBER_N=%d\n\n", KYBER_K, KYBER_N);

#if SW_TEST_ENABLED
    test_gen_a_sw();
    test_gen_at_sw();
#endif

    printf("\n--- HARDWARE TESTS ---\n");
    test_gen_a_hw();
    test_gen_at_hw();
    test_deterministic_hw();

#if SW_TEST_ENABLED
    if(sw_cycles > 0 && hw_cycles > 0) {
        uint32_t cycles_sw = sw_cycles;
        uint32_t cycles_hw = hw_cycles;
        printf("\n--- PERFORMANCE SUMMARY ---\n");
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        if (cycles_sw > 0) {
            printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
        }
    }
#endif

    printf("  Tests: %d/%d passed\n", passed_tests, total_tests);

    if (passed_tests == total_tests) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return EXIT_SUCCESS;
}
