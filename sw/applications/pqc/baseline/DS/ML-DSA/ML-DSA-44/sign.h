#ifndef SIGN_H
#define SIGN_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "polyvec.h"
#include "poly.h"


int crypto_sign_keypair(uint8_t *pk, uint8_t *sk, uint8_t *seed);


int crypto_sign_signature_internal(uint8_t *sig,
                                   size_t *siglen,
                                   uint8_t *m,
                                   size_t mlen,
                                   const uint8_t *pre,
                                   size_t prelen,
                                   const uint8_t rnd[RNDBYTES],
                                   const uint8_t *sk);

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          uint8_t *m, size_t mlen,
                          const uint8_t *ctx, size_t ctxlen,
                          const uint8_t *sk, const uint8_t rnd[RNDBYTES]);

int crypto_sign(uint8_t *sm, size_t *smlen,
                uint8_t *m, size_t mlen,
                const uint8_t *ctx, size_t ctxlen,
                const uint8_t *sk, const uint8_t rnd[RNDBYTES]);




int crypto_sign_verify_internal(const uint8_t *sig,
                                size_t siglen,
                                const uint8_t *m,
                                size_t mlen,
                                const uint8_t *pre,
                                size_t prelen,
                                const uint8_t *pk);


int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *ctx, size_t ctxlen,
                       const uint8_t *pk);

int crypto_sign_open(uint8_t *m, size_t *mlen,
                     const uint8_t *sm, size_t smlen,
                     const uint8_t *ctx, size_t ctxlen,
                     const uint8_t *pk);

#endif
