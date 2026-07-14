#ifndef KYBER_POLY_NTT_H
#define KYBER_POLY_NTT_H

#include <stdint.h>

#define KEM_K 2
#define KEM_N 256
#define KEM_Q 3329
#define KEM_QINV 62209

typedef struct {
    int16_t coeffs[KEM_N];
} kyber_poly;

typedef struct {
    kyber_poly vec[KEM_K];
} kyber_polyvec;

extern const int16_t zetas_KEM[128];

int32_t kyber_barrett_reduce(int32_t a);
int16_t fqmul(int16_t a, int16_t b);

void kyber_ntt_sw(int16_t r[KEM_N]);
void kyber_invntt_sw(int16_t r[KEM_N]);
void kyber_ntt_hw(int16_t r[KEM_N]);
void kyber_invntt_hw(int16_t r[KEM_N]);

void kyber_poly_add(kyber_poly *r, const kyber_poly *a, const kyber_poly *b);
void kyber_poly_reduce_sw(kyber_poly *r);
void kyber_poly_reduce_hw(kyber_poly *r);

void kyber_polyvec_ntt_sw(kyber_polyvec *v);
void kyber_polyvec_ntt_hw(kyber_polyvec *v);
void kyber_polyvec_invntt_sw(kyber_polyvec *v);
void kyber_polyvec_invntt_hw(kyber_polyvec *v);
void kyber_polyvec_add_sw(kyber_polyvec *r, const kyber_polyvec *a, const kyber_polyvec *b);
void kyber_polyvec_add_hw(kyber_polyvec *r, const kyber_polyvec *a, const kyber_polyvec *b);
void kyber_polyvec_reduce_sw(kyber_polyvec *r);
void kyber_polyvec_reduce_hw(kyber_polyvec *r);

#endif
