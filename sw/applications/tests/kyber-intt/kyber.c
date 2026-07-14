#include "kyber.h"


static inline int16_t montgomery_reduce(int32_t a) {
    int16_t t = (int16_t)a * KEM_QINV;
    return (a - (int32_t)t * KEM_Q) >> 16;
}

int32_t kyber_barrett_reduce(int32_t a) {
    const int32_t v = ((1 << 26) + KEM_Q/2) / KEM_Q;
    int32_t t = ((int32_t)v * a + (1 << 25)) >> 26;
    t *= KEM_Q;
    return a - t;
}

int16_t fqmul(int16_t a, int16_t b) {
    return montgomery_reduce((int32_t)a * b);
}

void kyber_ntt(int16_t r[256]) {
    unsigned int len, start, j, k = 1;
    int16_t t, zeta;
    for (len = 128; len >= 2; len >>= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = zetas_KEM[k++];
            for (j = start; j < start + len; j++) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
}

void kyber_invntt(int16_t r[256]) {
    unsigned int start, len, j, k = 127;
    int16_t t, zeta;
    const int16_t f = 1441; 
    for (len = 2; len <= 128; len <<= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = zetas_KEM[k--];
            for (j = start; j < start + len; j++) {
                t = r[j];
                r[j] = kyber_barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    for (j = 0; j < 256; j++) r[j] = fqmul(r[j], f);
}