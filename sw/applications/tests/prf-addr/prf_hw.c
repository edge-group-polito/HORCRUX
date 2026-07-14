/*
 * Hardware-accelerated prf_addr implementation using OP_PRF_ADDR
 *
 * This file provides HW-optimized prf_addr for SPHINCS+ (FIPS 205 compliant)
 * using the OP_PRF_ADDR instruction that performs the complete PRF operation
 * in hardware (single keccak permutation).
 * 
 * Computes: SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 */

#include "prf_hw.h"
#include <string.h>

/* ========================================================================
 * Hardware instruction intrinsics (from keccak_coproc.S)
 * ======================================================================== */

/* Load a pair of 32-bit words into horcrux register file at index */
extern void cus_load(uint32_t lo, uint32_t hi, uint32_t index);

/* Store (read) a 32-bit word from horcrux register file at index */
extern uint32_t cus_store(uint32_t index);

/* Execute OP_PRF_ADDR: performs PRF using pub_seed, addr, and sk_seed (FIPS 205)
 * Input: reg[0:3]=pub_seed (16 bytes), reg[4:11]=addr (32 bytes), reg[12:15]=sk_seed (16 bytes)
 * Output: reg[0:3]=PRF result (16 bytes)
 * Computes: SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 */
extern void prf_addr_hw_compute(void);

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
 * prf_addr HW implementation using OP_PRF_ADDR
 * ======================================================================== */

/*
 * prf_addr_hw: Hardware-accelerated PRF using OP_PRF_ADDR (FIPS 205)
 * 
 * Computes: SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 */
void prf_addr_hw(uint8_t *out, const spx_ctx *ctx, const uint8_t *addr) {
    uint32_t i;
    
    /* Load pub_seed to reg[0:3] (16 bytes = 4 words) */
    cus_load(load32(ctx->pub_seed + 0), load32(ctx->pub_seed + 4), 0);
    cus_load(load32(ctx->pub_seed + 8), load32(ctx->pub_seed + 12), 2);
    
    /* Load addr to reg[4:11] (32 bytes = 8 words) */
    for (i = 0; i < 8; i += 2) {
        cus_load(load32(addr + 4*i), load32(addr + 4*(i+1)), 4 + i);
    }
    
    /* Load sk_seed to reg[12:15] (16 bytes = 4 words) */
    cus_load(load32(ctx->sk_seed + 0), load32(ctx->sk_seed + 4), 12);
    cus_load(load32(ctx->sk_seed + 8), load32(ctx->sk_seed + 12), 14);
    
    /* Memory fence before triggering compute */
    __asm__ volatile("fence" ::: "memory");
    
    /* Execute OP_PRF_ADDR */
    prf_addr_hw_compute();
    
    /* Read result from reg[0:3] */
    for (i = 0; i < 4; i++) {
        uint32_t w = cus_store(i);
        store32(out + 4*i, w);
    }
}
