#ifndef PRF_SW_H
#define PRF_SW_H

#include <stdint.h>
#include <stddef.h>
#include "test_params.h"

/*
 * Software reference implementation of SPHINCS+ prf_addr
 * for shake256 variant.
 * 
 * prf_addr(sk_seed, addr) = SHAKE256(sk_seed || addr)[0:SPX_N]
 */

/* SPHINCS+ signing context */
typedef struct {
    uint8_t pub_seed[SPX_N];
    uint8_t sk_seed[SPX_N];
} spx_ctx;

/* Initialize context */
void spx_ctx_init(spx_ctx *ctx, const uint8_t *pub_seed, const uint8_t *sk_seed);

/* 
 * Software prf_addr - pseudo-random function
 * 
 * out:   output hash (SPX_N bytes)
 * ctx:   signing context with sk_seed
 * addr:  SPHINCS+ address (32 bytes)
 */
void prf_addr_sw(uint8_t *out, const spx_ctx *ctx, const uint8_t *addr);

/* Address manipulation functions */
static inline void set_chain_addr(uint8_t *addr, uint32_t chain) {
    addr[SPX_OFFSET_CHAIN_ADDR] = (uint8_t)chain;
}

static inline void set_hash_addr(uint8_t *addr, uint32_t hash) {
    addr[SPX_OFFSET_HASH_ADDR] = (uint8_t)hash;
}

static inline void set_keypair_addr(uint8_t *addr, uint32_t keypair) {
    addr[SPX_OFFSET_KP_ADDR2] = (uint8_t)(keypair >> 8);
    addr[SPX_OFFSET_KP_ADDR1] = (uint8_t)(keypair >> 16);
    addr[SPX_OFFSET_KP_ADDR2 + 1] = (uint8_t)keypair;
}

static inline void copy_addr(uint8_t *out, const uint8_t *in) {
    for (int i = 0; i < SPX_ADDR_BYTES; i++) {
        out[i] = in[i];
    }
}

#endif /* PRF_SW_H */
