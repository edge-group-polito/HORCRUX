#include "dilithium.h"
#include "bfu.h"

const int32_t zetas_DSA[DSA_N] = {
         0,    25847, -2608894,  -518909,   237124,  -777960,  -876248,   466468,
   1826347,  2353451,  -359251, -2091905,  3119733, -2884855,  3111497,  2680103,
   2725464,  1024112, -1079900,  3585928,  -549488, -1119584,  2619752, -2108549,
  -2118186, -3859737, -1399561, -3277672,  1757237,   -19422,  4010497,   280005,
   2706023,    95776,  3077325,  3530437, -1661693, -3592148, -2537516,  3915439,
  -3861115, -3043716,  3574422, -2867647,  3539968,  -300467,  2348700,  -539299,
  -1699267, -1643818,  3505694, -3821735,  3507263, -2140649, -1600420,  3699596,
    811944,   531354,   954230,  3881043,  3900724, -2556880,  2071892, -2797779,
  -3930395, -1528703, -3677745, -3041255, -1452451,  3475950,  2176455, -1585221,
  -1257611,  1939314, -4083598, -1000202, -3190144, -3157330, -3632928,   126922,
   3412210,  -983419,  2147896,  2715295, -2967645, -3693493,  -411027, -2477047,
   -671102, -1228525,   -22981, -1308169,  -381987,  1349076,  1852771, -1430430,
  -3343383,   264944,   508951,  3097992,    44288, -1100098,   904516,  3958618,
  -3724342,    -8578,  1653064, -3249728,  2389356,  -210977,   759969, -1316856,
    189548, -3553272,  3159746, -1851402, -2409325,  -177440,  1315589,  1341330,
   1285669, -1584928,  -812732, -1439742, -3019102, -3881060, -3628969,  3839961,
   2091667,  3407706,  2316500,  3817976, -3342478,  2244091, -2446433, -3562462,
    266997,  2434439, -1235728,  3513181, -3520352, -3759364, -1197226, -3193378,
    900702,  1859098,   909542,   819034,   495491, -1613174,   -43260,  -522500,
   -655327, -3122442,  2031748,  3207046, -3556995,  -525098,  -768622, -3595838,
    342297,   286988, -2437823,  4108315,  3437287, -3342277,  1735879,   203044,
   2842341,  2691481, -2590150,  1265009,  4055324,  1247620,  2486353,  1595974,
  -3767016,  1250494,  2635921, -3548272, -2994039,  1869119,  1903435, -1050970,
  -1333058,  1237275, -3318210, -1430225,  -451100,  1312455,  3306115, -1962642,
  -1279661,  1917081, -2546312, -1374803,  1500165,   777191,  2235880,  3406031,
   -542412, -2831860, -1671176, -1846953, -2584293, -3724270,   594136, -3776993,
  -2013608,  2432395,  2454455,  -164721,  1957272,  3369112,   185531, -1207385,
  -3183426,   162844,  1616392,  3014001,   810149,  1652634, -3694233, -1799107,
  -3038916,  3523897,  3866901,   269760,  2213111,  -975884,  1717735,   472078,
   -426683,  1723600, -1803090,  1910376, -1667432, -1104333,  -260646, -3833893,
  -2939036, -2235985,  -420899, -2286327,   183443,  -976891,  1612842, -3545687,
   -554416,  3919660,   -48306, -1362209,  3937738,  1400424,  -846154,  1976782
};

static inline int32_t montgomery_reduce_dsa(int64_t a)
{
    int32_t t = (int32_t)a * DSA_QINV;
    return (int32_t)((a - (int64_t)t * DSA_Q) >> 32);
}

int32_t dilithium_reduce32(int32_t a)
{
    int32_t t = (a + (1 << 22)) >> 23;
    return a - t * DSA_Q;
}

/*************************************************
 * Software forward NTT (Cooley-Tukey, bitreversed output)
 *************************************************/
