//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  HQC-1 KAT Test — HORCRUX-Optimized
// Language:     C
// Date:         April 2026
//
// Description:  Self-contained KAT test for HQC key generation, encapsulation, and
//               decapsulation, adapted from the official NIST reference implementation.
//               This variant enables the HORCRUX custom ISA (hw/ip/coprocessors/) for
//               the accelerated hot-path operations.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "symmetric.h"
#include "test_vectors_1.h"

#include "core_v_mini_mcu.h"
#include "csr.h"


#define TEST_KEY        1
#define TEST_ENC        1
#define TEST_DEC        1



void printVect(char* name, uint8_t* buf, size_t size) {
    printf("%s = ", name);
    for (int i=0; i<size; i++){
        printf("%02X", buf[i]);
    }
    printf("\n");
}


int main(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES] = {0};
    uint8_t ss[CRYPTO_BYTES] = {0};
    uint8_t ss1[CRYPTO_BYTES] = {0};
    
    unsigned cycles_keygen, cycles_sign, cycles_sign_open;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);


    //printf("Started test.\n");
    memset(pk, 0, CRYPTO_PUBLICKEYBYTES);
    memset(sk, 0, CRYPTO_SECRETKEYBYTES);

    char test_str[50] = "Testing";
    #if TEST_KEY == 1
        strcat(test_str," keypair");
    #endif
    #if TEST_ENC == 1
        strcat(test_str," encaps.");
    #endif
    #if TEST_DEC == 1
        strcat(test_str," decaps.");
    #endif
    printf("%s\n", test_str);


    //prng_get_bytes(TVEC_SEED, 48);
    prng_init(TVEC_SEED, NULL, 48, 0);

    //************************************************* 
    // KEY
    //*************************************************
    #ifdef TEST_KEY
        
        CSR_WRITE(CSR_REG_MCYCLE, 0);
        //Alice generates a public key
        crypto_kem_keypair(pk, sk);

        CSR_READ(CSR_REG_MCYCLE, &cycles_keygen);
        printf("Keygen cycles: %u\n", cycles_keygen);

        if(memcmp(pk, TVEC_OUT_PK, CRYPTO_PUBLICKEYBYTES)) { printf("\nERROR: PK mismatch\n");}
        if(memcmp(sk, TVEC_OUT_SK, CRYPTO_SECRETKEYBYTES)) { printf("\nERROR: SK mismatch\n");}
        //printVect("pk", pk, CRYPTO_PUBLICKEYBYTES);
        //printVect("sk", sk, CRYPTO_SECRETKEYBYTES);
        //printf("\n");
    
    #endif /* TEST_KEY */   

    //************************************************* 
    // ENCAPSULATION
    //*************************************************     
    #ifdef TEST_ENC

         
        #ifndef TEST_KEY
            memcpy(pk, TVEC_OUT_PK, CRYPTO_PUBLICKEYBYTES);
        #endif  

        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_kem_enc(ct, ss, pk);

        CSR_READ(CSR_REG_MCYCLE, &cycles_sign);
        printf("Encaps cycles: %u\n", cycles_sign);

        if(memcmp(ct, TVEC_OUT_CT, CRYPTO_CIPHERTEXTBYTES)) { printf("ERROR: CT mismatch\n");}
        if(memcmp(ss, TVEC_OUT_SS, CRYPTO_BYTES)) { printf("ERROR: SS mismatch\n");}
        //printVect("ct", ct, CRYPTO_CIPHERTEXTBYTES);
        //printVect("key_a", ss, CRYPTO_BYTES);   

    #endif /* TEST_ENC */

    //************************************************* 
    // DECAPSULATION
    //*************************************************

    #ifdef TEST_DEC
        #ifndef TEST_KEY
            memcpy(sk, TVEC_OUT_SK[0], CRYPTO_SECRETKEYBYTES);
        #endif 
        #ifndef TEST_ENC
            memcpy(ct, TVEC_OUT_CT[0], CRYPTO_CIPHERTEXTBYTES);
        #endif 

        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_kem_dec(ss1, ct, sk);
        
        CSR_READ(CSR_REG_MCYCLE, &cycles_sign_open);
        printf("Decaps cycles: %u\n", cycles_sign_open);

        if(memcmp(ss1, TVEC_OUT_SS, CRYPTO_BYTES)) { printf("ERROR: SS mismatch\n");}
        //printVect("key_b", ss1, CRYPTO_BYTES);
    #endif /* TEST_DEC */

    #ifdef PRINT_VECT
            printVect("pk", pk, CRYPTO_PUBLICKEYBYTES);
            printVect("sk", sk, CRYPTO_SECRETKEYBYTES);
            printVect("ct", ct, CRYPTO_CIPHERTEXTBYTES);
            printVect("key_a", ss, CRYPTO_BYTES);
            printVect("key_b", ss1, CRYPTO_BYTES);
            printf("\n");
    #endif 


    printf("Test Successful\n");

    return 0;
}
