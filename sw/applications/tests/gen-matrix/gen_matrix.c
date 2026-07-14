/*
 * Gen Matrix - HARDWARE-OPTIMIZED IMPLEMENTATION (Granular Keccak Instructions)
 * Replicates Kyber gen_matrix functionality (gen_a and gen_at)
 *
 * Uses HW-managed Keccak state throughout: the 1600-bit state lives in the
 * coprocessor register file from absorb through all squeeze blocks.
 * No redundant load/store of the full state between operations.
 *
 * Instructions used:
 *   keccak_hw_init()        - zero HW state
 *   keccak_hw_absorb_xor()  - XOR word pair into HW state
 *   keccak_hw_permute()     - in-place Keccak-f1600 permutation
 *   keccak_hw_store_rate()  - burst-store rate words from HW to memory
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "fips202.h"
#include "kyber_params.h"

/* Rejection sampling: convert uniform random bytes to uniform random coefficients mod KYBER_Q */
static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
  unsigned int ctr, pos;
  uint16_t val0, val1;

  ctr = pos = 0;
  while(ctr < len && pos + 3 <= buflen) {
    val0 = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
    val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4)) & 0xFFF;
    pos += 3;

    if(val0 < KYBER_Q)
      r[ctr++] = val0;
    if(ctr < len && val1 < KYBER_Q)
      r[ctr++] = val1;
  }

  return ctr;
}

/* Load 4 bytes into uint32_t in little-endian order */
static uint32_t load32(const uint8_t x[4]) {
  return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
         ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

/*
 * xof_absorb_hw - Absorb seed + domain separation directly into HW Keccak state
 *
 * Matches the exact computation of the original incremental API:
 *   shake128_absorb_once(state, seed, 32)  → absorb seed as complete SHAKE128 block
 *   shake128_absorb(state, {x,y}, 2)       → permute (pos==rate), then XOR {x,y}
 *   shake128_finalize(state)               → pad second block
 *
 * This is a TWO-BLOCK absorption:
 *   Block 1: seed (32 bytes) + 0x1F@byte32 + 0x80@byte167_MSB → PERMUTE
 *   Block 2: x@byte0 + y@byte1 + 0x1F@byte2 + 0x80@byte167_MSB (not yet permuted)
 *
 * SHAKE128 rate = 168 bytes = 42 words (indices 0..41), 21 word-pairs.
 * Last rate lane (64-bit) at byte offset 160 = word indices 40 (low) and 41 (high).
 */
static void xof_absorb_hw(const uint8_t seed[KYBER_SYMBYTES], uint8_t x, uint8_t y)
{
  unsigned int i;

  /* 1. Zero HW state */
  keccak_hw_init();

  /* 2. XOR seed (32 bytes = 4 word-pairs at indices 0,2,4,6) */
  for(i = 0; i < KYBER_SYMBYTES/4; i += 2)
    keccak_hw_absorb_xor(load32(seed + 4*i), load32(seed + 4*(i+1)), i);

  /* 3. SHAKE128 padding for first block:
   *    0x1F at byte 32 (word 8, low byte) + 0x80 at byte 167 MSB (word 41, bit 31) */
  keccak_hw_absorb_xor(0x1F, 0, 8);
  keccak_hw_absorb_xor(0, 0x80000000u, 40);

  /* 4. PERMUTE first block (this is the key step the original code does) */
  keccak_hw_permute();

  /* 5. XOR domain separation {x, y} + second block padding into permuted state
   *    x at byte 0, y at byte 1, 0x1F at byte 2 → all in word 0 */
  {
    uint32_t w0 = (uint32_t)x | ((uint32_t)y << 8) | ((uint32_t)0x1F << 16);
    keccak_hw_absorb_xor(w0, 0, 0);
  }

  /* 6. Final padding bit for second block: 0x80 at byte 167 MSB (word 41, bit 31) */
  keccak_hw_absorb_xor(0, 0x80000000u, 40);
}

/*
 * xof_squeezeblocks_hw - Squeeze full blocks from HW Keccak state
 *
 * Each block: permute in-place → store rate bytes to memory.
 * State stays in HW registers across all blocks (no load/store of full state).
 *
 * SHAKE128 rate = 168 bytes = 42 32-bit words.
 */
#define SHAKE128_RATE_WORDS (SHAKE128_RATE / 4)  /* 42 */

static void xof_squeezeblocks_hw(uint8_t *out, size_t nblocks)
{
  while(nblocks--) {
    keccak_hw_permute();
    keccak_hw_store_rate(out, SHAKE128_RATE_WORDS);
    out += SHAKE128_RATE;
  }
}

#define GEN_MATRIX_NBLOCKS ((12*KYBER_N/8*(1 << 12)/KYBER_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)

/*
 * gen_matrix - Deterministically generate matrix A or A^T
 *
 * Fully HW-managed: state lives in coprocessor registers from absorb through
 * all squeeze blocks. No keccak_state struct needed.
 */
void gen_matrix(polyvec *a, const uint8_t seed[KYBER_SYMBYTES], int transposed)
{
  unsigned int ctr, i, j;
  unsigned int buflen;
  uint8_t buf[GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES];

  for(i=0; i<KYBER_K; i++) {
    for(j=0; j<KYBER_K; j++) {
      /* Absorb seed + domain separation directly into HW */
      if(transposed)
        xof_absorb_hw(seed, i, j);
      else
        xof_absorb_hw(seed, j, i);

      /* Squeeze initial blocks (state stays in HW between permutations) */
      xof_squeezeblocks_hw(buf, GEN_MATRIX_NBLOCKS);
      buflen = GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES;
      ctr = rej_uniform(a[i].vec[j].coeffs, KYBER_N, buf, buflen);

      /* Squeeze additional blocks as needed for rejection sampling */
      while(ctr < KYBER_N) {
        xof_squeezeblocks_hw(buf, 1);
        buflen = XOF_BLOCKBYTES;
        ctr += rej_uniform(a[i].vec[j].coeffs + ctr, KYBER_N - ctr, buf, buflen);
      }
    }
  }
}

/* Macro wrappers */
void gen_a(polyvec *a, const uint8_t seed[KYBER_SYMBYTES])
{
  gen_matrix(a, seed, 0);
}

void gen_at(polyvec *a, const uint8_t seed[KYBER_SYMBYTES])
{
  gen_matrix(a, seed, 1);
}
