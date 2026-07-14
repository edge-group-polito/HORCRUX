/*
 * Hardware-accelerated WOTS+ chain computation
 */

#include "wots_chain_hw.h"
#include <string.h>

/*
 * gen_chain_hw: HW-accelerated chain computation
 * Uses HW thash with potential chain optimization
 */
void gen_chain_hw(uint8_t *out, const uint8_t *in,
                  uint32_t start, uint32_t steps,
                  const spx_ctx *ctx, uint8_t *addr) {
    uint32_t i;

    /* Copy input to output buffer */
    memcpy(out, in, SPX_N);

    /* Apply HW thash 'steps' times */
    for (i = start; i < start + steps && i < SPX_WOTS_W; i++) {
        set_hash_addr(addr, i);
        /* Use chain-optimized thash */
        thash_hw_chain_opt(out, out, ctx, addr, (i == start));
    }
}

/*
 * Run multiple chains with HW acceleration
 *
 * This is the main optimization target for WOTS signing/verification.
 * WOTS-128f uses 35 chains with varying step counts.
 */
unsigned int run_multiple_chains_hw(uint8_t *outputs, const uint8_t *inputs,
                                     unsigned int num_chains,
                                     const uint32_t *starts,
                                     const uint32_t *steps_arr,
                                     const spx_ctx *ctx, uint8_t *addr) {
    unsigned int total_thash = 0;
    unsigned int i;

    for (i = 0; i < num_chains; i++) {
        /* Set chain address */
        set_chain_addr(addr, i);
        
        /* Generate chain with HW acceleration */
        gen_chain_hw(outputs + i * SPX_N, 
                     inputs + i * SPX_N,
                     starts[i], 
                     steps_arr[i],
                     ctx, addr);
        
        total_thash += steps_arr[i];
    }

    return total_thash;
}
