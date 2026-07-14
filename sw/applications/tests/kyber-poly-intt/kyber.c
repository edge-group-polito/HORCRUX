#include "kyber.h"
#include "bfu.h"

const int16_t zetas_KEM[128] = {
    -1044,  -758,  -359, -1517,  1493,  1422,   287,   202,
    -171,   622,  1577,   182,   962, -1202, -1474,  1468,
    573, -1325,   264,   383,  -829,  1458, -1602,  -130,
    -681,  1017,   732,   608, -1542,   411,  -205, -1571,
    1223,   652,  -552,  1015, -1293,  1491,  -282, -1544,
    516,    -8,  -320,  -666, -1618, -1162,   126,  1469,
    -853,   -90,  -271,   830,   107, -1421,  -247,  -951,
    -398,   961, -1508,  -725,   448, -1065,   677, -1275,
    -1103,   430,   555,   843, -1251,   871,  1550,   105,
    422,   587,   177,  -235,  -291,  -460,  1574,  1653,
    -246,   778,  1159,  -147,  -777,  1483,  -602,  1119,
    -1590,   644,  -872,   349,   418,   329,  -156,   -75,
    817,  1097,   603,   610,  1322, -1285, -1465,   384,
    -1215,  -136,  1218, -1335,  -874,   220, -1187, -1659,
    -1185, -1530, -1278,   794, -1510,  -854,  -870,   478,
    -108,  -308,   996,   991,   958, -1460,  1522,  1628
};

static inline int16_t montgomery_reduce(int32_t a)
{
    int16_t t = (int16_t)a * KEM_QINV;
    return (a - (int32_t)t * KEM_Q) >> 16;
}

int32_t kyber_barrett_reduce(int32_t a)
{
    const int32_t v = ((1 << 26) + KEM_Q / 2) / KEM_Q;
    int32_t t = ((int32_t)v * a + (1 << 25)) >> 26;
    t *= KEM_Q;
    return a - t;
}

int16_t fqmul(int16_t a, int16_t b)
{
    return montgomery_reduce((int32_t)a * b);
}

void kyber_ntt_sw(int16_t r[KEM_N])
{
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

void kyber_invntt_sw(int16_t r[KEM_N])
{
    unsigned int start, len, j, k = 127;
    int16_t t, zeta;
    const int16_t f = 1441;

    for (len = 2; len <= 128; len <<= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = zetas_KEM[k--];
            for (j = start; j < start + len; j++) {
                t = r[j];
                r[j] = (int16_t)kyber_barrett_reduce(t + r[j + len]);
                r[j + len] = r[j + len] - t;
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }

    for (j = 0; j < 256; j++) {
        r[j] = fqmul(r[j], f);
    }
}

void kyber_ntt_hw(int16_t r[KEM_N])
{
    int k = 1;

    for (int len = 128; len >= 2; len >>= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int16_t zeta = zetas_KEM[k++];
            int16_t *pj  = &r[start];
            int16_t *pjl = &r[start + len];
            int16_t *end = pj + len;

            while (pj < end) {
                uint32_t rs1 = (int32_t)*pj;
                uint32_t rs2 = (int32_t)*pjl;
                uint32_t rd;

                asm volatile (
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x0, %[rd], %[r1], t0, %[r3] \n"
                    : [rd] "=r" (rd)
                    : [r1] "r" (rs1), [r2] "r" (rs2), [r3] "r" (zeta)
                    : "cc", "t0"
                );

                *pj++  = (int16_t)(rd & 0xFFFF);
                *pjl++ = (int16_t)(rd >> 16);
            }
        }
    }
}

void kyber_invntt_hw(int16_t r[KEM_N])
{
    int k = 127;
    const int32_t f = 1441;

    for (int len = 2; len <= 64; len <<= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = (int32_t)zetas_KEM[k--];
            uint32_t out0, out1;
            int j;

            for (j = start; j < start + len - 1; j += 2) {
                asm volatile (
                    ".insn r 0x6b, 0x7, 0x1, %[o0], %[a0], %[b0], %[z]\n"
                    ".insn r 0x6b, 0x7, 0x1, %[o1], %[a1], %[b1], %[z]\n"
                    : [o0] "=r" (out0), [o1] "=r" (out1)
                    : [a0] "r" ((int32_t)r[j]),       [b0] "r" ((int32_t)r[j+len]),
                      [a1] "r" ((int32_t)r[j+1]),     [b1] "r" ((int32_t)r[j+1+len]),
                      [z]  "r" (zeta)
                    : "cc"
                );

                r[j]       = (int16_t)(out0 & 0xFFFF);
                r[j+len]   = (int16_t)(out0 >> 16);
                r[j+1]     = (int16_t)(out1 & 0xFFFF);
                r[j+1+len] = (int16_t)(out1 >> 16);
            }

            for (; j < start + len; j++) {
                asm volatile (
                    ".insn r 0x6b, 0x7, 0x1, %[o0], %[a0], %[b0], %[z]\n"
                    : [o0] "=r" (out0)
                    : [a0] "r" ((int32_t)r[j]), [b0] "r" ((int32_t)r[j+len]),
                      [z]  "r" (zeta)
                    : "cc"
                );

                r[j]     = (int16_t)(out0 & 0xFFFF);
                r[j+len] = (int16_t)(out0 >> 16);
            }
        }
    }

    {
        int32_t zeta = (int32_t)zetas_KEM[k--];
        for (int j = 0; j < 128; j += 2) {
            uint32_t bfu0, bfu1;
            int32_t s0, s1, s2, s3;

            asm volatile (
                ".insn r 0x6b, 0x7, 0x1, %[bf0], %[a0], %[b0], %[z]\n"
                ".insn r 0x6b, 0x7, 0x1, %[bf1], %[a1], %[b1], %[z]\n"
                : [bf0] "=&r" (bfu0), [bf1] "=&r" (bfu1)
                : [a0] "r" ((int32_t)r[j]),     [b0] "r" ((int32_t)r[j+128]),
                  [a1] "r" ((int32_t)r[j+1]),   [b1] "r" ((int32_t)r[j+1+128]),
                  [z]  "r" (zeta)
                : "cc"
            );

            asm volatile (
                ".insn r 0x3b, 0x7, 0x11, %[s0], %[lo0], %[f]\n"
                ".insn r 0x3b, 0x7, 0x11, %[s1], %[hi0], %[f]\n"
                ".insn r 0x3b, 0x7, 0x11, %[s2], %[lo1], %[f]\n"
                ".insn r 0x3b, 0x7, 0x11, %[s3], %[hi1], %[f]\n"
                : [s0] "=&r" (s0), [s1] "=&r" (s1), [s2] "=&r" (s2), [s3] "=&r" (s3)
                : [lo0] "r" ((int32_t)(int16_t)(bfu0 & 0xFFFF)),
                  [hi0] "r" ((int32_t)(int16_t)(bfu0 >> 16)),
                  [lo1] "r" ((int32_t)(int16_t)(bfu1 & 0xFFFF)),
                  [hi1] "r" ((int32_t)(int16_t)(bfu1 >> 16)),
                  [f]   "r" (f)
                : "cc"
            );

            r[j]       = (int16_t)s0;
            r[j+128]   = (int16_t)s1;
            r[j+1]     = (int16_t)s2;
            r[j+1+128] = (int16_t)s3;
        }
    }
}

