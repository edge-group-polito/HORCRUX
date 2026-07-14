/*
 * Hardware-accelerated thash implementation using OP_THASH2
 *
 * This file provides HW-optimized thash for SPHINCS+ using the new
 * OP_THASH2 instruction that performs the complete thash-robust
 * algorithm for 2-block input in hardware.
 */

#include "thash_hw.h"
#include <string.h>

/* ========================================================================
 * Hardware instruction intrinsics (from keccak_coproc.S)
 * ======================================================================== */

/* Load a pair of 32-bit words into horcrux register file at index */
extern void cus_load(uint32_t lo, uint32_t hi, uint32_t index);

/* Store (read) a 32-bit word from horcrux register file at index */
extern uint32_t cus_store(uint32_t index);

/* Execute OP_THASH2: performs thash robust for 2 blocks
 * Input: reg[0:3]=pub_seed, reg[4:11]=addr, reg[12:19]=input (32 bytes)
 * Output: reg[0:3]=hash result
 */
extern void thash2_hw_compute(void);

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

/* ========================================================================
 * thash HW implementation using OP_THASH2
 * ======================================================================== */

/*
 * thash_hw: Hardware-accelerated thash using OP_THASH2
 *
 * For inblocks=2, uses dedicated OP_THASH2 instruction.
 * For other cases, falls back to software implementation.
 */
void thash_hw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr) {
    uint32_t i;
    
    if (inblocks == 2) {
        /* Use OP_THASH2 for 2-block case */
        
        /* Load pub_seed to reg[0:3] (16 bytes = 4 words) */
        cus_load(load32(ctx->pub_seed + 0), load32(ctx->pub_seed + 4), 0);
        cus_load(load32(ctx->pub_seed + 8), load32(ctx->pub_seed + 12), 2);
        
        /* Load addr to reg[4:11] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(addr + 4*i), load32(addr + 4*(i+1)), 4 + i);
        }
        
        /* Load input to reg[12:19] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(in + 4*i), load32(in + 4*(i+1)), 12 + i);
        }
        
        /* Memory fence before triggering compute */
        __asm__ volatile("fence" ::: "memory");
        
        /* Execute OP_THASH2 */
        thash2_hw_compute();
        
        /* Read result from reg[0:3] */
        for (i = 0; i < 4; i++) {
            uint32_t w = cus_store(i);
            store32(out + 4*i, w);
        }
    } else {
        /* Fallback for other cases */
        thash_sw(out, in, inblocks, ctx, addr);
    }
}
