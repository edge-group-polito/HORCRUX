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

/* 256-level robust thash: 2 Keccak permutations with bitmask XOR */
extern void thash1_256_hw_compute(void);
extern void thash2_256_hw_compute(void);

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

static void thash_hw_multiblock(unsigned char *out, const unsigned char *in, unsigned int inblocks,
                                 const spx_ctx *ctx, uint32_t addr[8])
{
    SPX_VLA(uint8_t, buf, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
    SPX_VLA(uint8_t, bitmask, inblocks * SPX_N);
    unsigned int i;

    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);

    shake256(bitmask, inblocks * SPX_N, buf, SPX_N + SPX_ADDR_BYTES);

    for (i = 0; i < inblocks * SPX_N; i++) {
        buf[SPX_N + SPX_ADDR_BYTES + i] = in[i] ^ bitmask[i];
    }

    shake256(out, SPX_N, buf, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
}

/* ========================================================================
 * Hardware-accelerated thash (256-level)
 * Register layout: pub_seed=reg[0:7], addr=reg[8:15],
 *   thash1 input=reg[16:23] (32B), thash2 input=reg[16:31] (64B),
 *   result=reg[0:7] (32B)
 * ======================================================================== */

void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8])
{
    uint32_t i;
    const uint8_t *addr_bytes = (const uint8_t *)addr;

    if (inblocks == 1) {
        /* Load pub_seed to reg[0:7] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(ctx->pub_seed + 4*i),
                     load32(ctx->pub_seed + 4*(i+1)), i);
        }

        /* Load addr to reg[8:15] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(addr_bytes + 4*i),
                     load32(addr_bytes + 4*(i+1)), 8 + i);
        }

        /* Load input to reg[16:23] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(in + 4*i),
                     load32(in + 4*(i+1)), 16 + i);
        }

        __asm__ volatile("fence" ::: "memory");
        thash1_256_hw_compute();

        for (i = 0; i < 8; i++) {
            uint32_t w = cus_store(i);
            store32(out + 4*i, w);
        }
        return;
    }

    if (inblocks == 2) {
        /* Load pub_seed to reg[0:7] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(ctx->pub_seed + 4*i),
                     load32(ctx->pub_seed + 4*(i+1)), i);
        }

        /* Load addr to reg[8:15] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(addr_bytes + 4*i),
                     load32(addr_bytes + 4*(i+1)), 8 + i);
        }

        /* Load input to reg[16:31] (64 bytes = 16 words) */
        for (i = 0; i < 16; i += 2) {
            cus_load(load32(in + 4*i),
                     load32(in + 4*(i+1)), 16 + i);
        }

        __asm__ volatile("fence" ::: "memory");
        thash2_256_hw_compute();

        for (i = 0; i < 8; i++) {
            uint32_t w = cus_store(i);
            store32(out + 4*i, w);
        }
        return;
    }

    /* Fallback to SW robust shake256 for inblocks > 2 */
    thash_hw_multiblock(out, in, inblocks, ctx, addr);
}