void kyber_poly_add(kyber_poly *r, const kyber_poly *a, const kyber_poly *b)
{
    for (int i = 0; i < KEM_N; i++) {
        r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
    }
}

void kyber_poly_reduce_sw(kyber_poly *r)
{
    for (int i = 0; i < KEM_N; i++) {
        r->coeffs[i] = (int16_t)kyber_barrett_reduce(r->coeffs[i]);
    }
}

void kyber_poly_reduce_hw(kyber_poly *r)
{
    for (int i = 0; i < KEM_N; i++) {
        int32_t out;
        asm volatile (
            "addi t0, %1, 0\n\t"
            ".insn r 0x3b, 0x7, 5, %0, t0, x0\n\t"
            : "=&r"(out)
            : "r"((int32_t)r->coeffs[i])
            : "t0", "cc"
        );
        r->coeffs[i] = (int16_t)out;
    }
}

void kyber_polyvec_ntt_sw(kyber_polyvec *v)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_ntt_sw(v->vec[i].coeffs);
        kyber_poly_reduce_sw(&v->vec[i]);
    }
}

void kyber_polyvec_ntt_hw(kyber_polyvec *v)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_ntt_hw(v->vec[i].coeffs);
        kyber_poly_reduce_hw(&v->vec[i]);
    }
}

void kyber_polyvec_invntt_sw(kyber_polyvec *v)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_invntt_sw(v->vec[i].coeffs);
    }
}

void kyber_polyvec_invntt_hw(kyber_polyvec *v)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_invntt_hw(v->vec[i].coeffs);
    }
}

void kyber_polyvec_add_sw(kyber_polyvec *r, const kyber_polyvec *a, const kyber_polyvec *b)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
    }
}

void kyber_polyvec_add_hw(kyber_polyvec *r, const kyber_polyvec *a, const kyber_polyvec *b)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
    }
}

void kyber_polyvec_reduce_sw(kyber_polyvec *r)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_poly_reduce_sw(&r->vec[i]);
    }
}

void kyber_polyvec_reduce_hw(kyber_polyvec *r)
{
    for (int i = 0; i < KEM_K; i++) {
        kyber_poly_reduce_hw(&r->vec[i]);
    }
}
