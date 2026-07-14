/*
 * Hardware-accelerated thash implementation using OP_THASH1
 *
 * This file provides HW-optimized thash for SPHINCS+ using the new
 * OP_THASH1 instruction that performs the complete thash-robust
 * algorithm in hardware.
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

/* Execute OP_THASH1: performs thash robust for 1 block
 * Input: reg[0:3]=pub_seed, reg[4:11]=addr, reg[12:15]=input
 * Output: reg[0:3]=hash result
 */
extern void thash1_hw_compute(void);

/* Existing keccak functions for fallback */
extern void     keccak_hw_init(void);
extern void     keccak_hw_absorb_xor(uint32_t lo, uint32_t hi, uint32_t index);
extern void     keccak_hw_permute(void);
extern uint32_t keccak_hw_store_word(uint32_t index);

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
 * thash HW implementation using OP_THASH1
 * ======================================================================== */

/*
 * thash_hw: Hardware-accelerated thash using OP_THASH1
 *
 * For inblocks=1, uses dedicated OP_THASH1 instruction.
 * For inblocks>1, falls back to separate keccak operations.
 */
void thash_hw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr) {
    uint32_t i;
    
    if (inblocks == 1) {
        /* Use OP_THASH1 for single-block case */
        
        /* Load pub_seed to reg[0:3] (16 bytes = 4 words) */
        cus_load(load32(ctx->pub_seed + 0), load32(ctx->pub_seed + 4), 0);
        cus_load(load32(ctx->pub_seed + 8), load32(ctx->pub_seed + 12), 2);
        
        /* Load addr to reg[4:11] (32 bytes = 8 words) */
        for (i = 0; i < 8; i += 2) {
            cus_load(load32(addr + 4*i), load32(addr + 4*(i+1)), 4 + i);
        }
        
        /* Load input to reg[12:15] (16 bytes = 4 words) */
        cus_load(load32(in + 0), load32(in + 4), 12);
        cus_load(load32(in + 8), load32(in + 12), 14);
        
        /* Memory fence before triggering compute */
        __asm__ volatile("fence" ::: "memory");
        
        /* Execute OP_THASH1 */
        thash1_hw_compute();
        
        /* Read result from reg[0:3] */
        for (i = 0; i < 4; i++) {
            uint32_t w = cus_store(i);
            store32(out + 4*i, w);
        }
    } else {
        /* Fallback for inblocks > 1: use existing keccak functions */
        /* This path is rarely used (mainly for FORS pk hash with 33 blocks) */
        
        /* For now, fall back to SW implementation for multi-block */
        /* A proper implementation would use multiple keccak operations */
        thash_sw(out, in, inblocks, ctx, addr);
    }
}

/*
 * thash_hw_chain_opt: Optimized for WOTS chain processing
 *
 * In a WOTS chain, pub_seed and most of addr stay constant.
 * Only hash_addr (byte 31) changes per iteration.
 *
 * Optimization: Keep pub_seed and addr in register file,
 * only reload input per thash call.
 */
void thash_hw_chain_opt(uint8_t *out, const uint8_t *in,
                        const spx_ctx *ctx, const uint8_t *addr) {
    /* For now, use the standard thash_hw */
    thash_hw(out, in, 1, ctx, addr);
}
