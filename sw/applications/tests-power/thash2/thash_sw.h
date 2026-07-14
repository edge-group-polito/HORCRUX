#ifndef THASH_SW_H
#define THASH_SW_H

#include <stdint.h>
#include <stddef.h>
#include "test_params.h"

/*
 * Software reference implementation of SPHINCS+ thash (tweakable hash)
 * for shake256-robust variant with 2-block input.
 * 
 * thash robust construction:
 *   1. bitmask = SHAKE256(pub_seed || addr)[:inblocks*SPX_N]
 *   2. masked_input = input XOR bitmask
 *   3. output = SHAKE256(pub_seed || addr || masked_input)[:SPX_N]
 */

/* SPHINCS+ signing context */
typedef struct {
    uint8_t pub_seed[SPX_N];
    uint8_t sk_seed[SPX_N];
} spx_ctx;

/* Initialize context */
void spx_ctx_init(spx_ctx *ctx, const uint8_t *pub_seed, const uint8_t *sk_seed);

/* 
 * Software thash - tweakable hash function (robust variant)
 * 
 * out:      output hash (SPX_N bytes)
 * in:       input blocks (inblocks * SPX_N bytes)
 * inblocks: number of SPX_N-byte input blocks
 * ctx:      signing context with pub_seed
 * addr:     SPHINCS+ address (32 bytes)
 */
void thash_sw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr);

/* Address manipulation functions */
static inline void set_tree_height(uint8_t *addr, uint32_t height) {
    addr[SPX_OFFSET_TREE_HGT] = (uint8_t)height;
}

static inline void set_tree_index(uint8_t *addr, uint32_t index) {
    addr[SPX_OFFSET_TREE_INDEX + 0] = (uint8_t)(index >> 24);
    addr[SPX_OFFSET_TREE_INDEX + 1] = (uint8_t)(index >> 16);
    addr[SPX_OFFSET_TREE_INDEX + 2] = (uint8_t)(index >> 8);
    addr[SPX_OFFSET_TREE_INDEX + 3] = (uint8_t)(index);
}

static inline void set_type(uint8_t *addr, uint32_t type) {
    addr[SPX_OFFSET_TYPE] = (uint8_t)type;
}

static inline void copy_addr(uint8_t *out, const uint8_t *in) {
    for (int i = 0; i < SPX_ADDR_BYTES; i++) {
        out[i] = in[i];
    }
}

#endif /* THASH_SW_H */
