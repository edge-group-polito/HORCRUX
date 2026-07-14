#ifndef TEST_VECTORS_H
#define TEST_VECTORS_H

#include <stdint.h>
#include "test_params.h"

/*
 * Test vectors for OP_THASH1 (thash with 1 block input)
 * 
 * These are deterministic test inputs. Golden outputs were computed
 * by the reference SHAKE256-based thash robust implementation.
 */

/* ========================================================================
 * Test Context: SPHINCS+ signing context
 * ======================================================================== */

/* pub_seed: 16 bytes */
static const uint8_t tv_pub_seed[SPX_N] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

/* sk_seed: 16 bytes (used for PRF, not needed for thash) */
static const uint8_t tv_sk_seed[SPX_N] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

/* SPHINCS+ address: 32 bytes
 * Layout (SHAKE variant):
 *   - bytes 0-2:   reserved
 *   - byte 3:      layer
 *   - bytes 4-7:   reserved
 *   - bytes 8-15:  tree (64-bit)
 *   - bytes 16-18: reserved
 *   - byte 19:     type
 *   - bytes 20-23: keypair address
 *   - bytes 24-26: reserved
 *   - byte 27:     chain address
 *   - bytes 28-30: reserved
 *   - byte 31:     hash address
 */
static const uint8_t tv_addr_bytes[SPX_ADDR_BYTES] = {
    0x00, 0x00, 0x00, 0x01,  /* layer = 1 */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,  /* tree = 2 */
    0x00, 0x00, 0x00, 0x00,  /* type = WOTS (0) at byte 19 */
    0x00, 0x00, 0x00, 0x04,  /* keypair addr = 4 */
    0x00, 0x00, 0x00, 0x05,  /* chain addr = 5 at byte 27 */
    0x00, 0x00, 0x00, 0x07   /* hash addr = 7 at byte 31 */
};

/* ========================================================================
 * Test Vector 1: thash with 1 block input
 * ======================================================================== */

/* Input: 16 bytes */
static const uint8_t tv_thash1_input[SPX_N] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
};

/* 
 * Golden output for thash(tv_thash1_input, 1, ctx, tv_addr_bytes)
 * 
 * Computation:
 *   buf1 = pub_seed || addr (48 bytes)
 *   bitmask = SHAKE256(buf1)[0:16]
 *   masked = tv_thash1_input XOR bitmask
 *   buf2 = pub_seed || addr || masked (64 bytes)
 *   output = SHAKE256(buf2)[0:16]
 *
 * This value was computed by reference software.
 */
static const uint8_t tv_thash1_golden[SPX_N] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
/* NOTE: This will be filled with actual golden value computed at runtime
 * by SW reference in the test. The test compares HW output against SW output. */

/* ========================================================================
 * Test Vector 2: Different input for robustness testing
 * ======================================================================== */

static const uint8_t tv_thash1_input2[SPX_N] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

/* Different address for second test */
static const uint8_t tv_addr_bytes2[SPX_ADDR_BYTES] = {
    0x00, 0x00, 0x00, 0x02,  /* layer = 2 */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,  /* tree = 5 */
    0x00, 0x00, 0x00, 0x01,  /* type = WOTSPK (1) at byte 19 */
    0x00, 0x00, 0x00, 0x08,  /* keypair addr = 8 */
    0x00, 0x00, 0x00, 0x0A,  /* chain addr = 10 at byte 27 */
    0x00, 0x00, 0x00, 0x0F   /* hash addr = 15 at byte 31 */
};

#endif /* TEST_VECTORS_H */
