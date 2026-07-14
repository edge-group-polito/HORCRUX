///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Auth: Alessandra Dolmeta, Valeria Piscopo
// Email: alessandra.dolmeta@polito.it, valeria.piscopo@polito.it
// Affiliation: Politecnico di Torino - @VLSI Lab
// Date: October 2025
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "api.h"

#ifdef PERF_CNT_CYCLES
    #include "core_v_mini_mcu.h"
    #include "csr.h"
#endif

#include "test_vectors_shake256_128f_simple.h"


#define TEST_KEY  1
#define TEST_SIGN  1
#define TEST_SIGN_OPEN 1

#define MLEN_KAT 33
uint8_t keypair_rnd[CRYPTO_SEEDBYTES];
uint8_t signature_rnd[SPX_N];

#define CRYPTO_SECRETKEYBYTES SPX_SK_BYTES
#define CRYPTO_PUBLICKEYBYTES SPX_PK_BYTES
#define CRYPTO_BYTES SPX_BYTES
#define CRYPTO_SEEDBYTES 3*SPX_N

void printVect(char* name, uint8_t* buf, size_t size) {
    printf("%s = ", name);
    for (int i=0; i<size; i++){
        printf("%02X", buf[i]);
    }
    printf("\n");
}

int main(void)
{

    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];

    uint8_t m[MLEN_KAT];
    uint8_t sm[MLEN_KAT + CRYPTO_BYTES];
    uint8_t m1[MLEN_KAT + CRYPTO_BYTES];

        
    unsigned long long  mlen, smlen, mlen1;
    unsigned cycles_keygen, cycles_sign, cycles_sign_open;
    
    #if PERF_CNT_CYCLES == 1
        CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
        CSR_WRITE(CSR_REG_MCYCLE, 0);
    #endif


    char test_str[50] = "Testing";
    #if TEST_KEY
        strcat(test_str," keypair");
    #endif
    #if TEST_SIGN
        strcat(test_str," sign.");
    #endif
    #if TEST_SIGN_OPEN
        strcat(test_str," sign_open.");
    #endif
        printf("%s\n", test_str);

    //************************************************* 
    // KEY
    //*************************************************

    #if TEST_KEY
        // Filling coin vector with indcpa_keypair and kem_keypair seeds
        memcpy(keypair_rnd , TVEC_IN_SIGN_KEYPAIR, 48);
    
        #if PERF_CNT_CYCLES == 1
            CSR_WRITE(CSR_REG_MCYCLE, 0);
        #endif
        crypto_sign_keypair(pk, sk, keypair_rnd);        
        #if PERF_CNT_CYCLES == 1
            CSR_READ(CSR_REG_MCYCLE, &cycles_keygen);
            printf("Keygen cycles: %u\n", cycles_keygen);
        #endif
        

        if(memcmp(pk, TVEC_OUT_PK, CRYPTO_PUBLICKEYBYTES)) { printf("ERROR: PK mismatch\n"); }
        if(memcmp(sk, TVEC_OUT_SK, CRYPTO_SECRETKEYBYTES)) { printf("ERROR: SK mismatch\n");}
        //printVect("pk", pk, CRYPTO_PUBLICKEYBYTES);
        //printVect("sk", sk, CRYPTO_SECRETKEYBYTES);
    #endif /* TEST_KEY */

    //************************************************* 
    // SIGN
    //*************************************************
    #if TEST_SIGN
        // Setting up Input test vectors
        memcpy(m, TVEC_IN_M_SIGN, MLEN_KAT);
        memcpy(signature_rnd , TVEC_IN_SIGN_SIGNATURE, SPX_N);

        #if TEST_KEY == 0
          // In case we do not generate PK in this test we take it from the test vectors
          memcpy(sk, TVEC_OUT_SK, CRYPTO_SECRETKEYBYTES);
        #endif /* TEST_KEY */
        #if PERF_CNT_CYCLES == 1
            CSR_WRITE(CSR_REG_MCYCLE, 0);
        #endif
        crypto_sign(sm, &smlen, m, MLEN_KAT, sk, signature_rnd);
        
        #if PERF_CNT_CYCLES == 1
            CSR_READ(CSR_REG_MCYCLE, &cycles_sign);
            printf("Sign cycles: %d\n", cycles_sign);
        #endif
  
              if(memcmp(sm, TVEC_IN_SM_SIGN, SPX_BYTES)) { printf("ERROR: SM mismatch\n");}
        //printf("mlen1: %llu\n", SPX_BYTES);
        //printf("sm: ");
        //for (int i = 0; i < SPX_BYTES; i++) {
        //    printf("%02x", sm[i]);
        //}
        //printf("\n");

    #endif /* TEST_SIGN */
    
    //************************************************* 
    // IGN_OPEN
    //*************************************************
    #if TEST_SIGN_OPEN

        #if TEST_KEY == 0
          // In case we do not generate PK in this test we take it from the test vectors
          memcpy(pk, TVEC_OUT_PK, CRYPTO_PUBLICKEYBYTES);
        #endif /* TEST_KEY */

        #if TEST_SIGN == 0
          memcpy(sm, TVEC_IN_SM_SIGN, 17121);
        #endif /* TEST_SIGN */
              #if PERF_CNT_CYCLES == 1
            CSR_WRITE(CSR_REG_MCYCLE, 0);
        #endif
        crypto_sign_open(m1, &mlen1, sm, 17121, pk);
        #if PERF_CNT_CYCLES == 1
            CSR_READ(CSR_REG_MCYCLE, &cycles_sign_open);
            printf("Verify cycles: %d\n", cycles_sign_open);
        #endif
        if(memcmp(m1, TVEC_IN_M_SIGN, MLEN_KAT)) { printf("ERROR: M mismatch\n");}

    #endif /* TEST_SIGN_OPEN */
    
    printf("OK\n");

    printf("Test Successful\n");

    return 0;


}

