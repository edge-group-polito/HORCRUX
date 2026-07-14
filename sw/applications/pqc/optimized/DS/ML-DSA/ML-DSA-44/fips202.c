/* Based on the public domain implementation in crypto_hash/keccakc512/simple/ from
 * http://bench.cr.yp.to/supercop.html by Ronny Van Keer and the public domain "TweetFips202"
 * implementation from https://twitter.com/tweetfips202 by Gilles Van Assche, Daniel J. Bernstein,
 * and Peter Schwabe */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "fips202.h"


/* HW-managed state functions (absorb + permute in-place) */
extern void     keccak_hw_init(void);
extern void     keccak_hw_absorb_xor(uint32_t lo, uint32_t hi, uint32_t index);
extern void     keccak_hw_permute(void);
extern uint32_t keccak_hw_store_word(uint32_t index);
extern void     keccak_hw_store_rate(uint8_t *out, uint32_t n_words);


/*************************************************
* Name:        load32
*
* Description: Load 4 bytes into uint32_t in little-endian order
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns the loaded 32-bit unsigned integer
**************************************************/
static uint32_t load32(const uint8_t x[4]) {
  return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
         ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

/*************************************************
* Name:        keccak_absorb_once_hw
*
* Description: Absorb + pad entirely in HW register file.
*              Zeros HW state, absorbs full blocks with in-place permutation,
*              then absorbs remaining bytes + padding.
*              After return, HW state contains the padded (but not yet final-permuted) state.
*
* Arguments:   - unsigned int r: rate in bytes
*              - const uint8_t *in: pointer to input
*              - size_t inlen: length of input in bytes
*              - uint8_t p: domain-separation byte
**************************************************/
static void keccak_absorb_once_hw(unsigned int r,
                                   const uint8_t *in,
                                   size_t inlen,
                                   uint8_t p)
{
  unsigned int i;
  unsigned int rate_words = r / 4; /* number of 32-bit words in rate (always even) */

  /* 1. Zero HW state */
  keccak_hw_init();

  /* 2. Absorb full blocks: XOR rate words into HW, then permute in-place */
  while(inlen >= r) {
    for(i = 0; i < rate_words; i += 2)
      keccak_hw_absorb_xor(load32(in + 4*i), load32(in + 4*(i+1)), i);
    in += r;
    inlen -= r;
    keccak_hw_permute();
  }

  /* 3. Absorb remaining bytes + domain byte + final bit
   *    Build 32-bit XOR words for the partial block */
  {
    uint32_t xor_buf[42] = {0}; /* max rate_words = 168/4 = 42 (SHAKE128) */

    for(i = 0; i < inlen; i++)
      xor_buf[i/4] |= (uint32_t)in[i] << (8*(i%4));

    /* Domain separation byte at position inlen */
    xor_buf[inlen/4] |= (uint32_t)p << (8*(inlen%4));

    /* Final bit: bit 63 of last rate lane = bit 31 of word (rate_words - 1) */
    xor_buf[rate_words - 1] |= 0x80000000u;

    /* XOR non-zero pairs into HW state */
    for(i = 0; i < rate_words; i += 2) {
      if(xor_buf[i] | xor_buf[i+1])
        keccak_hw_absorb_xor(xor_buf[i], xor_buf[i+1], i);
    }
  }
}

static void keccak_absorb_once_hw_multi(unsigned int r,
                                        const uint8_t *in0,
                                        size_t in0len,
                                        const uint8_t *in1,
                                        size_t in1len,
                                        const uint8_t *in2,
                                        size_t in2len,
                                        uint8_t p)
{
  uint8_t block[SHAKE128_RATE];
  size_t block_pos = 0;
  unsigned int i;
  unsigned int rate_words = r / 4;

  keccak_hw_init();

  while(in0len > 0 || in1len > 0 || in2len > 0) {
    const uint8_t *in;
    size_t inlen;
    size_t take;

    if(in0len > 0) {
      in = in0;
      inlen = in0len;
      in0 = NULL;
      in0len = 0;
    } else if(in1len > 0) {
      in = in1;
      inlen = in1len;
      in1 = NULL;
      in1len = 0;
    } else {
      in = in2;
      inlen = in2len;
      in2 = NULL;
      in2len = 0;
    }

    while(inlen > 0) {
      take = r - block_pos;
      if(take > inlen)
        take = inlen;
      if(take > 0)
        memcpy(block + block_pos, in, take);
      block_pos += take;
      in += take;
      inlen -= take;

      if(block_pos == r) {
        for(i = 0; i < rate_words; i += 2)
          keccak_hw_absorb_xor(load32(block + 4*i), load32(block + 4*(i+1)), i);
        keccak_hw_permute();
        block_pos = 0;
      }
    }
  }

  {
    uint32_t xor_buf[42] = {0};

    for(i = 0; i < block_pos; i++)
      xor_buf[i/4] |= (uint32_t)block[i] << (8*(i%4));

    xor_buf[block_pos/4] |= (uint32_t)p << (8*(block_pos%4));
    xor_buf[rate_words - 1] |= 0x80000000u;

    for(i = 0; i < rate_words; i += 2) {
      if(xor_buf[i] | xor_buf[i+1])
        keccak_hw_absorb_xor(xor_buf[i], xor_buf[i+1], i);
    }
  }
}

/*************************************************
* Name:        keccak_hw_store_bytes
*
* Description: Read bytes from the HW keccak result (after permutation).
*
* Arguments:   - uint8_t *out: pointer to output buffer
*              - size_t len: number of bytes to read
**************************************************/
static void keccak_hw_store_bytes(uint8_t *out, size_t len) {
  unsigned int i;
  uint32_t w;

  /* Full 32-bit words */
  for(i = 0; i < len/4; i++) {
    w = keccak_hw_store_word(i);
    out[4*i]   = (uint8_t)(w);
    out[4*i+1] = (uint8_t)(w >> 8);
    out[4*i+2] = (uint8_t)(w >> 16);
    out[4*i+3] = (uint8_t)(w >> 24);
  }

  /* Remaining bytes (< 4) */
  if(len % 4) {
    w = keccak_hw_store_word(i);
    for(unsigned int b = 0; b < len % 4; b++)
      out[4*i + b] = (uint8_t)(w >> (8*b));
  }
}

void shake256_absorb_once_hw(const uint8_t *in, size_t inlen) {
  keccak_absorb_once_hw_multi(SHAKE256_RATE, in, inlen, NULL, 0, NULL, 0, 0x1F);
}

void shake256_squeezeblocks_hw(uint8_t *out, size_t nblocks) {
  const uint32_t rate_words = SHAKE256_RATE / 4;

  while(nblocks--) {
    keccak_hw_permute();
    keccak_hw_store_rate(out, rate_words);
    out += SHAKE256_RATE;
  }
}

/*************************************************
* Name:        keccak_absorb_hw
*
* Description: Absorb step of Keccak using HW coprocessor; incremental.
*              Data is absorbed directly into HW state registers.
*
* Arguments:   - unsigned int pos: position in current block to be absorbed
*              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
*              - const uint8_t *in: pointer to input to be absorbed into s
*              - size_t inlen: length of input in bytes
*
* Returns new position pos in current block
**************************************************/
static unsigned int keccak_absorb_hw(unsigned int pos,
                                     unsigned int r,
                                     const uint8_t *in,
                                     size_t inlen)
{
  unsigned int rate_words = r / 4;
  unsigned int i;

  while(pos + inlen >= r) {
    /* Build XOR buffer for remaining bytes in this block */
    uint32_t xor_buf[42] = {0};
    for(i = pos; i < r; i++)
      xor_buf[i/4] |= (uint32_t)*in++ << (8*(i%4));

    /* XOR into HW state */
    for(i = 0; i < rate_words; i += 2) {
      if(xor_buf[i] | xor_buf[i+1])
        keccak_hw_absorb_xor(xor_buf[i], xor_buf[i+1], i);
    }

    inlen -= r - pos;
    keccak_hw_permute();
    pos = 0;
  }

  /* Absorb remaining bytes (< r) */
  if(inlen > 0) {
    uint32_t xor_buf[42] = {0};
    for(i = 0; i < inlen; i++)
      xor_buf[(pos+i)/4] |= (uint32_t)*in++ << (8*((pos+i)%4));

    for(i = 0; i < rate_words; i += 2) {
      if(xor_buf[i] | xor_buf[i+1])
        keccak_hw_absorb_xor(xor_buf[i], xor_buf[i+1], i);
    }
    pos += inlen;
  }

  return pos;
}

/*************************************************
* Name:        keccak_finalize_hw
*
* Description: Finalize absorb step in HW state.
*              XORs domain separation byte and final padding bit.
*
* Arguments:   - unsigned int pos: position in current block
*              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
*              - uint8_t p: domain separation byte
**************************************************/
static void keccak_finalize_hw(unsigned int pos, unsigned int r, uint8_t p)
{
  unsigned int rate_words = r / 4;
  uint32_t xor_buf[42] = {0};

  /* Domain byte at position pos */
  xor_buf[pos/4] |= (uint32_t)p << (8*(pos%4));

  /* Final bit: bit 31 of word (rate_words-1) */
  xor_buf[rate_words - 1] |= 0x80000000u;

  /* XOR into HW state */
  for(unsigned int i = 0; i < rate_words; i += 2) {
    if(xor_buf[i] | xor_buf[i+1])
      keccak_hw_absorb_xor(xor_buf[i], xor_buf[i+1], i);
  }
}

/*************************************************
* Name:        keccak_squeeze_hw
*
* Description: Squeeze step of Keccak from HW state. Squeezes arbitrarily many bytes.
*              Can be called multiple times to keep squeezing, i.e., is incremental.
*
* Arguments:   - uint8_t *out: pointer to output
*              - size_t outlen: number of bytes to be squeezed (written to out)
*              - unsigned int pos: number of bytes in current block already squeezed
*              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
*
* Returns new position pos in current block
**************************************************/
static unsigned int keccak_squeeze_hw(uint8_t *out,
                                      size_t outlen,
                                      unsigned int pos,
                                      unsigned int r)
{
  uint32_t w;

  while(outlen) {
    if(pos == r) {
      keccak_hw_permute();
      pos = 0;
    }

    /* Read bytes from HW state */
    while(pos < r && outlen > 0) {
      w = keccak_hw_store_word(pos / 4);
      unsigned int byte_in_word = pos % 4;
      size_t bytes_in_word = 4 - byte_in_word;
      if(bytes_in_word > r - pos) bytes_in_word = r - pos;
      if(bytes_in_word > outlen) bytes_in_word = outlen;

      for(unsigned int b = 0; b < bytes_in_word; b++) {
        *out++ = (uint8_t)(w >> (8 * (byte_in_word + b)));
      }
      pos += bytes_in_word;
      outlen -= bytes_in_word;
    }
  }

  return pos;
}

/*************************************************
* Name:        keccak_squeezeblocks_hw
*
* Description: Squeeze full blocks from HW state.
*              Can be called multiple times to keep squeezing.
*
* Arguments:   - uint8_t *out: pointer to output blocks
*              - size_t nblocks: number of blocks to be squeezed
*              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
**************************************************/
static void keccak_squeezeblocks_hw(uint8_t *out,
                                    size_t nblocks,
                                    unsigned int r)
{
  while(nblocks) {
    keccak_hw_permute();
    keccak_hw_store_bytes(out, r);
    out += r;
    nblocks -= 1;
  }
}

/*************************************************
* Name:        shake128_init
*
* Description: Initilizes Keccak state for use as SHAKE128 XOF
*
* Arguments:   - keccak_state *state: pointer to (uninitialized) Keccak state
**************************************************/
void shake128_init(keccak_state *state)
{
  keccak_hw_init();
  state->pos = 0;
}

/*************************************************
* Name:        shake128_absorb
*
* Description: Absorb step of the SHAKE128 XOF; incremental.
*
* Arguments:   - keccak_state *state: pointer to (initialized) output Keccak state
*              - const uint8_t *in: pointer to input to be absorbed into s
*              - size_t inlen: length of input in bytes
**************************************************/
void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
  state->pos = keccak_absorb_hw(state->pos, SHAKE128_RATE, in, inlen);
}

