#ifndef THASH_HW_H
#define THASH_HW_H

#include <stdint.h>
#include <stddef.h>
#include "thash_sw.h"  /* Reuse spx_ctx and address functions */

/*
 * Hardware-accelerated thash implementation using HORCRUX Keccak coprocessor
 * 
 * Optimizations:
 * 1. Multi-buffer SHAKE256 using HW-managed state
 * 2. HW XOR operation for bitmask application  
 * 3. Persistent Keccak state between related operations
 */

/* 
 * HW-accelerated thash with multi-buffer SHAKE256
 * Uses Keccak coprocessor for both bitmask generation and final hash
 */
void thash_hw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr);

/*
 * HW thash with persistent state for chain optimization
 * The bitmask SHAKE256 state can be precomputed and reused
 * when pub_seed || addr prefix doesn't change (only hash_addr changes)
 */
void thash_hw_chain_opt(uint8_t *out, const uint8_t *in,
                        const spx_ctx *ctx, const uint8_t *addr,
                        int first_in_chain);

#endif /* THASH_HW_H */
