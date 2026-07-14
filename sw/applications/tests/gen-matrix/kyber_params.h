#ifndef KYBER_PARAMS_H
#define KYBER_PARAMS_H

/* Kyber-512 Parameters */
#define KYBER_N 256
#define KYBER_K 2
#define KYBER_Q 3329
#define KYBER_SYMBYTES 32
#define XOF_BLOCKBYTES 168

/* Polynomial types */
typedef struct {
    int16_t coeffs[KYBER_N];
} poly;

typedef struct {
    poly vec[KYBER_K];
} polyvec;

/* XOF State (SHAKE128-based) */
typedef struct {
    uint64_t s[25];
    unsigned int pos;
} xof_state;

#endif /* KYBER_PARAMS_H */
