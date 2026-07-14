#include <stdint.h>
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"
#include "poly.h"
#include "randombytes.h"
#include "symmetric.h"
#include "fips202.h"
#include "csr.h"

#include <stdio.h>
#include <string.h>

/*************************************************
* Name:        crypto_sign_keypair
*
* Description: Generates public and private key.
*
* Arguments:   - uint8_t *pk: pointer to output public key (allocated
*                             array of CRYPTO_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key (allocated
*                             array of CRYPTO_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk, uint8_t *seed) {
  unsigned int cycles_function;
  
  uint8_t seedbuf[2*SEEDBYTES + CRHBYTES];
  uint8_t tr[TRBYTES];
  const uint8_t *rho, *rhoprime, *key;
  polyvecl mat[K];
  polyvecl s1, s1hat;
  polyveck s2, t1, t0;

  /* Get randomness for rho, rhoprime and key */
  //randombytes(seedbuf, SEEDBYTES);
  memset(seedbuf, 0, sizeof(seedbuf));

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  memcpy(seedbuf, seed, SEEDBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair memcpy(seedbuf) cycles: %u\n", cycles_function);

  seedbuf[SEEDBYTES+0] = K;
  seedbuf[SEEDBYTES+1] = L;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256(seedbuf, 2*SEEDBYTES + CRHBYTES, seedbuf, SEEDBYTES+2);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair shake256(seed expansion) cycles: %u\n", cycles_function);

  rho = seedbuf;
  rhoprime = rho + SEEDBYTES;
  key = rhoprime + CRHBYTES;

  /* Expand matrix */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvec_matrix_expand(mat, rho);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyvec_matrix_expand cycles: %u\n", cycles_function);

  /* Sample short vectors s1 and s2 */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_uniform_eta(&s1, rhoprime, 0);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyvecl_uniform_eta cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_uniform_eta(&s2, rhoprime, L);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyveck_uniform_eta cycles: %u\n", cycles_function);

  /* Matrix-vector multiplication */
  s1hat = s1;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_ntt(&s1hat);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyvecl_ntt(s1hat) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvec_matrix_pointwise_montgomery(&t1, mat, &s1hat);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyvec_matrix_pointwise_montgomery cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_reduce(&t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyveck_reduce(t1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_invntt_tomont(&t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyveck_invntt_tomont(t1) cycles: %u\n", cycles_function);

  /* Add error vector s2 */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_add(&t1, &t1, &s2);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyveck_add(t1,s2) cycles: %u\n", cycles_function);

  /* Extract t1 and write public key */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_caddq(&t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyveck_caddq cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_power2round(&t1, &t0, &t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair polyveck_power2round cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  pack_pk(pk, rho, &t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair pack_pk cycles: %u\n", cycles_function);

  /* Compute H(rho, t1) and write secret key */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256(tr, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair shake256(tr) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  pack_sk(sk, rho, tr, key, &t0, &s1, &s2);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair pack_sk cycles: %u\n", cycles_function);

  return 0;
}

/*************************************************
* Name:        crypto_sign_signature_internal
*
* Description: Computes signature. Internal API.
*
* Arguments:   - uint8_t *sig:   pointer to output signature (of length CRYPTO_BYTES)
*              - size_t *siglen: pointer to output length of signature
*              - uint8_t *m:     pointer to message to be signed
*              - size_t mlen:    length of message
*              - uint8_t *pre:   pointer to prefix string
*              - size_t prelen:  length of prefix string
*              - uint8_t *rnd:   pointer to random seed
*              - uint8_t *sk:    pointer to bit-packed secret key
*
* Returns 0 (success)
**************************************************/
int crypto_sign_signature_internal(uint8_t *sig,
                                   size_t *siglen,
                                   uint8_t *m,
                                   size_t mlen,
                                   const uint8_t *pre,
                                   size_t prelen,
                                   const uint8_t rnd[RNDBYTES],
                                   const uint8_t *sk)
{
  unsigned int cycles_function;
  unsigned int n;
  uint8_t seedbuf[2*SEEDBYTES + TRBYTES + 2*CRHBYTES];
  uint8_t *rho, *tr, *key, *mu, *rhoprime;
  uint16_t nonce = 0;
  polyvecl mat[K], s1, y, z;
  polyveck t0, s2, w1, w0, h;
  poly cp;
  keccak_state state;

  rho = seedbuf;
  tr = rho + SEEDBYTES;
  key = tr + TRBYTES;
  mu = key + SEEDBYTES;
  rhoprime = mu + CRHBYTES;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  unpack_sk(rho, tr, key, &t0, &s1, &s2, sk);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal unpack_sk cycles: %u\n", cycles_function);

  /* Compute mu = CRH(tr, pre, msg) */
  /* Debug print */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256_init(&state);
  shake256_absorb(&state, tr, TRBYTES);
  shake256_absorb(&state, pre, prelen);
  shake256_absorb(&state, m, mlen);
  shake256_finalize(&state);
  shake256_squeeze(mu, CRHBYTES, &state);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal compute mu cycles: %u\n", cycles_function);

  /* Compute rhoprime = CRH(key, rnd, mu) */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256_init(&state);
  shake256_absorb(&state, key, SEEDBYTES);
  shake256_absorb(&state, rnd, RNDBYTES);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_finalize(&state);
  shake256_squeeze(rhoprime, CRHBYTES, &state);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal compute rhoprime cycles: %u\n", cycles_function);

  /* Expand matrix and transform vectors */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvec_matrix_expand(mat, rho);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvec_matrix_expand cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_ntt(&s1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_ntt(s1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_ntt(&s2);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_ntt(s2) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_ntt(&t0);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_ntt(t0) cycles: %u\n", cycles_function);

rej:
  /* Sample intermediate vector y */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_uniform_gamma1(&y, rhoprime, nonce++);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_uniform_gamma1 cycles: %u\n", cycles_function);

  /* Matrix-vector multiplication */
  z = y;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_ntt(&z);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_ntt(z) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal matrix pointwise cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_reduce(&w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_reduce(w1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_invntt_tomont(&w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_invntt_tomont(w1) cycles: %u\n", cycles_function);

  /* Decompose w and call the random oracle */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_caddq(&w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_caddq(w1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_decompose(&w1, &w0, &w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_decompose cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_pack_w1(sig, &w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_pack_w1 cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256_init(&state);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_absorb(&state, sig, K*POLYW1_PACKEDBYTES);
  shake256_finalize(&state);
  shake256_squeeze(sig, CTILDEBYTES, &state);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal challenge hash cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  poly_challenge(&cp, sig);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal poly_challenge cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  poly_ntt(&cp);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal poly_ntt(cp) cycles: %u\n", cycles_function);

  /* Compute z, reject if it reveals secret */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_pointwise_poly_montgomery(&z, &cp, &s1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal pointwise(z,cp,s1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_invntt_tomont(&z);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_invntt_tomont(z) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_add(&z, &z, &y);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_add(z,y) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_reduce(&z);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_reduce(z) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  if(polyvecl_chknorm(&z, GAMMA1 - BETA))
  {
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature_internal polyvecl_chknorm(reject) cycles: %u\n", cycles_function);
    goto rej;
  }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyvecl_chknorm(pass) cycles: %u\n", cycles_function);

  /* Check that subtracting cs2 does not change high bits of w and low bits
   * do not reveal secret information */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_pointwise_poly_montgomery(&h, &cp, &s2);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal pointwise(h,cp,s2) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_invntt_tomont(&h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_invntt_tomont(h) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_sub(&w0, &w0, &h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_sub(w0,h) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_reduce(&w0);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_reduce(w0) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  if(polyveck_chknorm(&w0, GAMMA2 - BETA))
  {
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature_internal polyveck_chknorm(w0,reject) cycles: %u\n", cycles_function);
    goto rej;
  }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_chknorm(w0,pass) cycles: %u\n", cycles_function);

  /* Compute hints for w1 */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_pointwise_poly_montgomery(&h, &cp, &t0);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal pointwise(h,cp,t0) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_invntt_tomont(&h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_invntt_tomont(h2) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_reduce(&h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_reduce(h) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  if(polyveck_chknorm(&h, GAMMA2))
  {
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature_internal polyveck_chknorm(h,reject) cycles: %u\n", cycles_function);
    goto rej;
  }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_chknorm(h,pass) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_add(&w0, &w0, &h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_add(w0,h) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  n = polyveck_make_hint(&h, &w0, &w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal polyveck_make_hint cycles: %u\n", cycles_function);

  if(n > OMEGA)
    goto rej;

  /* Write signature */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  pack_sig(sig, sig, &z, &h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature_internal pack_sig cycles: %u\n", cycles_function);

  *siglen = CRYPTO_BYTES;
  return 0;
}

/*************************************************
* Name:        crypto_sign_signature
*
* Description: Computes signature.
*
* Arguments:   - uint8_t *sig:   pointer to output signature (of length CRYPTO_BYTES)
*              - size_t *siglen: pointer to output length of signature
*              - uint8_t *m:     pointer to message to be signed
*              - size_t mlen:    length of message
*              - uint8_t *ctx:   pointer to contex string
*              - size_t ctxlen:  length of contex string
*              - uint8_t *sk:    pointer to bit-packed secret key
*
* Returns 0 (success) or -1 (context string too long)
**************************************************/
int crypto_sign_signature(uint8_t *sig,
                          size_t *siglen,
                          uint8_t *m,
                          size_t mlen,
                          const uint8_t *ctx,
                          size_t ctxlen,
                          const uint8_t *sk,
                          const uint8_t rnd[RNDBYTES])
{
  unsigned int cycles_function;
  int ret;
  size_t i;
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  /* Prepare pre = (0, ctxlen, ctx) */
  pre[0] = 0;
  pre[1] = ctxlen;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  for(i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature prepare pre cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  crypto_sign_signature_internal(sig,siglen,m,mlen,pre,2+ctxlen,rnd,sk);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_signature internal call cycles: %u\n", cycles_function);

  return 0;
}

/*************************************************
* Name:        crypto_sign
*
* Description: Compute signed message.
*
* Arguments:   - uint8_t *sm: pointer to output signed message (allocated
*                             array with CRYPTO_BYTES + mlen bytes),
*                             can be equal to m
*              - size_t *smlen: pointer to output length of signed
*                               message
*              - const uint8_t *m: pointer to message to be signed
*              - size_t mlen: length of message
*              - const uint8_t *ctx: pointer to context string
*              - size_t ctxlen: length of context string
*              - const uint8_t *sk: pointer to bit-packed secret key
*
* Returns 0 (success) or -1 (context string too long)
**************************************************/
int crypto_sign(uint8_t *sm,
                size_t *smlen,
                uint8_t *m,
                size_t mlen,
                const uint8_t *ctx,
                size_t ctxlen,
                const uint8_t *sk,
                const uint8_t rnd[RNDBYTES])
{
  unsigned int cycles_function;
  int ret;
  size_t i;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  for(i = 0; i < mlen; ++i)
    sm[CRYPTO_BYTES + mlen - 1 - i] = m[mlen - 1 - i];
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign copy message into sm cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  ret = crypto_sign_signature(sm, smlen, sm + CRYPTO_BYTES, mlen, ctx, ctxlen, sk, rnd);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign crypto_sign_signature cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  *smlen += mlen;
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign update smlen cycles: %u\n", cycles_function);
  return ret;
}

/*************************************************
* Name:        crypto_sign_verify_internal
*
* Description: Verifies signature. Internal API.
*
* Arguments:   - uint8_t *m: pointer to input signature
*              - size_t siglen: length of signature
*              - const uint8_t *m: pointer to message
*              - size_t mlen: length of message
*              - const uint8_t *pre: pointer to prefix string
*              - size_t prelen: length of prefix string
*              - const uint8_t *pk: pointer to bit-packed public key
*
* Returns 0 if signature could be verified correctly and -1 otherwise
**************************************************/
int crypto_sign_verify_internal(const uint8_t *sig,
                                size_t siglen,
                                const uint8_t *m,
                                size_t mlen,
                                const uint8_t *pre,
                                size_t prelen,
                                const uint8_t *pk)
{
  unsigned int cycles_function;
  unsigned int i;
  uint8_t buf[K*POLYW1_PACKEDBYTES];
  uint8_t rho[SEEDBYTES];
  uint8_t mu[CRHBYTES];
  uint8_t c[CTILDEBYTES];
  uint8_t c2[CTILDEBYTES];
  poly cp;
  polyvecl mat[K], z;
  polyveck t1, w1, h;
  keccak_state state;

  if(siglen != CRYPTO_BYTES)
    return -1;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  unpack_pk(rho, &t1, pk);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal unpack_pk cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  if(unpack_sig(c, &z, &h, sig))
  {
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_verify_internal unpack_sig(fail) cycles: %u\n", cycles_function);
    return -1;
  }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal unpack_sig(pass) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  if(polyvecl_chknorm(&z, GAMMA1 - BETA))
  {
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_verify_internal polyvecl_chknorm(fail) cycles: %u\n", cycles_function);
    return -1;
  }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyvecl_chknorm(pass) cycles: %u\n", cycles_function);

  /* Compute CRH(H(rho, t1), pre, msg) */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256(mu, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  shake256_init(&state);
  shake256_absorb(&state, mu, TRBYTES);
  shake256_absorb(&state, pre, prelen);
  shake256_absorb(&state, m, mlen);
  shake256_finalize(&state);
  shake256_squeeze(mu, CRHBYTES, &state);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal compute mu cycles: %u\n", cycles_function);

  /* Matrix-vector multiplication; compute Az - c2^dt1 */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  poly_challenge(&cp, c);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal poly_challenge cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvec_matrix_expand(mat, rho);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyvec_matrix_expand cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvecl_ntt(&z);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyvecl_ntt(z) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal matrix pointwise cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  poly_ntt(&cp);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal poly_ntt(cp) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_shiftl(&t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_shiftl(t1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_ntt(&t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_ntt(t1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_pointwise_poly_montgomery(&t1, &cp, &t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal pointwise(t1,cp,t1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_sub(&w1, &w1, &t1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_sub(w1,t1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_reduce(&w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_reduce(w1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_invntt_tomont(&w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_invntt_tomont(w1) cycles: %u\n", cycles_function);

  /* Reconstruct w1 */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_caddq(&w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_caddq(w1) cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_use_hint(&w1, &w1, &h);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_use_hint cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  polyveck_pack_w1(buf, &w1);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal polyveck_pack_w1 cycles: %u\n", cycles_function);

  /* Call random oracle and verify challenge */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  shake256_init(&state);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_absorb(&state, buf, K*POLYW1_PACKEDBYTES);
  shake256_finalize(&state);
  shake256_squeeze(c2, CTILDEBYTES, &state);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal challenge recompute cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  for(i = 0; i < CTILDEBYTES; ++i)
    if(c[i] != c2[i])
    {
      CSR_READ(CSR_REG_MCYCLE, &cycles_function);
      printf("crypto_sign_verify_internal challenge compare(fail) cycles: %u\n", cycles_function);
      return -1;
    }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify_internal challenge compare(pass) cycles: %u\n", cycles_function);

  return 0;
}

/*************************************************
* Name:        crypto_sign_verify
*
* Description: Verifies signature.
*
* Arguments:   - uint8_t *m: pointer to input signature
*              - size_t siglen: length of signature
*              - const uint8_t *m: pointer to message
*              - size_t mlen: length of message
*              - const uint8_t *ctx: pointer to context string
*              - size_t ctxlen: length of context string
*              - const uint8_t *pk: pointer to bit-packed public key
*
* Returns 0 if signature could be verified correctly and -1 otherwise
**************************************************/
int crypto_sign_verify(const uint8_t *sig,
                       size_t siglen,
                       const uint8_t *m,
                       size_t mlen,
                       const uint8_t *ctx,
                       size_t ctxlen,
                       const uint8_t *pk)
{
  unsigned int cycles_function;
  int ret;
  size_t i;
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = ctxlen;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  for(i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify prepare pre cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  ret = crypto_sign_verify_internal(sig,siglen,m,mlen,pre,2+ctxlen,pk);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_verify internal call cycles: %u\n", cycles_function);

  return ret;
}

/*************************************************
* Name:        crypto_sign_open
*
* Description: Verify signed message.
*
* Arguments:   - uint8_t *m: pointer to output message (allocated
*                            array with smlen bytes), can be equal to sm
*              - size_t *mlen: pointer to output length of message
*              - const uint8_t *sm: pointer to signed message
*              - size_t smlen: length of signed message
*              - const uint8_t *ctx: pointer to context tring
*              - size_t ctxlen: length of context string
*              - const uint8_t *pk: pointer to bit-packed public key
*
* Returns 0 if signed message could be verified correctly and -1 otherwise
**************************************************/
int crypto_sign_open(uint8_t *m,
                     size_t *mlen,
                     const uint8_t *sm,
                     size_t smlen,
                     const uint8_t *ctx,
                     size_t ctxlen,
                     const uint8_t *pk)
{
  unsigned int cycles_function;
  size_t i;

  if(smlen < CRYPTO_BYTES)
    goto badsig;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  *mlen = smlen - CRYPTO_BYTES;
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_open compute mlen cycles: %u\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  if(crypto_sign_verify(sm, CRYPTO_BYTES, sm + CRYPTO_BYTES, *mlen, ctx, ctxlen, pk))
  {
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_open crypto_sign_verify(fail) cycles: %u\n", cycles_function);
    goto badsig;
  }
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_open crypto_sign_verify(pass) cycles: %u\n", cycles_function);

  /* All good, copy msg, return 0 */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  for(i = 0; i < *mlen; ++i)
    m[i] = sm[CRYPTO_BYTES + i];
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_open copy message cycles: %u\n", cycles_function);
  return 0;

badsig:
  /* Signature verification failed */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  *mlen = 0;
  for(i = 0; i < smlen; ++i)
    m[i] = 0;
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_open badsig clear output cycles: %u\n", cycles_function);

  return -1;
}
