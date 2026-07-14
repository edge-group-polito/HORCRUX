/* Hardware-accelerated FIPS202 implementation for SPHINCS-128f-robust.
 *
 * Keccak state is managed in the HORCRUX coprocessor register file.
 * Public APIs keep the original SPHINCS signatures for drop-in compatibility.
 */

#include <stddef.h>
#include <stdint.h>

#include "fips202.h"

/* HW-managed state functions (absorb + permute in-place) */
extern void     keccak_hw_init(void);
extern void     keccak_hw_absorb_xor(uint32_t lo, uint32_t hi, uint32_t index);
extern void     keccak_hw_permute(void);
extern uint32_t keccak_hw_store_word(uint32_t index);

/*************************************************
 * Name:        load32
 *
 * Description: Load 4 bytes into uint32_t in little-endian order
 **************************************************/
static uint32_t load32(const uint8_t x[4]) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

/*************************************************
 * Name:        keccak_absorb_once_hw
 *
 * Description: Absorb + pad entirely in HW register file.
 **************************************************/
static void keccak_absorb_once_hw(unsigned int r,
                                  const uint8_t *in,
                                  size_t inlen,
                                  uint8_t p)
{
    unsigned int i;
    unsigned int rate_words = r / 4;

    keccak_hw_init();

    while (inlen >= r) {
        for (i = 0; i < rate_words; i += 2) {
            keccak_hw_absorb_xor(load32(in + 4 * i), load32(in + 4 * (i + 1)), i);
        }
        in += r;
        inlen -= r;
        keccak_hw_permute();
    }

    {
        uint32_t xor_buf[42] = {0};

        for (i = 0; i < inlen; i++) {
            xor_buf[i / 4] |= (uint32_t)in[i] << (8 * (i % 4));
        }

        xor_buf[inlen / 4] |= (uint32_t)p << (8 * (inlen % 4));
        xor_buf[rate_words - 1] |= 0x80000000u;

        for (i = 0; i < rate_words; i += 2) {
            if (xor_buf[i] | xor_buf[i + 1]) {
                keccak_hw_absorb_xor(xor_buf[i], xor_buf[i + 1], i);
            }
        }
    }
}

/*************************************************
 * Name:        keccak_hw_store_bytes
 *
 * Description: Read bytes from HW keccak state.
 **************************************************/
static void keccak_hw_store_bytes(uint8_t *out, size_t len) {
    unsigned int i;
    uint32_t w;

    for (i = 0; i < len / 4; i++) {
        w = keccak_hw_store_word(i);
        out[4 * i] = (uint8_t)w;
        out[4 * i + 1] = (uint8_t)(w >> 8);
        out[4 * i + 2] = (uint8_t)(w >> 16);
        out[4 * i + 3] = (uint8_t)(w >> 24);
    }

    if (len % 4) {
        w = keccak_hw_store_word(i);
        for (unsigned int b = 0; b < len % 4; b++) {
            out[4 * i + b] = (uint8_t)(w >> (8 * b));
        }
    }
}

/*************************************************
 * Name:        keccak_absorb_hw
 *
 * Description: Incremental absorb into HW state.
 **************************************************/
static unsigned int keccak_absorb_hw(unsigned int pos,
                                     unsigned int r,
                                     const uint8_t *in,
                                     size_t inlen)
{
    unsigned int rate_words = r / 4;
    unsigned int i;

    while (pos + inlen >= r) {
        uint32_t xor_buf[42] = {0};

        for (i = pos; i < r; i++) {
            xor_buf[i / 4] |= (uint32_t)(*in++) << (8 * (i % 4));
        }

        for (i = 0; i < rate_words; i += 2) {
            if (xor_buf[i] | xor_buf[i + 1]) {
                keccak_hw_absorb_xor(xor_buf[i], xor_buf[i + 1], i);
            }
        }

        inlen -= r - pos;
        keccak_hw_permute();
        pos = 0;
    }

    if (inlen > 0) {
        uint32_t xor_buf[42] = {0};

        for (i = 0; i < inlen; i++) {
            xor_buf[(pos + i) / 4] |= (uint32_t)(*in++) << (8 * ((pos + i) % 4));
        }

        for (i = 0; i < rate_words; i += 2) {
            if (xor_buf[i] | xor_buf[i + 1]) {
                keccak_hw_absorb_xor(xor_buf[i], xor_buf[i + 1], i);
            }
        }

        pos += inlen;
    }

    return pos;
}

