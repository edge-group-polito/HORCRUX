#ifndef FIPS202_H
#define FIPS202_H

#include <stddef.h>
#include <stdint.h>

#define SHAKE128_RATE 168
#define SHAKE256_RATE 136
#define SHA3_256_RATE 136
#define SHA3_384_RATE 104
#define SHA3_512_RATE 72

/* HW-managed Keccak state functions (direct coprocessor access) */
extern void     keccak_hw_init(void);
extern void     keccak_hw_absorb_xor(uint32_t lo, uint32_t hi, uint32_t index);
extern void     keccak_hw_permute(void);
extern uint32_t keccak_hw_store_word(uint32_t index);
extern uint32_t keccak_hw_read3(uint32_t byte_offset);

// Context for incremental API
typedef struct {
    uint64_t ctx[26];
} shake128incctx;

// Context for non-incremental API
typedef struct {
    uint64_t ctx[25];
} shake128ctx;

// Context for incremental API
typedef struct {
    uint64_t ctx[26];
} shake256incctx;

// Context for non-incremental API
typedef struct {
    uint64_t ctx[25];
} shake256ctx;

// Context for incremental API
typedef struct {
    uint64_t ctx[26];
} sha3_256incctx;

// Context for incremental API
typedef struct {
    uint64_t ctx[26];
} sha3_384incctx;

// Context for incremental API
typedef struct {
    uint64_t ctx[26];
} sha3_512incctx;

void shake128_absorb(shake128ctx *state, const uint8_t *input, size_t inlen);

void shake128_squeezeblocks(uint8_t *output, size_t nblocks, shake128ctx *state);

void shake128_inc_init(shake128incctx *state);
void shake128_inc_absorb(shake128incctx *state, const uint8_t *input, size_t inlen);
void shake128_inc_finalize(shake128incctx *state);
void shake128_inc_squeeze(uint8_t *output, size_t outlen, shake128incctx *state);

void shake256_absorb(shake256ctx *state, const uint8_t *input, size_t inlen);
void shake256_squeezeblocks(uint8_t *output, size_t nblocks, shake256ctx *state);

void shake256_inc_init(shake256incctx *state);
void shake256_inc_absorb(shake256incctx *state, const uint8_t *input, size_t inlen);
void shake256_inc_finalize(shake256incctx *state);
void shake256_inc_squeeze(uint8_t *output, size_t outlen, shake256incctx *state);

void shake128(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen);

void shake256(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen);

void sha3_256_inc_init(sha3_256incctx *state);
void sha3_256_inc_absorb(sha3_256incctx *state, const uint8_t *input, size_t inlen);
void sha3_256_inc_finalize(uint8_t *output, sha3_256incctx *state);

void sha3_256(uint8_t *output, const uint8_t *input, size_t inlen);

void sha3_384_inc_init(sha3_384incctx *state);
void sha3_384_inc_absorb(sha3_384incctx *state, const uint8_t *input, size_t inlen);
void sha3_384_inc_finalize(uint8_t *output, sha3_384incctx *state);

void sha3_384(uint8_t *output, const uint8_t *input, size_t inlen);

void sha3_512_inc_init(sha3_512incctx *state);
void sha3_512_inc_absorb(sha3_512incctx *state, const uint8_t *input, size_t inlen);
void sha3_512_inc_finalize(uint8_t *output, sha3_512incctx *state);

void sha3_512(uint8_t *output, const uint8_t *input, size_t inlen);

/* HW-managed multi-buffer functions */
void keccak_hw_store_rate(uint8_t *out, size_t nwords);

void shake256_hw_multi(uint8_t *out,
                       size_t outlen,
                       const uint8_t *in0,
                       size_t in0len,
                       const uint8_t *in1,
                       size_t in1len,
                       const uint8_t *in2,
                       size_t in2len);

void sha3_256_hw_multi(uint8_t out[32],
                       const uint8_t *in0,
                       size_t in0len,
                       const uint8_t *in1,
                       size_t in1len,
                       const uint8_t *in2,
                       size_t in2len);

void sha3_512_hw_multi(uint8_t out[64],
                       const uint8_t *in0,
                       size_t in0len,
                       const uint8_t *in1,
                       size_t in1len,
                       const uint8_t *in2,
                       size_t in2len);

#endif
