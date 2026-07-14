#include "dilithium.h"


static inline int32_t montgomery_reduce_dsa(int64_t a) {
    int32_t t = (int32_t)a * DSA_QINV;
    return (a - (int64_t)t * DSA_Q) >> 32;
}

int32_t reduce32(int32_t a) {
    int32_t t;
    t = (a + (1 << 22)) >> 23;
    t = a - t * DSA_Q;
    return t;
}

int32_t caddq(int32_t a) {
    a += (a >> 31) & DSA_Q;
    return a;
}

void dilithium_ntt(int32_t a[DSA_N]) {
    unsigned int len, start, j, k = 0;
    int32_t zeta, t;
    for (len = 128; len > 0; len >>= 1) {
        for (start = 0; start < DSA_N; start = j + len) {
            zeta = zetas_DSA[++k];
            for (j = start; j < start + len; ++j) {
                t = montgomery_reduce_dsa((int64_t)zeta * a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }
}

void dilithium_invntt(int32_t a[DSA_N]) {
    unsigned int start, len, j, k = 256;
    int32_t t, zeta;
    const int32_t f = 41978; 
    for (len = 1; len < DSA_N; len <<= 1) {
        for (start = 0; start < DSA_N; start = j + len) {
            zeta = -zetas_DSA[--k];
            for (j = start; j < start + len; ++j) {
                t = a[j];
                a[j] = t + a[j + len];
                a[j + len] = t - a[j + len];
                a[j + len] = montgomery_reduce_dsa((int64_t)zeta * a[j + len]);
            }
        }
    }
    for (j = 0; j < DSA_N; ++j) a[j] = montgomery_reduce_dsa((int64_t)f * a[j]);
}