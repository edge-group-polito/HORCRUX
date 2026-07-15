//////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
//               Valeria Piscopo    - valeria.piscopo@polito.it
// Design Name:  ML-KEM-1024 KAT Test — HORCRUX-Optimized
// Language:     C
// Date:         April 2026
//
// Description:  Self-contained KAT test for ML-KEM (Kyber) key generation,
//               encapsulation, and decapsulation, adapted from the official NIST
//               reference implementation.
//               This variant enables the HORCRUX custom ISA (hw/ip/coprocessors/) for
//               the accelerated hot-path operations.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "kem.h"
#include "test_vectors_1024.h"


#include "core_v_mini_mcu.h"
#include "csr.h"

#define TEST_KEY  1
#define TEST_ENC  1
#define TEST_DEC  1


uint8_t keypair_rnd[2*KYBER_SYMBYTES];
uint8_t encaps_rnd[KYBER_SYMBYTES];

void printVect(const char* name, const uint8_t* buf, size_t size) {
    printf("%s = ", name);
    for (int i=0; i<size; i++){
        printf("%02X", buf[i]);
    }
    printf("\n");
}

uint8_t pk[KYBER_PUBLICKEYBYTES];
uint8_t sk[KYBER_SECRETKEYBYTES];
uint8_t ct[KYBER_CIPHERTEXTBYTES];
uint8_t key_a[KYBER_SSBYTES];
uint8_t key_b[KYBER_SSBYTES];

int main(void)
{
    unsigned cycles_keygen, cycles_encaps, cycles_decaps;


    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);


    printf("Started tests with size %d\n", 256*KYBER_K);

    char test_str[50] = "Testing";
    #if TEST_KEY
        strcat(test_str," keypair");
    #endif
    #if TEST_ENC
        strcat(test_str," encaps.");
    #endif
    #if TEST_DEC
        strcat(test_str," decaps.");
    #endif
        printf("%s\n", test_str);

    //************************************************* 
    // KEY
    //*************************************************
    #if TEST_KEY
        // Filling coin vector with indcpa_kem_keypair seeds, obtained from the initialization of randombyte with seed
        memcpy(keypair_rnd , TVEC_IN_KEM_KEYPAIR, KYBER_SYMBYTES*2);


        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_kem_keypair_derand(pk, sk, keypair_rnd);

        CSR_READ(CSR_REG_MCYCLE, &cycles_keygen);
        printf("Keygen cycles: %d\n", cycles_keygen);

        if(memcmp(pk, TVEC_OUT_PK, KYBER_PUBLICKEYBYTES)) { printf("ERROR: PK mismatch\n");}
        if(memcmp(sk, TVEC_OUT_SK, KYBER_SECRETKEYBYTES)) { printf("ERROR: SK mismatch\n");}
    #endif /* TEST_KEY */

    //************************************************* 
    // ENCAPSULATION
    //*************************************************
    #if TEST_ENC
        // Setting up Input test vectors
        memcpy(encaps_rnd, TVEC_IN_KEM_ENC, KYBER_SYMBYTES);

        #if TEST_KEY == 0
            memcpy(pk, TVEC_OUT_PK, KYBER_PUBLICKEYBYTES);
        #endif /* TEST_KEY */

        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_kem_enc_derand(ct, key_b, pk, encaps_rnd);

        CSR_READ(CSR_REG_MCYCLE, &cycles_encaps);
        printf("Encaps cycles: %d\n", cycles_encaps);


        if(memcmp(ct, TVEC_OUT_CT, KYBER_CIPHERTEXTBYTES)) { printf("ERROR: CT mismatch\n");}
        if(memcmp(key_b, TVEC_OUT_SS, KYBER_SSBYTES)) { printf("ERROR: SS mismatch\n");}
    #endif /* TEST_ENC */

    //************************************************* 
    // DECAPSULATION
    //*************************************************

    #if TEST_DEC
        #if TEST_KEY == 0
            // In case we do not generate SK in this test we take it from the test vectors
            memcpy(sk, TVEC_OUT_SK, KYBER_SECRETKEYBYTES);
        #endif /* TEST_KEY */
        #if TEST_ENC == 0
            // In case we do not generate CT in this test we take it from the test vectors
            memcpy(ct, TVEC_OUT_CT, KYBER_CIPHERTEXTBYTES);
        #endif /* TEST_ENC */


        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_kem_dec(key_a, ct, sk);

        CSR_READ(CSR_REG_MCYCLE, &cycles_decaps);
        printf("Decaps cycles: %d\n", cycles_decaps);

        if(memcmp(key_a, TVEC_OUT_SS, KYBER_SSBYTES)) { printf("ERROR: SS mismatch\n");}
    #endif /* TEST_DEC */



    printVect("key_b", key_b, KYBER_SSBYTES);
    printVect("key_a", key_a, KYBER_SSBYTES);
    printVect("key_r", TVEC_OUT_SS, KYBER_SSBYTES);
    printf("Test Successful\n");
    

    return 0;
}

