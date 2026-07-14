#include <stdint.h>
#include "params.h"
#include "ntt.h"
#include "reduce.h"

/* Code to generate zetas and zetas_inv used in the number-theoretic transform:

#define KYBER_ROOT_OF_UNITY 17

static const uint8_t tree[128] = {
  0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120,
  4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
  2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
  6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
  1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
  5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
  3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
  7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127
};

void init_ntt() {
  unsigned int i;
  int16_t tmp[128];

  tmp[0] = MONT;
  for(i=1;i<128;i++)
    tmp[i] = fqmul(tmp[i-1],MONT*KYBER_ROOT_OF_UNITY % KYBER_Q);

  for(i=0;i<128;i++) {
    zetas[i] = tmp[tree[i]];
    if(zetas[i] > KYBER_Q/2)
      zetas[i] -= KYBER_Q;
    if(zetas[i] < -KYBER_Q/2)
      zetas[i] += KYBER_Q;
  }
}
*/

const int16_t zetas[128] = {
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

/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*              using MQMULK custom instruction
*
* Arguments:   - int16_t a: first factor
*              - int16_t b: second factor
*
* Returns 16-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static int16_t fqmul(int16_t a, int16_t b) {
  int32_t t;
  asm volatile (
        ".insn r 0x3b, 0x7, 0x11, %[rd], %[r1], %[r2] \n"
        : [rd] "=&r" (t)
        : [r1] "r" ((int32_t)a), [r2] "r" ((int32_t)b)
        : "cc"
    );
  return (int16_t)t;
}


/*************************************************
* Name:        ntt
*
* Description: Inplace number-theoretic transform (NTT) in Rq.
*              input is in standard order, output is in bitreversed order
*              Uses BFNTTK custom instruction for butterflies
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void ntt(int16_t r[256]) {
    int k = 1;
    for (int len = 128; len >= 2; len >>= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = (int32_t)zetas[k++];
            uint32_t out0, out1;
            int j;
            for (j = start; j < start + len - 1; j += 2) {
                /* Butterfly pair: process j and j+1 together (2x unrolled) */
                asm volatile (
                    ".insn r 0x6b, 0x7, 0x0, %[o0], %[a0], %[b0], %[z]\n"
                    ".insn r 0x6b, 0x7, 0x0, %[o1], %[a1], %[b1], %[z]\n"
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
            /* Handle odd remainder (when len is odd - never for Kyber, but safe) */
            for (; j < start + len; j++) {
                asm volatile (
                    ".insn r 0x6b, 0x7, 0x0, %[o0], %[a0], %[b0], %[z]\n"
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
}

/*************************************************
* Name:        invntt_tomont
*
* Description: Inplace inverse number-theoretic transform in Rq and
*              multiplication by Montgomery factor 2^16.
*              Input is in bitreversed order, output is in standard order
*              Uses BFINTTK custom instruction for butterflies and
*              MQMULK for final scaling
*
* Arguments:   - int16_t r[256]: pointer to input/output vector of elements of Zq
**************************************************/
void invntt(int16_t r[256]) {
    int k = 127;
    const int32_t f = 1441; /* mont^2/128 */

    /* Stages len = 2, 4, ..., 64 (normal INTT butterflies) */
    for (int len = 2; len <= 64; len <<= 1) {
        for (int start = 0; start < 256; start = start + 2 * len) {
            int32_t zeta = (int32_t)zetas[k--];
            uint32_t out0, out1;
            int j;
            for (j = start; j < start + len - 1; j += 2) {
                /* 2x unrolled INTT butterfly */
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

    /* Last stage (len=128): INTT butterfly + merged fqmul scaling by f=1441 */
    {
        int32_t zeta = (int32_t)zetas[k--];
        for (int j = 0; j < 128; j += 2) {
            uint32_t bfu0, bfu1;
            int32_t s0, s1, s2, s3;
            asm volatile (
                /* Two INTT butterflies */
                ".insn r 0x6b, 0x7, 0x1, %[bf0], %[a0], %[b0], %[z]\n"
                ".insn r 0x6b, 0x7, 0x1, %[bf1], %[a1], %[b1], %[z]\n"
                : [bf0] "=&r" (bfu0), [bf1] "=&r" (bfu1)
                : [a0] "r" ((int32_t)r[j]),     [b0] "r" ((int32_t)r[j+128]),
                  [a1] "r" ((int32_t)r[j+1]),   [b1] "r" ((int32_t)r[j+1+128]),
                  [z]  "r" (zeta)
                : "cc"
            );
            /* Extract butterfly results and immediately scale by f using MQMULK */
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
            r[j]     = (int16_t)s0;
            r[j+128] = (int16_t)s1;
            r[j+1]     = (int16_t)s2;
            r[j+1+128] = (int16_t)s3;
        }
    }
}

/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^2-zeta)
*              used for multiplication of elements in Rq in NTT domain
*              Uses MQMULK custom instruction
*
* Arguments:   - int16_t r[2]: pointer to the output polynomial
*              - const int16_t a[2]: pointer to the first factor
*              - const int16_t b[2]: pointer to the second factor
*              - int16_t zeta: integer defining the reduction polynomial
**************************************************/
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta)
{
  /* r[0] = fqmul(fqmul(a[1], b[1]), zeta) + fqmul(a[0], b[0])
   * r[1] = fqmul(a[0], b[1]) + fqmul(a[1], b[0]) */
  asm volatile (
      ".insn r 0x3b, 0x7, 0x11, t1, %[ra1], %[rb1] \n"   /* t1 = fqmul(a1, b1) */
      ".insn r 0x3b, 0x7, 0x11, t3, %[ra0], %[rb0] \n"   /* t3 = fqmul(a0, b0) */
      ".insn r 0x3b, 0x7, 0x11, t2, t1, %[zeta] \n"      /* t2 = fqmul(t1, zeta) */
      "add %[rd0], t2, t3 \n"                             /* r[0] = t2 + t3 */
      ".insn r 0x3b, 0x7, 0x11, t1, %[ra0], %[rb1] \n"   /* t1 = fqmul(a0, b1) */
      ".insn r 0x3b, 0x7, 0x11, t2, %[ra1], %[rb0] \n"   /* t2 = fqmul(a1, b0) */
      "add %[rd1], t1, t2 \n"                             /* r[1] = t1 + t2 */
      : [rd0] "=&r" (r[0]), [rd1] "=&r" (r[1])
      : [ra1] "r" ((int32_t)a[1]), [rb1] "r" ((int32_t)b[1]),
        [zeta] "r" ((int32_t)zeta),
        [ra0] "r" ((int32_t)a[0]), [rb0] "r" ((int32_t)b[0])
      : "cc", "t1", "t2", "t3"
  );
}