/*************************************************
* Name:        shake128_finalize
*
* Description: Finalize absorb step of the SHAKE128 XOF.
*
* Arguments:   - keccak_state *state: pointer to Keccak state
**************************************************/
void shake128_finalize(keccak_state *state)
{
  keccak_finalize_hw(state->pos, SHAKE128_RATE, 0x1F);
  state->pos = SHAKE128_RATE;
}

/*************************************************
* Name:        shake128_squeeze
*
* Description: Squeeze step of SHAKE128 XOF. Squeezes arbitraily many
*              bytes. Can be called multiple times to keep squeezing.
*
* Arguments:   - uint8_t *out: pointer to output blocks
*              - size_t outlen : number of bytes to be squeezed (written to output)
*              - keccak_state *s: pointer to input/output Keccak state
**************************************************/
void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
  state->pos = keccak_squeeze_hw(out, outlen, state->pos, SHAKE128_RATE);
}

/*************************************************
* Name:        shake128_absorb_once
*
* Description: Initialize, absorb into and finalize SHAKE128 XOF; non-incremental.
*
* Arguments:   - keccak_state *state: pointer to (uninitialized) output Keccak state
*              - const uint8_t *in: pointer to input to be absorbed into s
*              - size_t inlen: length of input in bytes
**************************************************/
void shake128_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen)
{
  keccak_absorb_once_hw(SHAKE128_RATE, in, inlen, 0x1F);
  state->pos = SHAKE128_RATE;
}

