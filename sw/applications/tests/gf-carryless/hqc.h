#ifndef HQC_H
#define HQC_H

#include <stdint.h>

#define NUM_HQC_TESTS 10
#define NTESTS_8BIT   10

typedef struct {
    uint64_t val[2]; 
} ref_out_t;

// Reference functions
void gf2_mul64_sw(uint64_t c[2], uint64_t a, uint64_t b);
void gf_carryless_mul_sw(uint8_t *c, uint8_t a, uint8_t b);

// 64-bit Test Vectors
extern const uint64_t hqc_a[NUM_HQC_TESTS];
extern const uint64_t hqc_b[NUM_HQC_TESTS];
extern const ref_out_t hqc_out_ref[NUM_HQC_TESTS];

// 8-bit Test Vectors
extern const uint8_t hqc_a_8bit[NTESTS_8BIT];
extern const uint8_t hqc_b_8bit[NTESTS_8BIT];
extern const uint8_t hqc_c0_expected[NTESTS_8BIT];
extern const uint8_t hqc_c1_expected[NTESTS_8BIT];

#endif