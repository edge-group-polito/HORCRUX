#ifndef PRF_HW_H
#define PRF_HW_H

#include <stdint.h>
#include "prf_sw.h"

/*
 * Hardware-accelerated prf_addr implementation using OP_PRF_ADDR
 *
 * Uses the dedicated OP_PRF_ADDR instruction.
 */

/*
 * prf_addr_hw: Hardware-accelerated PRF using OP_PRF_ADDR
 */
void prf_addr_hw(uint8_t *out, const spx_ctx *ctx, const uint8_t *addr);

#endif /* PRF_HW_H */
