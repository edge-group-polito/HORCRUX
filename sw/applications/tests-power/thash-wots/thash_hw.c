/*
 * Hardware-accelerated thash implementation using HORCRUX Keccak coprocessor
 *
 * This file provides HW-optimized versions of thash for SPHINCS+.
 * Uses HW-managed Keccak state for improved performance.
 */

#include "thash_hw.h"
#include <string.h>

/* ========================================================================
 * Hardware Keccak coprocessor functions (from keccak_coproc.S)
 * ======================================================================== */

extern void     keccak_hw_init(void);
extern void     keccak_hw_absorb_xor(uint32_t lo, uint32_t hi, uint32_t index);
extern void     keccak_hw_permute(void);
extern uint32_t keccak_hw_store_word(uint32_t index);
extern uint32_t keccak_hw_read3(uint32_t byte_offset);

/* ========================================================================
 * Helper functions
 * ======================================================================== */

#define SHAKE256_RATE 136

static uint32_t load32(const uint8_t *x) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

/*
 * HW-managed SHAKE256: absorb + pad entirely in HW state
 */
static void shake256_hw_absorb_once(unsigned int rate,
                                     const uint8_t *in, size_t inlen,
                                     uint8_t domain_sep) {
    unsigned int i;
    unsigned int rate_words = rate / 4;
    
    /* Zero HW state */
    keccak_hw_init();
    
    /* Absorb full blocks */
    while (inlen >= rate) {
        for (i = 0; i < rate_words; i += 2) {
            keccak_hw_absorb_xor(load32(in + 4*i), load32(in + 4*(i+1)), i);
        }
        in += rate;
        inlen -= rate;
        keccak_hw_permute();
    }
    
    /* Absorb remaining bytes + padding */
    /* For simplicity, handle byte-by-byte for tail */
    uint8_t temp[SHAKE256_RATE] = {0};
    memcpy(temp, in, inlen);
    temp[inlen] = domain_sep;  /* SHAKE256: 0x1F */
    temp[rate - 1] ^= 0x80;    /* Final bit */
    
    for (i = 0; i < rate_words; i += 2) {
        keccak_hw_absorb_xor(load32(temp + 4*i), load32(temp + 4*(i+1)), i);
    }
}

/*
 * Squeeze output from HW state using READ3 for arbitrary byte offsets
 */
static void shake256_hw_squeeze(uint8_t *out, size_t outlen) {
    size_t i;
    
    /* Use keccak_hw_read3 for efficient 3-byte reads */
    for (i = 0; i + 3 <= outlen; i += 3) {
        uint32_t val = keccak_hw_read3(i);
        out[i]   = (uint8_t)(val);
        out[i+1] = (uint8_t)(val >> 8);
        out[i+2] = (uint8_t)(val >> 16);
    }
    
    /* Handle remaining bytes */
    while (i < outlen) {
        /* Fall back to word reads for remaining bytes */
        uint32_t word_idx = i / 4;
        uint32_t byte_off = i % 4;
        uint32_t val = keccak_hw_store_word(word_idx);
        out[i] = (uint8_t)(val >> (byte_off * 8));
        i++;
    }
}

/*
 * Full HW SHAKE256 one-shot
 */
static void shake256_hw(uint8_t *output, size_t outlen,
                        const uint8_t *input, size_t inlen) {
    shake256_hw_absorb_once(SHAKE256_RATE, input, inlen, 0x1F);
    keccak_hw_permute();
    shake256_hw_squeeze(output, outlen);
}

/* ========================================================================
 * thash HW implementations
 * ======================================================================== */

/*
 * thash_hw: Hardware-accelerated thash (robust variant)
 *
 * Same algorithm as SW:
 *   1. bitmask = SHAKE256(pub_seed || addr)[:inblocks*SPX_N]
 *   2. masked = input XOR bitmask
 *   3. output = SHAKE256(pub_seed || addr || masked)[:SPX_N]
 *
 * Optimization: Uses HW Keccak for both SHAKE256 calls
 */
void thash_hw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr) {
    uint8_t buf[SPX_N + SPX_ADDR_BYTES + SPX_WOTS_LEN * SPX_N];
    uint8_t bitmask[SPX_WOTS_LEN * SPX_N];
    size_t i;
    size_t inlen = inblocks * SPX_N;
    
    /* Build prefix: pub_seed || addr */
    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);
    
    /* Step 1: Generate bitmask using HW SHAKE256 */
    shake256_hw(bitmask, inlen, buf, SPX_N + SPX_ADDR_BYTES);
    
    /* Step 2: XOR input with bitmask */
    for (i = 0; i < inlen; i++) {
        buf[SPX_N + SPX_ADDR_BYTES + i] = in[i] ^ bitmask[i];
    }
    
    /* Step 3: Generate output using HW SHAKE256 */
    shake256_hw(out, SPX_N, buf, SPX_N + SPX_ADDR_BYTES + inlen);
}

/*
 * thash_hw_chain_opt: Optimized for WOTS chain processing
 *
 * In a chain, each thash call has:
 *   - Same pub_seed (constant)
 *   - Same addr except hash_addr field changes (0,1,2...)
 *
 * Optimization opportunity:
 *   - The bitmask generation for pub_seed||addr[0..30] prefix could be
 *     partially precomputed. However, since hash_addr is at byte 31,
 *     it's part of the absorb block, so we can't fully reuse state.
 *
 * Current implementation: Same as thash_hw
 * Future: Could batch multiple thash calls for better locality
 */
void thash_hw_chain_opt(uint8_t *out, const uint8_t *in,
                        const spx_ctx *ctx, const uint8_t *addr,
                        int first_in_chain) {
    (void)first_in_chain;  /* Reserved for future optimization */
    thash_hw(out, in, 1, ctx, addr);
}
