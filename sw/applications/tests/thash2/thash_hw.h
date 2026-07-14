#ifndef THASH_HW_H
#define THASH_HW_H

#include <stdint.h>
#include "thash_sw.h"

/*
 * Hardware-accelerated thash implementation using OP_THASH2
 *
 * Uses the dedicated OP_THASH2 instruction for 2-block thash.
 */

/*
 * thash_hw: Hardware-accelerated thash using OP_THASH2
 * 
 * For inblocks=2, uses dedicated OP_THASH2 instruction.
 * Other cases fall back to software.
 */
void thash_hw(uint8_t *out, const uint8_t *in, unsigned int inblocks,
              const spx_ctx *ctx, const uint8_t *addr);

#endif /* THASH_HW_H */