/*************************************************
* Name:        shake128_squeezeblocks
*
* Description: Squeeze step of SHAKE128 XOF. Squeezes full blocks of
*              SHAKE128_RATE bytes each. Can be called multiple times
*              to keep squeezing. Assumes new block has not yet been
*              started (state->pos = SHAKE128_RATE).
*
* Arguments:   - uint8_t *out: pointer to output blocks
*              - size_t nblocks: number of blocks to be squeezed (written to output)
*              - keccak_state *s: pointer to input/output Keccak state
**************************************************/
void shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state)
{
  keccak_squeezeblocks_hw(out, nblocks, SHAKE128_RATE);
}

/*************************************************
* Name:        shake256_init
*
* Description: Initilizes Keccak state for use as SHAKE256 XOF
*
* Arguments:   - keccak_state *state: pointer to (uninitialized) Keccak state
**************************************************/
void shake256_init(keccak_state *state)
{
  keccak_hw_init();
  state->pos = 0;
}

/*************************************************
* Name:        shake256_absorb
*
* Description: Absorb step of the SHAKE256 XOF; incremental.
*
* Arguments:   - keccak_state *state: pointer to (initialized) output Keccak state
*              - const uint8_t *in: pointer to input to be absorbed into s
*              - size_t inlen: length of input in bytes
**************************************************/
void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
  state->pos = keccak_absorb_hw(state->pos, SHAKE256_RATE, in, inlen);
}

