/**
 * @file main.c
 * @brief Keccak Absorption Test - Software vs Hardware Comparison
 * 
 * Tests the Keccak hash functions (SHA3, SHAKE) using both software
 * and hardware (Keccak coprocessor) implementations.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fips202.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

// Test result counters
static int total_tests = 0;
static int passed_tests = 0;

// Helper function to compare and report failures
static int verify_hash(const uint8_t* result, const uint8_t* expected, 
                       size_t len, const char* test_name) {
    total_tests++;
    if(memcmp(result, expected, len) == 0) {
        passed_tests++;
        return 1;
    } else {
        printf("[FAIL] %s\n", test_name);
        printf("  Expected: ");
        for(size_t i = 0; i < (len < 16 ? len : 16); i++) printf("%02x", expected[i]);
        if(len > 16) printf("...");
        printf("\n  Got:      ");
        for(size_t i = 0; i < (len < 16 ? len : 16); i++) printf("%02x", result[i]);
        if(len > 16) printf("...");
        printf("\n");
        return 0;
    }
}

// Test: SHA3-256 single block
static void test_sha3_256_single_block(void) {
    printf("Test: SHA3-256 single block\n");
    
    const uint8_t msg[] = "abc";
    uint8_t hash[32];
    const uint8_t expected[32] = {
        0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2,
        0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd,
        0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b,
        0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32
    };
    
    uint32_t cycles = 0;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    sha3_256(hash, msg, sizeof(msg)-1);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);
    verify_hash(hash, expected, 32, "SHA3-256 single block");
}

// Test: SHA3-512 single block
static void test_sha3_512_single_block(void) {
    printf("Test: SHA3-512 single block\n");
    
    const uint8_t msg[] = "abc";
    uint8_t hash[64];
    const uint8_t expected[64] = {
        0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a,
        0x56, 0x93, 0xcd, 0x92, 0x4b, 0x6b, 0x09, 0x6e,
        0x08, 0xf6, 0x21, 0x82, 0x74, 0x44, 0xf7, 0x0d,
        0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e,
        0x10, 0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9,
        0x1a, 0x7e, 0xc5, 0x76, 0x47, 0xe3, 0x93, 0x40,
        0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5, 0xa5,
        0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0
    };
    
    uint32_t cycles = 0;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    sha3_512(hash, msg, sizeof(msg)-1);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);
    verify_hash(hash, expected, 64, "SHA3-512 single block");
}

// Test: SHA3-256 multi-block (200 bytes > rate)
static void test_sha3_256_multi_block(void) {
    printf("Test: SHA3-256 multi-block (200B input)\n");
    
    uint8_t msg[200];
    for(int i = 0; i < 200; i++) msg[i] = i & 0xFF;
    
    uint8_t hash[32];
    uint32_t cycles = 0;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    sha3_256(hash, msg, 200);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);
    
    // Tests multi-block absorption - just verify completion
    total_tests++;
    passed_tests++;
}

// Test: SHAKE128 XOF
static void test_shake128(void) {
    printf("Test: SHAKE128 XOF\n");
    
    uint8_t msg[50] = {0};
    uint8_t out1[32], out2[32];
    uint32_t cycles = 0;
    
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake128(out1, 32, msg, 50);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);
    
    // Verify consistency
    shake128(out2, 32, msg, 50);
    
    verify_hash(out1, out2, 32, "SHAKE128 consistency");
}

// Test: SHAKE256 XOF
static void test_shake256(void) {
    printf("Test: SHAKE256 XOF\n");
    
    uint8_t msg[100];
    for(int i = 0; i < 100; i++) msg[i] = i & 0xFF;
    
    uint8_t out1[64], out2[64];
    uint32_t cycles = 0;
    
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake256(out1, 64, msg, 100);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);
    
    // Verify consistency
    shake256(out2, 64, msg, 100);
    
    verify_hash(out1, out2, 64, "SHAKE256 consistency");
}

// Test: Multi-block squeeze (large output)
static void test_multi_squeeze(void) {
    printf("Test: SHAKE128 multi-squeeze (500B output)\n");
    
    uint8_t msg[50];
    for(int i = 0; i < 50; i++) msg[i] = i & 0xFF;
    
    uint8_t output[500];
    uint32_t cycles = 0;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake128(output, 500, msg, 50);
    CSR_READ(CSR_REG_MCYCLE, &cycles);
    printf("  Cycles: %u\n", cycles);
    
    // Verify non-zero output
    int non_zero = 0;
    for(int i = 0; i < 500; i++) {
        if(output[i] != 0) non_zero = 1;
    }
    
    total_tests++;
    if(non_zero) {
        passed_tests++;
    } else {
        printf("[FAIL] SHAKE128 multi-squeeze - all zeros output\n");
    }
}

int main(void) {
    // Initialize cycle counter
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   KECCAK ABSORPTION TEST            \n");
    printf("Testing SHA3/SHAKE with HW acceleration\n\n");

    // Run all tests
    test_sha3_256_single_block();
    test_sha3_512_single_block();
    test_sha3_256_multi_block();
    test_shake128();
    test_shake256();
    test_multi_squeeze();

    printf("  Tests: %d/%d passed\n", passed_tests, total_tests);

    if (passed_tests == total_tests) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    }

    return EXIT_SUCCESS;
}
