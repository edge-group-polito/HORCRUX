/**
 * @file symmetric.c
 * @brief Cryptographic primitives: a SHAKE-256–based pseudo-random number generator (PRNG) and extendable-output
 * function (XOF), plus hash functions built on SHA3-256 and SHA3-512.
 */

#include "symmetric.h"
#include <stdint.h>

/**
 * @typedef shake256_prng_ctx
 * @brief Incremental SHAKE-256 prng context.
 *
 */
shake256incctx shake256_prng_ctx;

/**
 * @brief SHAKE-256 with incremental API and domain separation
 *
 * Derived from function SHAKE_256 in fips202.c
 *
 * @param[in] entropy_input Pointer to input entropy bytes
 * @param[in] personalization_string Pointer to the personalization string
 * @param[in] enlen Length of entropy string in bytes
 * @param[in] perlen Length of the personalization string in bytes
 */
void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen) {
    uint8_t domain = HQC_PRNG_DOMAIN;
    shake256_inc_init(&shake256_prng_ctx);
    shake256_inc_absorb(&shake256_prng_ctx, entropy_input, enlen);
    shake256_inc_absorb(&shake256_prng_ctx, personalization_string, perlen);
    shake256_inc_absorb(&shake256_prng_ctx, &domain, 1);
    shake256_inc_finalize(&shake256_prng_ctx);
}

/**
 * @brief A SHAKE-256 based PRNG
 *
 * Derived from function SHAKE_256 in fips202.c
 *
 * @param[out] output Pointer to output
 * @param[in] outlen length of output in bytes
 */
void prng_get_bytes(uint8_t *output, uint32_t outlen) {
    shake256_inc_squeeze(output, outlen, &shake256_prng_ctx);
}

/**
 * @brief Initializes a SHAKE256 XOF context with a given seed.
 *
 * @param[out] xof_ctx   Pointer to the XOF context to be initialized.
 * @param[in]  seed      Pointer to the input seed.
 * @param[in]  seed_size Size of the seed in bytes.
 */
void xof_init(shake256_xof_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size) {
    uint8_t xof_domain = HQC_XOF_DOMAIN;
    shake256_inc_init(xof_ctx);
    shake256_inc_absorb(xof_ctx, seed, seed_size);
    shake256_inc_absorb(xof_ctx, &xof_domain, 1);
    shake256_inc_finalize(xof_ctx);
}

/**
 * @brief Extracts pseudorandom bytes from a SHAKE256 XOF context.
 *
 * @param[in,out] xof_ctx     Pointer to the initialized XOF context.
 * @param[out]    output      Pointer to the buffer where the output bytes will be written.
 * @param[in]     output_size Number of bytes to extract.
 *
 * @details This function squeezes the specified number of pseudorandom bytes from
 * the SHAKE256 XOF context and stores them in the provided output buffer.
 * The context must have been initialized beforehand using `xof_init()`.
 */
void xof_get_bytes(shake256_xof_ctx *xof_ctx, uint8_t *output, uint32_t output_size) {
    const uint8_t bsize = sizeof(uint64_t);
    const uint8_t remainder = output_size % bsize;
    uint8_t tmp[sizeof(uint64_t)];
    shake256_inc_squeeze(output, output_size - remainder, xof_ctx);
    if (remainder != 0) {
        shake256_inc_squeeze(tmp, bsize, xof_ctx);
        output += output_size - remainder;
        for (uint8_t i = 0; i < remainder; i++) {
            output[i] = tmp[i];
        }
    }
}

/**
 * @brief Computes the hash function I (SHA3-512) with domain separation.
 *
 * @param[out] output Pointer to the buffer where the 64-byte hash output will be stored.
 * @param[in]  seed   Pointer to the input seed to be hashed.
 *
 * @details This function implements the random oracle `I` as specified,
 * using the SHA3-512 hash function. It produces a 64-byte output from the given seed.
 * Optimized: Uses HW-managed multi-buffer absorption (single block: 33 bytes < 72 rate).
 */
void hash_i(uint8_t *output, const uint8_t *seed) {
    uint8_t i_domain = HQC_I_FCT_DOMAIN;
    sha3_512_hw_multi(output, seed, SEED_BYTES, &i_domain, 1, NULL, 0);
}

/**
 * @brief Compute the hash function H (SHA3-256) with domain separation.
 *
 * @param[out] output      Buffer (32 bytes) to receive the hash output.
 * @param[in]  ek_kem      Encapsulation key of the KEM.
 * Optimized: Uses HW-managed multi-buffer absorption.
 */
void hash_h(uint8_t *output, const uint8_t ek_kem[PUBLIC_KEY_BYTES]) {
    uint8_t h_domain = HQC_H_FCT_DOMAIN;
    sha3_256_hw_multi(output, ek_kem, PUBLIC_KEY_BYTES, &h_domain, 1, NULL, 0);
}

