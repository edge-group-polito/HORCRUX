#include <stdint.h>
#include <string.h>

#include "thash.h"
#include "address.h"
#include "params.h"
#include "utils.h"

#include "fips202.h"

/* ========================================================================
 * Hardware instruction intrinsics (from keccak_coproc.S)
 * ======================================================================== */

extern void cus_load(uint32_t lo, uint32_t hi, uint32_t index);
extern uint32_t cus_store(uint32_t index);

/* 128-level simple thash: single Keccak permutation, no bitmask */
extern void thash1_simple_hw_compute(void);
extern void thash2_simple_hw_compute(void);

/* ========================================================================
 * Helper functions
 * ======================================================================== */

static uint32_t load32(const uint8_t *x) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

static void store32(uint8_t *x, uint32_t v) {
    x[0] = (uint8_t)(v);
    x[1] = (uint8_t)(v >> 8);
    x[2] = (uint8_t)(v >> 16);
    x[3] = (uint8_t)(v >> 24);
}

/**
 * Takes an array of inblocks concatenated arrays of SPX_N bytes.
 * Uses HW acceleration for 1-block and 2-block cases (simple thash).
 */
void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8])
{
    uint32_t i;
    const uint8_t *addr_bytes = (const uint8_t *)addr;

    if (inblocks == 1) {
        /* Load pub_seed to reg[0:3] (16 bytes = 4 words) */
        cus_load(load32(ctx->pub_seed + 0), load32(ctx->pub_seed + 4), 0);
        cus_load(load32(ctx->pub_seed + 8), load32(ctx->pub_seed + 12), 2);

        /* Load addr to reg[4:11] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(addr_bytes + 4*i), load32(addr_bytes + 4*(i+1)), 4 + i);
        }

        /* Load input to reg[12:15] (16 bytes = 4 words) */
        cus_load(load32(in + 0), load32(in + 4), 12);
        cus_load(load32(in + 8), load32(in + 12), 14);

        __asm__ volatile("fence" ::: "memory");
        thash1_simple_hw_compute();

        for (i = 0; i < 4; i++) {
            uint32_t w = cus_store(i);
            store32(out + 4*i, w);
        }
        return;
    }

    if (inblocks == 2) {
        /* Load pub_seed to reg[0:3] (16 bytes = 4 words) */
        cus_load(load32(ctx->pub_seed + 0), load32(ctx->pub_seed + 4), 0);
        cus_load(load32(ctx->pub_seed + 8), load32(ctx->pub_seed + 12), 2);

        /* Load addr to reg[4:11] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(addr_bytes + 4*i), load32(addr_bytes + 4*(i+1)), 4 + i);
        }

        /* Load input to reg[12:19] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(in + 4*i), load32(in + 4*(i+1)), 12 + i);
        }

        __asm__ volatile("fence" ::: "memory");
        thash2_simple_hw_compute();

        for (i = 0; i < 4; i++) {
            uint32_t w = cus_store(i);
            store32(out + 4*i, w);
        }
        return;
    }

    /* Fallback to HW-accelerated SHAKE256 for inblocks > 2 */
    {
        SPX_VLA(uint8_t, buf, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);

        memcpy(buf, ctx->pub_seed, SPX_N);
        memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);
        memcpy(buf + SPX_N + SPX_ADDR_BYTES, in, inblocks * SPX_N);

        shake256(out, SPX_N, buf, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
    }
}
