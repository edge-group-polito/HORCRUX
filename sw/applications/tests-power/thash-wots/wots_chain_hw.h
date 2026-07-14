#ifndef WOTS_CHAIN_HW_H
#define WOTS_CHAIN_HW_H

#include <stdint.h>
#include "thash_hw.h"

/*
 * Hardware-accelerated WOTS+ chain computation using HORCRUX Keccak coprocessor
 *
 * Optimizations:
 * 1. Uses HW thash with Keccak coprocessor
 * 2. Potential Keccak state persistence between chain iterations
 * 3. Batch processing for multiple chains
 */

/* 
 * gen_chain_hw: HW-accelerated chain computation
 */
void gen_chain_hw(uint8_t *out, const uint8_t *in,
                  uint32_t start, uint32_t steps,
                  const spx_ctx *ctx, uint8_t *addr);

/*
 * Run multiple independent chains with HW acceleration
 * Returns total thash calls made
 */
unsigned int run_multiple_chains_hw(uint8_t *outputs, const uint8_t *inputs,
                                     unsigned int num_chains,
                                     const uint32_t *starts,
                                     const uint32_t *steps_arr,
                                     const spx_ctx *ctx, uint8_t *addr);

#endif /* WOTS_CHAIN_HW_H */