/*************************************************
 * Name:        keccak_finalize_hw
 *
 * Description: Finalize absorb in HW state.
 **************************************************/
static void keccak_finalize_hw(unsigned int pos, unsigned int r, uint8_t p)
{
    unsigned int rate_words = r / 4;
    uint32_t xor_buf[42] = {0};

    xor_buf[pos / 4] |= (uint32_t)p << (8 * (pos % 4));
    xor_buf[rate_words - 1] |= 0x80000000u;

    for (unsigned int i = 0; i < rate_words; i += 2) {
        if (xor_buf[i] | xor_buf[i + 1]) {
            keccak_hw_absorb_xor(xor_buf[i], xor_buf[i + 1], i);
        }
    }
}

/*************************************************
 * Name:        keccak_squeeze_hw
 *
 * Description: Incremental squeeze from HW state.
 **************************************************/
static unsigned int keccak_squeeze_hw(uint8_t *out,
                                      size_t outlen,
                                      unsigned int pos,
                                      unsigned int r)
{
    uint32_t w;

    while (outlen) {
        if (pos == r) {
            keccak_hw_permute();
            pos = 0;
        }

        while (pos < r && outlen > 0) {
            unsigned int byte_in_word;
            size_t bytes_in_word;

            w = keccak_hw_store_word(pos / 4);
            byte_in_word = pos % 4;
            bytes_in_word = 4 - byte_in_word;

            if (bytes_in_word > r - pos) {
                bytes_in_word = r - pos;
            }
            if (bytes_in_word > outlen) {
                bytes_in_word = outlen;
            }

            for (unsigned int b = 0; b < bytes_in_word; b++) {
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
 **************************************************/
static void keccak_squeezeblocks_hw(uint8_t *out,
                                    size_t nblocks,
                                    unsigned int r)
{
    while (nblocks) {
        keccak_hw_permute();
        keccak_hw_store_bytes(out, r);
        out += r;
        nblocks--;
    }
}

/* ========================================================================
 * SHAKE128 API
 * ======================================================================== */

void shake128_inc_init(uint64_t *s_inc) {
    keccak_hw_init();
    s_inc[25] = 0;
}

void shake128_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen) {
    s_inc[25] = keccak_absorb_hw((unsigned int)s_inc[25], SHAKE128_RATE, input, inlen);
}

void shake128_inc_finalize(uint64_t *s_inc) {
    keccak_finalize_hw((unsigned int)s_inc[25], SHAKE128_RATE, 0x1F);
    s_inc[25] = SHAKE128_RATE;
}

void shake128_inc_squeeze(uint8_t *output, size_t outlen, uint64_t *s_inc) {
    s_inc[25] = keccak_squeeze_hw(output, outlen, (unsigned int)s_inc[25], SHAKE128_RATE);
}

void shake128_absorb(uint64_t *s, const uint8_t *input, size_t inlen) {
    (void)s;
    keccak_absorb_once_hw(SHAKE128_RATE, input, inlen, 0x1F);
}

void shake128_squeezeblocks(uint8_t *output, size_t nblocks, uint64_t *s) {
    (void)s;
    keccak_squeezeblocks_hw(output, nblocks, SHAKE128_RATE);
}

void shake128(uint8_t *output, size_t outlen,
              const uint8_t *input, size_t inlen) {
    keccak_absorb_once_hw(SHAKE128_RATE, input, inlen, 0x1F);

    while (outlen >= SHAKE128_RATE) {
        keccak_hw_permute();
        keccak_hw_store_bytes(output, SHAKE128_RATE);
        output += SHAKE128_RATE;
        outlen -= SHAKE128_RATE;
    }

    if (outlen > 0) {
        keccak_hw_permute();
        keccak_hw_store_bytes(output, outlen);
    }
}

/* ========================================================================
 * SHAKE256 API
 * ======================================================================== */

void shake256_inc_init(uint64_t *s_inc) {
    keccak_hw_init();
    s_inc[25] = 0;
}

void shake256_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen) {
    s_inc[25] = keccak_absorb_hw((unsigned int)s_inc[25], SHAKE256_RATE, input, inlen);
}

void shake256_inc_finalize(uint64_t *s_inc) {
    keccak_finalize_hw((unsigned int)s_inc[25], SHAKE256_RATE, 0x1F);
    s_inc[25] = SHAKE256_RATE;
}

void shake256_inc_squeeze(uint8_t *output, size_t outlen, uint64_t *s_inc) {
    s_inc[25] = keccak_squeeze_hw(output, outlen, (unsigned int)s_inc[25], SHAKE256_RATE);
}

void shake256_absorb(uint64_t *s, const uint8_t *input, size_t inlen) {
    (void)s;
    keccak_absorb_once_hw(SHAKE256_RATE, input, inlen, 0x1F);
}

void shake256_squeezeblocks(uint8_t *output, size_t nblocks, uint64_t *s) {
    (void)s;
    keccak_squeezeblocks_hw(output, nblocks, SHAKE256_RATE);
}

void shake256(uint8_t *output, size_t outlen,
              const uint8_t *input, size_t inlen) {
    keccak_absorb_once_hw(SHAKE256_RATE, input, inlen, 0x1F);

    while (outlen >= SHAKE256_RATE) {
        keccak_hw_permute();
        keccak_hw_store_bytes(output, SHAKE256_RATE);
        output += SHAKE256_RATE;
        outlen -= SHAKE256_RATE;
    }

    if (outlen > 0) {
        keccak_hw_permute();
        keccak_hw_store_bytes(output, outlen);
    }
}

/* ========================================================================
 * SHA3-256 API
 * ======================================================================== */

void sha3_256_inc_init(uint64_t *s_inc) {
    keccak_hw_init();
    s_inc[25] = 0;
}

void sha3_256_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen) {
    s_inc[25] = keccak_absorb_hw((unsigned int)s_inc[25], SHA3_256_RATE, input, inlen);
}

void sha3_256_inc_finalize(uint8_t *output, uint64_t *s_inc) {
    keccak_finalize_hw((unsigned int)s_inc[25], SHA3_256_RATE, 0x06);
    keccak_hw_permute();
    keccak_hw_store_bytes(output, 32);
}

void sha3_256(uint8_t *output, const uint8_t *input, size_t inlen) {
    keccak_absorb_once_hw(SHA3_256_RATE, input, inlen, 0x06);
    keccak_hw_permute();
    keccak_hw_store_bytes(output, 32);
}

/* ========================================================================
 * SHA3-512 API
 * ======================================================================== */

void sha3_512_inc_init(uint64_t *s_inc) {
    keccak_hw_init();
    s_inc[25] = 0;
}

void sha3_512_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen) {
    s_inc[25] = keccak_absorb_hw((unsigned int)s_inc[25], SHA3_512_RATE, input, inlen);
}

void sha3_512_inc_finalize(uint8_t *output, uint64_t *s_inc) {
    keccak_finalize_hw((unsigned int)s_inc[25], SHA3_512_RATE, 0x06);
    keccak_hw_permute();
    keccak_hw_store_bytes(output, 64);
}

void sha3_512(uint8_t *output, const uint8_t *input, size_t inlen) {
    keccak_absorb_once_hw(SHA3_512_RATE, input, inlen, 0x06);
    keccak_hw_permute();
    keccak_hw_store_bytes(output, 64);
}