void dilithium_ntt_sw(int32_t a[DSA_N])
{
    unsigned int len, start, j, k = 0;
    int32_t zeta, t;

    for (len = 128; len > 0; len >>= 1) {
        for (start = 0; start < DSA_N; start = j + len) {
            zeta = zetas_DSA[++k];
            for (j = start; j < start + len; ++j) {
                t = montgomery_reduce_dsa((int64_t)zeta * a[j + len]);
                a[j + len] = a[j] - t;
                a[j]       = a[j] + t;
            }
        }
    }
}

/*************************************************
 * Software inverse NTT (Gentleman-Sande, includes final scaling)
 *************************************************/
void dilithium_invntt_sw(int32_t a[DSA_N])
{
    unsigned int start, len, j, k = 256;
    int32_t t, zeta;
    const int32_t f = 41978; /* mont^2/256 */

    for (len = 1; len < DSA_N; len <<= 1) {
        for (start = 0; start < DSA_N; start = j + len) {
            zeta = -zetas_DSA[--k];
            for (j = start; j < start + len; ++j) {
                t          = a[j];
                a[j]       = t + a[j + len];
                a[j + len] = t - a[j + len];
                a[j + len] = montgomery_reduce_dsa((int64_t)zeta * a[j + len]);
            }
        }
    }

    for (j = 0; j < DSA_N; ++j)
        a[j] = montgomery_reduce_dsa((int64_t)f * a[j]);
}

/*************************************************
 * Hardware forward NTT — 2x unrolled BFNTTD butterfly
 * Uses custom instructions: BFNTTD (0x6b,0x7,0x2) + RDRDHI (0x3b,0x7,0x17)
 *************************************************/
void dilithium_ntt_hw(int32_t a[DSA_N])
{
    int k = 0;
    bfu_result_t out0, out1;

    for (int len = 128; len > 0; len >>= 1) {
        for (int start = 0; start < DSA_N; start = start + 2 * len) {
            int32_t zeta = zetas_DSA[++k];
            int j;

            /* 2x unrolled butterfly */
            for (j = start; j < start + len - 1; j += 2) {
                asm volatile (
                    "addi t0, %[b0], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd0_lo], %[a0], t0, %[z] \n"
                    ".insn r 0x3b, 0x7, 0x17, %[rd0_hi], x0, x0 \n"
                    "addi t0, %[b1], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd1_lo], %[a1], t0, %[z] \n"
                    ".insn r 0x3b, 0x7, 0x17, %[rd1_hi], x0, x0 \n"
                    : [rd0_lo] "=&r" (out0.rd_lo), [rd0_hi] "=&r" (out0.rd_hi),
                      [rd1_lo] "=&r" (out1.rd_lo), [rd1_hi] "=&r" (out1.rd_hi)
                    : [a0] "r" ((uint32_t)a[j]),       [b0] "r" ((uint32_t)a[j + len]),
                      [a1] "r" ((uint32_t)a[j + 1]),   [b1] "r" ((uint32_t)a[j + 1 + len]),
                      [z]  "r" (zeta)
                    : "cc", "t0"
                );
                a[j]           = (int32_t)out0.rd_lo;
                a[j + len]     = (int32_t)out0.rd_hi;
                a[j + 1]       = (int32_t)out1.rd_lo;
                a[j + 1 + len] = (int32_t)out1.rd_hi;
            }

            /* Tail: handles odd remainder (e.g. len=1 stage) */
            for (; j < start + len; ++j) {
                asm volatile (
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x2,  %[rd1], %[r1], t0, %[r3] \n"
                    ".insn r 0x3b, 0x7, 0x17, %[rd2], x0, x0 \n"
                    : [rd1] "=&r" (out0.rd_lo), [rd2] "=&r" (out0.rd_hi)
                    : [r1] "r" ((uint32_t)a[j]),
                      [r2] "r" ((uint32_t)a[j + len]),
                      [r3] "r" (zeta)
                    : "cc", "t0"
                );
                a[j]       = (int32_t)out0.rd_lo;
                a[j + len] = (int32_t)out0.rd_hi;
            }
        }
    }
}

/*************************************************
 * Hardware inverse NTT — 2x unrolled BFINTTD butterfly + final MQMULD scaling
 * Uses custom instructions: BFINTTD (0x6b,0x7,0x3) + RDRDHI2 (0x3b,0x7,0x18)
 *                           MQMULD  (0x3b,0x7,0x12)
 *************************************************/
