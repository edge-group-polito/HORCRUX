#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "verify.h"
#include "symmetric.h"
#include "randombytes.h"

#include "csr.h"

#include <stdio.h>
/*************************************************
* Name:        crypto_kem_keypair_derand
*
* Description: Generates public and private key
*              for CCA-secure Kyber key encapsulation mechanism
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                (an already allocated array of KYBER_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                (an already allocated array of KYBER_SECRETKEYBYTES bytes)
*              - uint8_t *coins: pointer to input randomness
*                (an already allocated array filled with 2*KYBER_SYMBYTES random bytes)
**
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair_derand(uint8_t *pk,
                              uint8_t *sk,
                              const uint8_t *coins)
{
  unsigned int cycles_function;
  //printf("coins: ");
  //for (size_t i = 0; i < 2 * KYBER_SYMBYTES; i++) {
  //  printf("%02X", coins[i]);
  //}
  //printf("\n");

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  indcpa_keypair_derand(pk, sk, coins);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_keypair_derand indcpa_keypair_derand cycles: %d\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  memcpy(sk+KYBER_INDCPA_SECRETKEYBYTES, pk, KYBER_PUBLICKEYBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_keypair_derand memcpy pk->sk cycles: %d\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  hash_h(sk+KYBER_SECRETKEYBYTES-2*KYBER_SYMBYTES, pk, KYBER_PUBLICKEYBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_keypair_derand hash_h cycles: %d\n", cycles_function);

  /* Value z for pseudo-random output on reject */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  memcpy(sk+KYBER_SECRETKEYBYTES-KYBER_SYMBYTES, coins+KYBER_SYMBYTES, KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_keypair_derand memcpy z cycles: %d\n", cycles_function);
  
  //printf("pk: ");
  //for (size_t i = 0; i < KYBER_PUBLICKEYBYTES; i++) {
  //  printf("%02X", pk[i]);
  //}
  //printf("\n");
  //printf("sk (indcpa part): ");
  //for (size_t i = 0; i < KYBER_INDCPA_SECRETKEYBYTES; i++) {
  //  printf("%02X", sk[i]);
  //}
  //printf("\n");
  return 0;
}

/*************************************************
* Name:        crypto_kem_keypair
*
* Description: Generates public and private key
*              for CCA-secure Kyber key encapsulation mechanism
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                (an already allocated array of KYBER_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                (an already allocated array of KYBER_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair(uint8_t *pk,
                       uint8_t *sk)
{
  uint8_t coins[2*KYBER_SYMBYTES];
  randombytes(coins, 2*KYBER_SYMBYTES);
  crypto_kem_keypair_derand(pk, sk, coins);
  return 0;
}

/*************************************************
* Name:        crypto_kem_enc_derand
*
* Description: Generates cipher text and shared
*              secret for given public key
*
* Arguments:   - uint8_t *ct: pointer to output cipher text
*                (an already allocated array of KYBER_CIPHERTEXTBYTES bytes)
*              - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of KYBER_SSBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                (an already allocated array of KYBER_PUBLICKEYBYTES bytes)
*              - const uint8_t *coins: pointer to input randomness
*                (an already allocated array filled with KYBER_SYMBYTES random bytes)
**
* Returns 0 (success)
**************************************************/
int crypto_kem_enc_derand(uint8_t *ct,
                          uint8_t *ss,
                          const uint8_t *pk,
                          const uint8_t *coins)
{
  unsigned int cycles_function;
  uint8_t buf[2*KYBER_SYMBYTES];
  /* Will contain key, coins */
  uint8_t kr[2*KYBER_SYMBYTES];

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  memcpy(buf, coins, KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_enc_derand memcpy coins->buf cycles: %d\n", cycles_function);

  /* Multitarget countermeasure for coins + contributory KEM */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  hash_h(buf+KYBER_SYMBYTES, pk, KYBER_PUBLICKEYBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_enc_derand hash_h cycles: %d\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  hash_g(kr, buf, 2*KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_enc_derand hash_g cycles: %d\n", cycles_function);

  /* coins are in kr+KYBER_SYMBYTES */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  indcpa_enc(ct, buf, pk, kr+KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_enc_derand indcpa_enc cycles: %d\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  memcpy(ss,kr,KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_enc_derand memcpy kr->ss cycles: %d\n", cycles_function);
  return 0;
}

/*************************************************
* Name:        crypto_kem_enc
*
* Description: Generates cipher text and shared
*              secret for given public key
*
* Arguments:   - uint8_t *ct: pointer to output cipher text
*                (an already allocated array of KYBER_CIPHERTEXTBYTES bytes)
*              - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of KYBER_SSBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                (an already allocated array of KYBER_PUBLICKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_enc(uint8_t *ct,
                   uint8_t *ss,
                   const uint8_t *pk)
{
  uint8_t coins[KYBER_SYMBYTES];
  randombytes(coins, KYBER_SYMBYTES);
  crypto_kem_enc_derand(ct, ss, pk, coins);
  return 0;
}

/*************************************************
* Name:        crypto_kem_dec
*
* Description: Generates shared secret for given
*              cipher text and private key
*
* Arguments:   - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of KYBER_SSBYTES bytes)
*              - const uint8_t *ct: pointer to input cipher text
*                (an already allocated array of KYBER_CIPHERTEXTBYTES bytes)
*              - const uint8_t *sk: pointer to input private key
*                (an already allocated array of KYBER_SECRETKEYBYTES bytes)
*
* Returns 0.
*
* On failure, ss will contain a pseudo-random value.
**************************************************/
int crypto_kem_dec(uint8_t *ss,
                   const uint8_t *ct,
                   const uint8_t *sk)
{
  int fail;
  unsigned int cycles_function;
  uint8_t buf[2*KYBER_SYMBYTES];
  /* Will contain key, coins */
  uint8_t kr[2*KYBER_SYMBYTES];
//  uint8_t cmp[KYBER_CIPHERTEXTBYTES+KYBER_SYMBYTES];
  uint8_t cmp[KYBER_CIPHERTEXTBYTES];
  const uint8_t *pk = sk+KYBER_INDCPA_SECRETKEYBYTES;

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  indcpa_dec(buf, ct, sk);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec indcpa_dec cycles: %d\n", cycles_function);

  /* Multitarget countermeasure for coins + contributory KEM */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  memcpy(buf+KYBER_SYMBYTES, sk+KYBER_SECRETKEYBYTES-2*KYBER_SYMBYTES, KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec memcpy z->buf cycles: %d\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  hash_g(kr, buf, 2*KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec hash_g cycles: %d\n", cycles_function);

  /* coins are in kr+KYBER_SYMBYTES */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  indcpa_enc(cmp, buf, pk, kr+KYBER_SYMBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec indcpa_enc(cmp) cycles: %d\n", cycles_function);

  CSR_WRITE(CSR_REG_MCYCLE, 0);
  fail = verify(ct, cmp, KYBER_CIPHERTEXTBYTES);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec verify cycles: %d\n", cycles_function);

  /* Compute rejection key */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  rkprf(ss,sk+KYBER_SECRETKEYBYTES-KYBER_SYMBYTES,ct);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec rkprf cycles: %d\n", cycles_function);

  /* Copy true key to return buffer if fail is false */
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  cmov(ss,kr,KYBER_SYMBYTES,!fail);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_kem_dec cmov cycles: %d\n", cycles_function);

  return 0;
}
