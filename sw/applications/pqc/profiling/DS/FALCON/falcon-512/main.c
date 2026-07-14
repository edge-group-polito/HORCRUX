/////////////////////////////////////////////////////////////////////////////////////
//                                                                                 //
// Auth: Alessandra Dolmeta, Valeria Piscopo - Politecnico di Torino               //
// Date: September 2024                                                            //
// Desc: Entry point for testing the FALCON implementation using input and output  //
//       test vector coming from NIST                                              //
//                                                                                 //
/////////////////////////////////////////////////////////////////////////////////////


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "api.h"
#include "test_vectors_512.h"


#include "core_v_mini_mcu.h"
#include "csr.h"



#define TEST_KEY  1
#define TEST_SIGN  1
#define TEST_SIGN_OPEN 1

#ifndef NTESTS
#define NTESTS 1
#else
#if (NTESTS > N_TVECT)
#error "NTEST Must be less than Number of total test vectors"
#endif
#endif


#define MAXMLEN 2048
#define MLEN_KAT 33

// Global variables
uint8_t keypair_rnd[CRYPTO_BYTES];


#if PRINT_VECT
void printVect(char* name, uint8_t* buf, size_t size) {
    printf("%s = ", name);
    for (int i=0; i<size; i++){
        printf("%02X", buf[i]);
    }
    printf("\n");
}
#endif


int main(void)
{

    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];

    unsigned char  m[MLEN_KAT];
    unsigned char  sm[MLEN_KAT + CRYPTO_BYTES];
    unsigned char  m1[MLEN_KAT + CRYPTO_BYTES];
    unsigned char  nonce[40];
    unsigned char seed_sign[48];

    size_t siglen;
    size_t mlen;
    unsigned long long  smlen, mlen1;
    
    
    unsigned cycles_keygen, cycles_sign, cycles_sign_open;

    int r;
    size_t i, k;

    printf("Started %d test\n", NTESTS);
    
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

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

    for(int i=0; i<NTESTS; i++) {

        printf("Test %d: ", i);

    //************************************************* 
    // KEY
    //*************************************************

    #if TEST_KEY
        // Filling coin vector with indcpa_keypair and kem_keypair seeds
        memcpy(keypair_rnd, TVEC_IN_SIGN_KEYPAIR[i], 48);
        
        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_sign_keypair(pk, sk, keypair_rnd);
      
        CSR_READ(CSR_REG_MCYCLE, &cycles_keygen);
        printf("Keygen cycles: %d\n", cycles_keygen);


        if(memcmp(pk, TVEC_OUT_PK[i], CRYPTO_PUBLICKEYBYTES)) { printf("ERROR: PK mismatch\n"); return -1;}
        if(memcmp(sk, TVEC_OUT_SK[i], CRYPTO_SECRETKEYBYTES)) { printf("ERROR: SK mismatch\n"); return -1;}
    #endif /* TEST_KEY */

    //************************************************* 
    // SIGN
    //*************************************************
    #if TEST_SIGN
        // Setting up Input test vectors
        memcpy(m, TVEC_IN_M_SIGN[i], MLEN_KAT);
        memcpy(nonce, TVEC_IN_NONCE_SIGN[i], 40);
        memcpy(seed_sign, TVEC_IN_SEED_SIGN[i], 48 );

        #if TEST_KEY == 0
          // In case we do not generate PK in this test we take it from the test vectors
          memcpy(sk, TVEC_OUT_SK[i], CRYPTO_SECRETKEYBYTES);
        #endif /* TEST_KEY */
        
        CSR_WRITE(CSR_REG_MCYCLE, 0);

        crypto_sign(sm, &smlen, m, MLEN_KAT, sk, nonce, seed_sign);
        
        CSR_READ(CSR_REG_MCYCLE, &cycles_sign);
        printf("Sign cycles: %d\n", cycles_sign);
        
        if(memcmp(sm, TVEC_IN_SM_SIGN[i], CRYPTO_BYTES)) { printf("ERROR: SM mismatch\n"); return -1;}

    #endif /* TEST_SIGN */
    
    //************************************************* 
    // IGN_OPEN
    //*************************************************
    #if TEST_SIGN_OPEN

        #if TEST_KEY == 0
          // In case we do not generate PK in this test we take it from the test vectors
          memcpy(pk, TVEC_OUT_PK[i], CRYPTO_PUBLICKEYBYTES);
        #endif /* TEST_KEY */

        #if TEST_SIGN == 0
          memcpy(sm, TVEC_IN_SM_SIGN[i], 691);
        #endif /* TEST_SIGN */

        CSR_WRITE(CSR_REG_MCYCLE, 0);
        
        crypto_sign_open(m1, &mlen1, sm, 691, pk);
        
        CSR_READ(CSR_REG_MCYCLE, &cycles_sign_open);
        printf("Sign open cycles: %d\n", cycles_sign_open);
        
        if(memcmp(m1, TVEC_IN_M_SIGN[i], MLEN_KAT)) { printf("ERROR: M mismatch\n"); return -1;}


    #endif /* TEST_SIGN_OPEN */

    printf("OK\n");

    }

    printf("Test Successful\n");

    return 0;


}

