#ifndef THASH_HW_H
#define THASH_HW_H

#include <stdint.h>
#include <stddef.h>
#include "thash_sw.h"  /* Reuse spx_ctx and address functions */

/*
 * Hardware-accelerated thash implementation using OP_THASH1
 * 
 * OP_THASH1 performs the complete thash-robust algorithm for 1 block:
 *   1. Generates bitmask = SHAKE256(pub_seed || addr)[0:16]
 *   2. Applies XOR with input
 *   3. Computes final hash = SHAKE256(pub_seed || addr || masked)[0:16]
 *
 * Register file layout before OP_THASH1:
 *   reg[0:3]   (16 bytes) = pub_seed
 *   reg[4:11]  (32 bytes) = addr
 *   reg[12:15] (16 bytes) = input
 *
 * Register file layout after OP_THASH1:
 *   reg[0:3]   (16 bytes) = output hash
 */

/* 
 * HW-accelerated thash using OP_THASH1 instruction
 * 
 * Precondition: pub_seed, addr, input loaded to register file
 * Postcondition: output in reg[0:3]
 */
void thash_hw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr);

/*
 * Optimized thash for WOTS chains - leverages register file persistence
 */
void thash_hw_chain_opt(uint8_t *out, const uint8_t *in,
                        const spx_ctx *ctx, const uint8_t *addr);

#endif /* THASH_HW_H */