/**
 * @brief Compute the hash function G (SHA3-512) with domain separation.
 *
 * @param[out] output        Buffer (64 bytes) to receive the hash output.
 * @param[in]  hash_ek_kem   Hash of the KEM encapsulation key.
 * @param[in]  m             Message bytes.
 * @param[in]  salt          Salt value.
 * Optimized: Uses HW-managed multi-buffer absorption (single block: 65 bytes < 72 rate).
 */
void hash_g(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
            const uint8_t salt[SALT_BYTES]) {
    /* Concatenate m + salt + domain into a small buffer (33 bytes) */
    uint8_t tail[PARAM_SECURITY_BYTES + SALT_BYTES + 1];
    for (size_t i = 0; i < PARAM_SECURITY_BYTES; i++)
        tail[i] = m[i];
    for (size_t i = 0; i < SALT_BYTES; i++)
        tail[PARAM_SECURITY_BYTES + i] = salt[i];
    tail[PARAM_SECURITY_BYTES + SALT_BYTES] = HQC_G_FCT_DOMAIN;

    sha3_512_hw_multi(output, hash_ek_kem, SEED_BYTES, tail, sizeof(tail), NULL, 0);
}

/* Helper: load 4 bytes as uint32_t little-endian */
static inline uint32_t load32_sym(const uint8_t x[4]) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

/* Helper: HW-managed multi-input absorption for SHA3-256
 * Absorbs multiple buffers directly into HW register file */
static void sha3_256_absorb_hw_6(uint8_t out[32],
                                  const uint8_t *in0, size_t in0len,
                                  const uint8_t *in1, size_t in1len,
                                  const uint8_t *in2, size_t in2len,
                                  const uint8_t *in3, size_t in3len,
                                  const uint8_t *in4, size_t in4len,
                                  const uint8_t *in5, size_t in5len)
{
    uint8_t block[SHA3_256_RATE];
    size_t block_pos = 0;
    unsigned int i;
    const unsigned int rate_words = SHA3_256_RATE / 4;

    keccak_hw_init();

    /* Input array for sequential processing */
    const uint8_t *inputs[6] = {in0, in1, in2, in3, in4, in5};
    size_t lengths[6] = {in0len, in1len, in2len, in3len, in4len, in5len};

    for (int idx = 0; idx < 6; idx++) {
        const uint8_t *in = inputs[idx];
        size_t inlen = lengths[idx];

        while (inlen > 0) {
            size_t take = SHA3_256_RATE - block_pos;
            if (take > inlen)
                take = inlen;
            for (i = 0; i < take; i++)
                block[block_pos + i] = in[i];
            block_pos += take;
            in += take;
            inlen -= take;

            if (block_pos == SHA3_256_RATE) {
                for (i = 0; i < rate_words; i += 2)
                    keccak_hw_absorb_xor(load32_sym(block + 4*i), load32_sym(block + 4*(i+1)), i);
                keccak_hw_permute();
                block_pos = 0;
            }
        }
    }

    /* Pad final block with SHA3 domain separator 0x06 */
    {
        uint32_t xor_buf[34] = {0};
        for (i = 0; i < block_pos; i++)
            xor_buf[i/4] |= (uint32_t)block[i] << (8*(i%4));
        xor_buf[block_pos/4] |= (uint32_t)0x06 << (8*(block_pos%4));
        xor_buf[rate_words - 1] |= 0x80000000u;

        for (i = 0; i < rate_words; i += 2) {
            if (xor_buf[i] | xor_buf[i+1])
                keccak_hw_absorb_xor(xor_buf[i], xor_buf[i+1], i);
        }
    }

    keccak_hw_permute();
    /* Extract 32 bytes */
    for (i = 0; i < 8; i++) {
        uint32_t w = keccak_hw_store_word(i);
        out[4*i]   = (uint8_t)(w);
        out[4*i+1] = (uint8_t)(w >> 8);
        out[4*i+2] = (uint8_t)(w >> 16);
        out[4*i+3] = (uint8_t)(w >> 24);
    }
}

/**
 * @brief Compute the hash function J (SHA3-256) with domain separation.
 *
 * @param[out] output       Buffer (32 bytes) to receive the hash output.
 * @param[in]  hash_ek_kem  Hash of the KEM encapsulation key.
 * @param[in]  sigma        The string sigma.
 * @param[in]  c_kem        Pointer to ciphertext struct (includes c_pke.u, c_pke.v, and salt).
 * Optimized: Uses HW-managed direct absorption for 6 inputs.
 */
void hash_j(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t sigma[PARAM_SECURITY_BYTES],
            const ciphertext_kem_t *c_kem) {
    uint8_t k_domain = HQC_J_FCT_DOMAIN;
    sha3_256_absorb_hw_6(output,
                         hash_ek_kem, SEED_BYTES,
                         sigma, PARAM_SECURITY_BYTES,
                         (const uint8_t *)c_kem->c_pke.u, VEC_N_SIZE_BYTES,
                         (const uint8_t *)c_kem->c_pke.v, VEC_N1N2_SIZE_BYTES,
                         c_kem->salt, SALT_BYTES,
                         &k_domain, 1);
}