void dilithium_invntt_hw(int32_t a[DSA_N])
{
    int k = 256;
    const int32_t f = 41978;
    bfu_result_t out0, out1;

    for (int len = 1; len < DSA_N; len <<= 1) {
        for (int start = 0; start < DSA_N; start = start + 2 * len) {
            int32_t zeta = -zetas_DSA[--k];
            int j;

            /* 2x unrolled INTT butterfly */
            for (j = start; j < start + len - 1; j += 2) {
                asm volatile (
                    "addi t0, %[b0], 0\n"
                    ".insn r 0x6b, 0x7, 0x3,  %[rd0_lo], %[a0], t0, %[z] \n"
                    ".insn r 0x3b, 0x7, 0x18, %[rd0_hi], x0, x0 \n"
                    "addi t0, %[b1], 0\n"
                    ".insn r 0x6b, 0x7, 0x3,  %[rd1_lo], %[a1], t0, %[z] \n"
                    ".insn r 0x3b, 0x7, 0x18, %[rd1_hi], x0, x0 \n"
                    : [rd0_lo] "=&r" (out0.rd_lo), [rd0_hi] "=&r" (out0.rd_hi),
                      [rd1_lo] "=&r" (out1.rd_lo), [rd1_hi] "=&r" (out1.rd_hi)
                    : [a0] "r" ((uint32_t)a[j]),       [b0] "r" ((uint32_t)a[j + len]),
                      [a1] "r" ((uint32_t)a[j + 1]),   [b1] "r" ((uint32_t)a[j + 1 + len]),
                      [z]  "r" (zeta)
                    : "cc", "t0"
                );
                a[j]           = (int32_t)out0.rd_lo;
                a[j + len]     = (int32_t)out0.rd_hi;
                a[j + 1]       = (int32_t)out1.rd_lo;
                a[j + 1 + len] = (int32_t)out1.rd_hi;
            }

            /* Tail */
            for (; j < start + len; ++j) {
                asm volatile (
                    "addi t0, %[r2], 0\n"
                    ".insn r 0x6b, 0x7, 0x3,  %[rd1], %[r1], t0, %[r3] \n"
                    ".insn r 0x3b, 0x7, 0x18, %[rd2], x0, x0 \n"
                    : [rd1] "=&r" (out0.rd_lo), [rd2] "=&r" (out0.rd_hi)
                    : [r1] "r" ((uint32_t)a[j]),
                      [r2] "r" ((uint32_t)a[j + len]),
                      [r3] "r" (zeta)
                    : "cc", "t0"
                );
                a[j]       = (int32_t)out0.rd_lo;
                a[j + len] = (int32_t)out0.rd_hi;
            }
        }
    }

    /* Final scaling by f = mont^2/256 */
    for (int j = 0; j < DSA_N; ++j) {
        bfu_result_t out;
        asm volatile (
            "addi t0, %[r1], 0\n"
            ".insn r 0x3b, 0x7, 0x12, %[rd], t0, %[r2] \n"
            : [rd] "=r" (out.rd_lo)
            : [r1] "r" ((uint32_t)a[j]), [r2] "r" (f)
            : "cc", "t0"
        );
        a[j] = (int32_t)out.rd_lo;
    }
}

/*************************************************
 * Polyvector wrappers
 *************************************************/
void dilithium_polyvec_ntt_sw(dsa_polyvec *v)
{
    for (int i = 0; i < DSA_K; i++)
        dilithium_ntt_sw(v->vec[i].coeffs);
}

void dilithium_polyvec_ntt_hw(dsa_polyvec *v)
{
    for (int i = 0; i < DSA_K; i++)
        dilithium_ntt_hw(v->vec[i].coeffs);
}

void dilithium_polyvec_invntt_sw(dsa_polyvec *v)
{
    for (int i = 0; i < DSA_K; i++)
        dilithium_invntt_sw(v->vec[i].coeffs);
}

void dilithium_polyvec_invntt_hw(dsa_polyvec *v)
{
    for (int i = 0; i < DSA_K; i++)
        dilithium_invntt_hw(v->vec[i].coeffs);
}
