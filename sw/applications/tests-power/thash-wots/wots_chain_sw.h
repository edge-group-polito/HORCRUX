#ifndef WOTS_CHAIN_SW_H
#define WOTS_CHAIN_SW_H

#include <stdint.h>
#include "thash_sw.h"

/*
 * Software reference implementation of WOTS+ chain computation
 * 
 * The chain computation iteratively applies thash:
 *   out = thash(thash(...thash(in)...))  [steps times]
 * 
 * Each iteration uses a different hash address.
 */

/* 
 * gen_chain_sw: Apply thash 'steps' times starting from 'start'
 * 
 * out:   output (SPX_N bytes)
 * in:    input (SPX_N bytes)
 * start: starting index in chain
 * steps: number of thash iterations
 * ctx:   signing context
 * addr:  address (modified in place: hash_addr field)
 */
void gen_chain_sw(uint8_t *out, const uint8_t *in,
                  uint32_t start, uint32_t steps,
                  const spx_ctx *ctx, uint8_t *addr);

/*
 * Benchmark helper: run multiple independent chains
 * Returns total thash calls made (for performance comparison)
 */
unsigned int run_multiple_chains_sw(uint8_t *outputs, const uint8_t *inputs,
                                     unsigned int num_chains,
                                     const uint32_t *starts,
                                     const uint32_t *steps_arr,
                                     const spx_ctx *ctx, uint8_t *addr);

#endif /* WOTS_CHAIN_SW_H */
