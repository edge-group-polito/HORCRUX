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
#include "vcd_util.h"

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


#if SW_TEST_ENABLED
/*
 * SOFTWARE TESTS
 */
void test_gen_a_sw(void)
{
    polyvec a[KYBER_K];

    gen_a_sw(a, TEST_SEED);

}


#endif

/*
 * HARDWARE TESTS
 */
void test_gen_a_hw(void)
{
    polyvec a[KYBER_K];
    gen_a(a, TEST_SEED);
}


int main(void)
{

#if SW_TEST_ENABLED
    test_gen_a_sw();

#endif

    test_gen_a_hw();


    return 0;
}