/*************************************************
* Name:        shake256_finalize
*
* Description: Finalize absorb step of the SHAKE256 XOF.
*
* Arguments:   - keccak_state *state: pointer to Keccak state
**************************************************/
void shake256_finalize(keccak_state *state)
{
  keccak_finalize_hw(state->pos, SHAKE256_RATE, 0x1F);
  state->pos = SHAKE256_RATE;
}

/*************************************************
* Name:        shake256_squeeze
*
* Description: Squeeze step of SHAKE256 XOF. Squeezes arbitraily many
*              bytes. Can be called multiple times to keep squeezing.
*
* Arguments:   - uint8_t *out: pointer to output blocks
*              - size_t outlen : number of bytes to be squeezed (written to output)
*              - keccak_state *s: pointer to input/output Keccak state
**************************************************/
void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
  state->pos = keccak_squeeze_hw(out, outlen, state->pos, SHAKE256_RATE);
}

/*************************************************
* Name:        shake256_absorb_once
*
* Description: Initialize, absorb into and finalize SHAKE256 XOF; non-incremental.
*
* Arguments:   - keccak_state *state: pointer to (uninitialized) output Keccak state
*              - const uint8_t *in: pointer to input to be absorbed into s
*              - size_t inlen: length of input in bytes
**************************************************/
void shake256_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen)
{
  keccak_absorb_once_hw(SHAKE256_RATE, in, inlen, 0x1F);
  state->pos = SHAKE256_RATE;
}

