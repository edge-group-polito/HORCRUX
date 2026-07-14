/*
 * Software reference implementation of WOTS+ chain computation
 */

#include "wots_chain_sw.h"
#include <string.h>

/*
 * gen_chain_sw: Software reference chain computation
 * 
 * Applies thash iteratively:
 *   for i in [start, start+steps):
 *       set_hash_addr(addr, i)
 *       out = thash(out, 1, ctx, addr)
 */
void gen_chain_sw(uint8_t *out, const uint8_t *in,
                  uint32_t start, uint32_t steps,
                  const spx_ctx *ctx, uint8_t *addr) {
    uint32_t i;

    /* Copy input to output buffer */
    memcpy(out, in, SPX_N);

    /* Apply thash 'steps' times */
    for (i = start; i < start + steps && i < SPX_WOTS_W; i++) {
        set_hash_addr(addr, i);
        thash_sw(out, out, 1, ctx, addr);
    }
}

/*
 * Run multiple independent chains (for benchmarking)
 * 
 * This models the WOTS signature generation/verification flow
 * where 35 chains are processed with varying step counts.
 */
unsigned int run_multiple_chains_sw(uint8_t *outputs, const uint8_t *inputs,
                                     unsigned int num_chains,
                                     const uint32_t *starts,
                                     const uint32_t *steps_arr,
                                     const spx_ctx *ctx, uint8_t *addr) {
    unsigned int total_thash = 0;
    unsigned int i;

    for (i = 0; i < num_chains; i++) {
        /* Set chain address for this iteration */
        set_chain_addr(addr, i);
        
        /* Generate chain */
        gen_chain_sw(outputs + i * SPX_N, 
                     inputs + i * SPX_N,
                     starts[i], 
                     steps_arr[i],
                     ctx, addr);
        
        /* Count thash calls */
        total_thash += steps_arr[i];
    }

    return total_thash;
}
