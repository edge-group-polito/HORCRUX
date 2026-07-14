#ifndef FALCON_H
#define FALCON_H

#include <stdint.h>

#define FALCON_Q     12289
#define FALCON_QINV  53247 // Standard Montgomery inverse: Q * QINV mod 2^16 = 1
#define FALCON_Q0I   12287 // -Q^-1 mod 2^16 (used in your specific falcon snippet)
#define FALCON_R     4091  // R = 2^16 mod q
#define FALCON_R2    10952 // R2 = 2^32 mod q

// Test vector size (logn = 4, N = 16)
#define FALCON_N     16

// Falcon arithmetic functions
uint32_t mq_montymul_sw(uint32_t x, uint32_t y);
uint32_t mq_add_sw(uint32_t x, uint32_t y);
uint32_t mq_sub_sw(uint32_t x, uint32_t y);
void mq_NTT_sw(uint16_t *a, unsigned logn);
void mq_iNTT_sw(uint16_t *a, unsigned logn);

extern const uint32_t falcon_source1[100];
extern const uint32_t falcon_source0[100];
extern const uint32_t falcon_golden[100];

// Twiddle factors for NTT (forward) - GMb table subset for logn=4 (N=16)
extern const uint16_t falcon_GMb_16[16];

// Twiddle factors for iNTT (inverse) - iGMb table subset for logn=4 (N=16)
extern const uint16_t falcon_iGMb_16[16];

// Test vectors for NTT/iNTT (N=16, logn=4)
extern const uint16_t falcon_ntt_input[FALCON_N];
extern const uint16_t falcon_ntt_output[FALCON_N];
extern const uint16_t falcon_intt_input[FALCON_N];
extern const uint16_t falcon_intt_output[FALCON_N];

#endif