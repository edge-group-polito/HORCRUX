//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  ML-DSA-87 KAT Test — HORCRUX-Optimized
// Language:     C
// Date:         April 2026
//
// Description:  Self-contained KAT test for ML-DSA (Dilithium) key generation, signing,
//               and verification, adapted from the official NIST reference
//               implementation.
//               This variant enables the HORCRUX custom ISA (hw/ip/coprocessors/) for
//               the accelerated hot-path operations.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "params.h"
#include "sign.h"
#include "test_vectors_5.h"


#include "core_v_mini_mcu.h"
#include "csr.h"


#define TEST_KEY  1
#define TEST_SIGN  1
#define TEST_SIGN_OPEN  1

#define MLEN_KAT 33


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

    uint8_t seed_keygen[SEEDBYTES];
    uint8_t seed_sign[RNDBYTES];

    size_t siglen;
    size_t mlen;
    size_t smlen, mlen1;

    unsigned cycles_keygen, cycles_sign, cycles_sign_open;


    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("Started test.\n");
    memset(pk, 0, CRYPTO_PUBLICKEYBYTES);
    memset(sk, 0, CRYPTO_SECRETKEYBYTES);
    memset(seed_keygen, 0, SEEDBYTES);    
    
    char test_str[50] = "Testing";
    #if TEST_KEY == 1
        strcat(test_str," keypair");
    #endif
    #if TEST_SIGN == 1
        strcat(test_str," sign.");
    #endif
    #if TEST_SIGN_OPEN == 1
        strcat(test_str," verify.");
    #endif
    printf("%s\n", test_str);


    //************************************************* 
    // KEY
    //*************************************************

    #if TEST_KEY

        memcpy(seed_keygen, TVEC_SEED_IN_KEYPAIR, SEEDBYTES);
        
        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_sign_keypair(pk, sk, seed_keygen);

        CSR_READ(CSR_REG_MCYCLE, &cycles_keygen);
        printf("Keygen cycles: %u\n", cycles_keygen);


        if(memcmp(pk, TVEC_OUT_PK, CRYPTO_PUBLICKEYBYTES)) { printf("\nERROR: PK mismatch\n");}
        if(memcmp(sk, TVEC_OUT_SK, CRYPTO_SECRETKEYBYTES)) { printf("\nERROR: SK mismatch\n");}
        //printVect("pk", pk, CRYPTO_PUBLICKEYBYTES);
        //printVect("sk", sk, CRYPTO_SECRETKEYBYTES);
        //printf("\n");


    #endif /* TEST_KEY */

    //************************************************* 
    // SIGN
    //*************************************************
    #if TEST_SIGN
        // Setting up Input test vectors
        memcpy(m, TVEC_IN_M_SIGN, MLEN_KAT);
        memcpy(seed_sign, TVEC_SEED_IN_SIGN, RNDBYTES);

        #if TEST_KEY == 0 
          memcpy(sk, TVEC_OUT_SK, CRYPTO_SECRETKEYBYTES);
        #endif /* TEST_KEY */

        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_sign(sm, &smlen, m, 33, NULL, 0, sk, seed_sign);

        CSR_READ(CSR_REG_MCYCLE, &cycles_sign);
        printf("Sign cycles: %d\n", cycles_sign);

        if(memcmp(sm, TVEC_IN_SM_SIGN, 4660)) { printf("ERROR: SM mismatch\n");}
        //printVect("sm", sm, 4660);

    #endif /* TEST_SIGN */
    
    //************************************************* 
    // IGN_OPEN
    //*************************************************
    #if TEST_SIGN_OPEN

        #if TEST_KEY== 0 
          memcpy(pk, TVEC_OUT_PK, CRYPTO_PUBLICKEYBYTES);
        #endif /* TEST_KEY */
        #if TEST_SIGN == 0 
          memcpy(sm, TVEC_IN_SM_SIGN, 4660);
        #endif /* TEST_SIGN */


        CSR_WRITE(CSR_REG_MCYCLE, 0);
     
       crypto_sign_open(m1, &mlen1, sm, 4660, NULL, 0, pk);

        CSR_READ(CSR_REG_MCYCLE, &cycles_sign_open);
        printf("Verify cycles: %d\n", cycles_sign_open);

        
        if(memcmp(m1, TVEC_IN_M_SIGN, MLEN_KAT)) { printf("ERROR: M mismatch\n");}
        printf("m1: ");
        for (int i = 0; i < MLEN_KAT; i++) {
          printf("%02x", m1[i]);
        }
        printf("\n");

    #endif /* TEST_SIGN_OPEN */

    printf("OK\n");
    printf("Test Successful\n");

    return 0;


}

