#ifndef API_H
#define API_H

#include <stddef.h>
#include <stdint.h>

#define pqcrystals_dilithium2_PUBLICKEYBYTES 1312
#define pqcrystals_dilithium2_SECRETKEYBYTES 2560
#define pqcrystals_dilithium2_BYTES 2420


int pqcrystals_dilithium2_ref_keypair(uint8_t *pk, uint8_t *sk);

int pqcrystals_dilithium2_ref_signature(uint8_t *sig, size_t *siglen,
                                        const uint8_t *m, size_t mlen,
                                        const uint8_t *ctx, size_t ctxlen,
                                        const uint8_t *sk);

int pqcrystals_dilithium2_ref(uint8_t *sm, size_t *smlen,
                              const uint8_t *m, size_t mlen,
                              const uint8_t *ctx, size_t ctxlen,
                              const uint8_t *sk);

int pqcrystals_dilithium2_ref_verify(const uint8_t *sig, size_t siglen,
                                     const uint8_t *m, size_t mlen,
                                     const uint8_t *ctx, size_t ctxlen,
                                     const uint8_t *pk);

int pqcrystals_dilithium2_ref_open(uint8_t *m, size_t *mlen,
                                   const uint8_t *sm, size_t smlen,
                                   const uint8_t *ctx, size_t ctxlen,
                                   const uint8_t *pk);


#endif
