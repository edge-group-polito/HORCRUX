#include <stdint.h>
#include <string.h>

#include "address.h"
#include "utils.h"
#include "params.h"
#include "hash.h"
#include "fips202.h"

/* ========================================================================
 * Hardware instruction intrinsics (from keccak_coproc.S)
 * ======================================================================== */

/* Load a pair of 32-bit words into horcrux register file at index */
extern void cus_load(uint32_t lo, uint32_t hi, uint32_t index);

/* Store (read) a 32-bit word from horcrux register file at index */
extern uint32_t cus_store(uint32_t index);

/* Execute OP_PRF_256: performs PRF at 256-level
 * Input: reg[0:7]=pub_seed (32 bytes), reg[8:15]=addr (32 bytes), reg[16:23]=sk_seed (32 bytes)
 * Output: reg[0:7]=PRF result (32 bytes)
 */
extern void prf_256_hw_compute(void);

/* ========================================================================
 * Helper functions
 * ======================================================================== */

static uint32_t load32_prf(const uint8_t *x) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

static void store32_prf(uint8_t *x, uint32_t v) {
    x[0] = (uint8_t)(v);
    x[1] = (uint8_t)(v >> 8);
    x[2] = (uint8_t)(v >> 16);
    x[3] = (uint8_t)(v >> 24);
}

/* For SHAKE256, there is no immediate reason to initialize at the start,
   so this function is an empty operation. */
void initialize_hash_function(spx_ctx* ctx)
{
    (void)ctx; /* Suppress an 'unused parameter' warning. */
}

/*
 * Computes PRF(pk_seed, sk_seed, addr) = SHAKE256(pub_seed || addr || sk_seed)
 * 
 * Now uses hardware-accelerated OP_PRF_ADDR which is FIPS 205 compliant.
 */
void prf_addr(unsigned char *out, const spx_ctx *ctx,
              const uint32_t addr[8])
{
    uint32_t i;
    const uint8_t *addr_bytes = (const uint8_t *)addr;
    
    /* Load pub_seed to reg[0:7] (32 bytes = 8 words) */
    for (i = 0; i < 8; i += 2) {
        cus_load(load32_prf(ctx->pub_seed + 4*i),
                 load32_prf(ctx->pub_seed + 4*(i+1)), i);
    }
    
    /* Load addr to reg[8:15] (32 bytes = 8 words) */
    for (i = 0; i < 8; i += 2) {
        cus_load(load32_prf(addr_bytes + 4*i),
                 load32_prf(addr_bytes + 4*(i+1)), 8 + i);
    }
    
    /* Load sk_seed to reg[16:23] (32 bytes = 8 words) */
    for (i = 0; i < 8; i += 2) {
        cus_load(load32_prf(ctx->sk_seed + 4*i),
                 load32_prf(ctx->sk_seed + 4*(i+1)), 16 + i);
    }
    
    /* Memory fence before triggering compute */
    __asm__ volatile("fence" ::: "memory");
    
    /* Execute OP_PRF_256 */
    prf_256_hw_compute();
    
    /* Read result from reg[0:7] (32 bytes = 8 words) */
    for (i = 0; i < 8; i++) {
        uint32_t w = cus_store(i);
        store32_prf(out + 4*i, w);
    }
}

/**
 * Computes the message-dependent randomness R, using a secret seed and an
 * optional randomization value as well as the message.
 */
void gen_message_random(unsigned char *R, const unsigned char *sk_prf,
                        const unsigned char *optrand,
                        const unsigned char *m, unsigned long long mlen,
                        const spx_ctx *ctx)
{
    (void)ctx;
    uint64_t s_inc[26];

    shake256_inc_init(s_inc);
    shake256_inc_absorb(s_inc, sk_prf, SPX_N);
    shake256_inc_absorb(s_inc, optrand, SPX_N);
    shake256_inc_absorb(s_inc, m, mlen);
    shake256_inc_finalize(s_inc);
    shake256_inc_squeeze(R, SPX_N, s_inc);
}

/**
 * Computes the message hash using R, the public key, and the message.
 * Outputs the message digest and the index of the leaf. The index is split in
 * the tree index and the leaf index, for convenient copying to an address.
 */
void hash_message(unsigned char *digest, uint64_t *tree, uint32_t *leaf_idx,
                  const unsigned char *R, const unsigned char *pk,
                  const unsigned char *m, unsigned long long mlen,
                  const spx_ctx *ctx)
{
    (void)ctx;
#define SPX_TREE_BITS (SPX_TREE_HEIGHT * (SPX_D - 1))
#define SPX_TREE_BYTES ((SPX_TREE_BITS + 7) / 8)
#define SPX_LEAF_BITS SPX_TREE_HEIGHT
#define SPX_LEAF_BYTES ((SPX_LEAF_BITS + 7) / 8)
#define SPX_DGST_BYTES (SPX_FORS_MSG_BYTES + SPX_TREE_BYTES + SPX_LEAF_BYTES)

    unsigned char buf[SPX_DGST_BYTES];
    unsigned char *bufp = buf;
    uint64_t s_inc[26];

    shake256_inc_init(s_inc);
    shake256_inc_absorb(s_inc, R, SPX_N);
    shake256_inc_absorb(s_inc, pk, SPX_PK_BYTES);
    shake256_inc_absorb(s_inc, m, mlen);
    shake256_inc_finalize(s_inc);
    shake256_inc_squeeze(buf, SPX_DGST_BYTES, s_inc);

    memcpy(digest, bufp, SPX_FORS_MSG_BYTES);
    bufp += SPX_FORS_MSG_BYTES;

#if SPX_TREE_BITS > 64
    #error For given height and depth, 64 bits cannot represent all subtrees
#endif

    if (SPX_D == 1) {
        *tree = 0;
    } else {
        *tree = bytes_to_ull(bufp, SPX_TREE_BYTES);
        *tree &= (~(uint64_t)0) >> (64 - SPX_TREE_BITS);
    }
    bufp += SPX_TREE_BYTES;

    *leaf_idx = (uint32_t)bytes_to_ull(bufp, SPX_LEAF_BYTES);
    *leaf_idx &= (~(uint32_t)0) >> (32 - SPX_LEAF_BITS);
}
