#ifndef TEST_VECTORS_H
#define TEST_VECTORS_H

#include <stdint.h>
#include "test_params.h"

/*
 * Test vectors for OP_PRF_ADDR (PRF for secret key derivation)
 * 
 * Used in WOTS+ secret key generation (skgen function).
 * prf_addr(pub_seed, sk_seed, addr) = SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 * (FIPS 205 compliant)
 */

/* ========================================================================
 * Test Context: SPHINCS+ signing context
 * ======================================================================== */

/* pub_seed: 16 bytes (used in PRF per FIPS 205) */
static const uint8_t tv_pub_seed[SPX_N] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

/* sk_seed: 16 bytes (secret seed for PRF) */
static const uint8_t tv_sk_seed[SPX_N] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

/* ========================================================================
 * Test Vector 1: WOTS secret key derivation
 * ======================================================================== */

/* SPHINCS+ address for WOTS secret key: type = WOTS */
static const uint8_t tv_addr_wots[SPX_ADDR_BYTES] = {
    0x00, 0x00, 0x00, 0x01,  /* layer = 1 */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,  /* tree = 2 */
    0x00, 0x00, 0x00, 0x00,  /* type = WOTS (0) at byte 19 */
    0x00, 0x00, 0x00, 0x04,  /* keypair addr = 4 */
    0x00, 0x00, 0x00, 0x05,  /* chain addr = 5 at byte 27 */
    0x00, 0x00, 0x00, 0x00   /* hash addr = 0 at byte 31 (for skgen) */
};

/* ========================================================================
 * Test Vector 2: Different address for chain element
 * ======================================================================== */

static const uint8_t tv_addr_wots2[SPX_ADDR_BYTES] = {
    0x00, 0x00, 0x00, 0x02,  /* layer = 2 */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,  /* tree = 5 */
    0x00, 0x00, 0x00, 0x00,  /* type = WOTS (0) */
    0x00, 0x00, 0x00, 0x08,  /* keypair addr = 8 */
    0x00, 0x00, 0x00, 0x0A,  /* chain addr = 10 */
    0x00, 0x00, 0x00, 0x00   /* hash addr = 0 */
};

/* ========================================================================
 * Test Vector 3: Multiple addresses (simulating full WOTS skgen)
 * ======================================================================== */

/* Base address for WOTS chain generation */
static const uint8_t tv_addr_base[SPX_ADDR_BYTES] = {
    0x00, 0x00, 0x00, 0x00,  /* layer = 0 */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  /* tree = 1 */
    0x00, 0x00, 0x00, 0x00,  /* type = WOTS (0) */
    0x00, 0x00, 0x00, 0x00,  /* keypair addr = 0 */
    0x00, 0x00, 0x00, 0x00,  /* chain addr = 0 */
    0x00, 0x00, 0x00, 0x00   /* hash addr = 0 */
};

#endif /* TEST_VECTORS_H */