/*************************************************
* Name:        shake256_squeezeblocks
*
* Description: Squeeze step of SHAKE256 XOF. Squeezes full blocks of
*              SHAKE256_RATE bytes each. Can be called multiple times
*              to keep squeezing. Assumes next block has not yet been
*              started (state->pos = SHAKE256_RATE).
*
* Arguments:   - uint8_t *out: pointer to output blocks
*              - size_t nblocks: number of blocks to be squeezed (written to output)
*              - keccak_state *s: pointer to input/output Keccak state
**************************************************/
void shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state)
{
  keccak_squeezeblocks_hw(out, nblocks, SHAKE256_RATE);
}

/*************************************************
* Name:        shake128
*
* Description: SHAKE128 XOF with non-incremental API
*              Uses HW-managed state: absorb + permute entirely in coprocessor.
*
* Arguments:   - uint8_t *out: pointer to output
*              - size_t outlen: requested output length in bytes
*              - const uint8_t *in: pointer to input
*              - size_t inlen: length of input in bytes
**************************************************/
void shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  keccak_absorb_once_hw(SHAKE128_RATE, in, inlen, 0x1F);

  /* Squeeze full blocks (state stays in HW between permutations) */
  while(outlen >= SHAKE128_RATE) {
    keccak_hw_permute();
    keccak_hw_store_bytes(out, SHAKE128_RATE);
    out += SHAKE128_RATE;
    outlen -= SHAKE128_RATE;
  }

  /* Squeeze remaining bytes */
  if(outlen > 0) {
    keccak_hw_permute();
    keccak_hw_store_bytes(out, outlen);
  }
}

/*************************************************
* Name:        shake256
*
* Description: SHAKE256 XOF with non-incremental API
*              Uses HW-managed state: absorb + permute entirely in coprocessor.
*
* Arguments:   - uint8_t *out: pointer to output
*              - size_t outlen: requested output length in bytes
*              - const uint8_t *in: pointer to input
*              - size_t inlen: length of input in bytes
**************************************************/
void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  keccak_absorb_once_hw(SHAKE256_RATE, in, inlen, 0x1F);

  /* Squeeze full blocks (state stays in HW between permutations) */
  while(outlen >= SHAKE256_RATE) {
    keccak_hw_permute();
    keccak_hw_store_bytes(out, SHAKE256_RATE);
    out += SHAKE256_RATE;
    outlen -= SHAKE256_RATE;
  }

  /* Squeeze remaining bytes */
  if(outlen > 0) {
    keccak_hw_permute();
    keccak_hw_store_bytes(out, outlen);
  }
}

/*************************************************
* Name:        sha3_256
*
* Description: SHA3-256 with non-incremental API
*              Uses HW-managed state: absorb + permute entirely in coprocessor.
*
* Arguments:   - uint8_t *h: pointer to output (32 bytes)
*              - const uint8_t *in: pointer to input
*              - size_t inlen: length of input in bytes
**************************************************/
void sha3_256(uint8_t h[32], const uint8_t *in, size_t inlen)
{
  keccak_absorb_once_hw(SHA3_256_RATE, in, inlen, 0x06);
  keccak_hw_permute();
  keccak_hw_store_bytes(h, 32);
}

/*************************************************
* Name:        sha3_512
*
* Description: SHA3-512 with non-incremental API
*              Uses HW-managed state: absorb + permute entirely in coprocessor.
*
* Arguments:   - uint8_t *h: pointer to output (64 bytes)
*              - const uint8_t *in: pointer to input
*              - size_t inlen: length of input in bytes
**************************************************/
void sha3_512(uint8_t h[64], const uint8_t *in, size_t inlen)
{
  keccak_absorb_once_hw(SHA3_512_RATE, in, inlen, 0x06);
  keccak_hw_permute();
  keccak_hw_store_bytes(h, 64);
}
