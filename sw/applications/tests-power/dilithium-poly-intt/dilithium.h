#ifndef DILITHIUM_POLY_INTT_H
#define DILITHIUM_POLY_INTT_H

#include <stdint.h>

#define DSA_N    256
#define DSA_K    4
#define DSA_Q    8380417
#define DSA_QINV 58728449

typedef struct {
    int32_t coeffs[DSA_N];
} dsa_poly;

typedef struct {
    dsa_poly vec[DSA_K];
} dsa_polyvec;

extern const int32_t zetas_DSA[DSA_N];

int32_t dilithium_reduce32(int32_t a);

/* Single-polynomial transforms */
void dilithium_ntt_sw(int32_t a[DSA_N]);
void dilithium_invntt_sw(int32_t a[DSA_N]);
void dilithium_ntt_hw(int32_t a[DSA_N]);
void dilithium_invntt_hw(int32_t a[DSA_N]);

/* Polyvector transforms */
void dilithium_polyvec_ntt_sw(dsa_polyvec *v);
void dilithium_polyvec_ntt_hw(dsa_polyvec *v);
void dilithium_polyvec_invntt_sw(dsa_polyvec *v);
void dilithium_polyvec_invntt_hw(dsa_polyvec *v);

#endif
