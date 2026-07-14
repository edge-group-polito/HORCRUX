#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "core_v_mini_mcu.h"
#include "csr.h"

#include "test_vectors.h"
#include "chain_lengths_sw.h"
#include "chain_lengths_hw.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

// Enable/disable intermediate debug prints
#define ENABLE_DEBUG_PRINTS 0

/**
 * Compare two arrays of nibbles and print result
 */
static int compare_results(const uint8_t *result, const uint8_t *expected, 
                          uint32_t len, const char *test_name) {
    int errors = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (result[i] != expected[i]) {
            printf("[ERROR] %s: Mismatch at index %d: got %d, expected %d\n",
                   test_name, i, result[i], expected[i]);
            errors++;
        }
    }
    
    if (errors == 0) {
        //printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s: %d errors\n", test_name, errors);
    }
    
    return errors;
}

/**
 * Print array of nibbles (for debug)
 */
static void print_nibbles(const uint8_t *data, uint32_t len, const char *label) {
#if ENABLE_DEBUG_PRINTS
    printf("%s (len=%d):\n  ", label, len);
    for (uint32_t i = 0; i < len; i++) {
        printf("%d ", data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < len) {
            printf("\n  ");
        }
    }
    printf("\n");
#endif
}

/**
 * Print message bytes (for debug)
 */
static void print_message(const uint8_t *msg, uint32_t len, const char *label) {
#if ENABLE_DEBUG_PRINTS
    printf("%s (len=%d):\n  ", label, len);
    for (uint32_t i = 0; i < len; i++) {
        printf("%02x ", msg[i]);
        if ((i + 1) % 16 == 0 && i + 1 < len) {
            printf("\n  ");
        }
    }
    printf("\n");
#endif
}

/**
 * Test software implementation for a given variant
 */
static int test_sw_variant(const test_vector_t *tv, uint32_t *cycles_out) {
    uint8_t result[67];  // Max length is 67 for 256f
    uint32_t cycles_sw = 0;
    
    printf("\n=== SW Test: %s ===\n", tv->name);
    print_message(tv->msg, tv->msg_bytes, "Input message");
    
    // Software chain_lengths with cycle counting
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    chain_lengths_sw(result, tv->len1, tv->len2, tv->msg);
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    if (cycles_out != NULL) {
        *cycles_out = cycles_sw;
    }
    
    printf("%s SW Cycles: %u\n", tv->name, cycles_sw);
    
    print_nibbles(result, tv->len, "SW Result");
    
    // Compare against expected
    return compare_results(result, tv->expected_output, tv->len, tv->name);
}

/**
 * Test hardware implementation for a given variant
 */
static int test_hw_variant(const test_vector_t *tv, uint32_t variant_idx, uint32_t *cycles_out) {
    uint8_t result[67];  // Max length is 67 for 256f
    uint32_t cycles_hw = 0;
    const uint32_t *msg32 = (const uint32_t *)tv->msg;
    (void)variant_idx;
    
    printf("\n=== HW Test: %s ===\n", tv->name);
    print_message(tv->msg, tv->msg_bytes, "Input message");
    
    // Hardware chain_lengths with cycle counting - use optimized variant-specific functions
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    if (tv->len1 == 32) {
        chain_lengths_hw_128f(result, msg32);
    } else if (tv->len1 == 48) {
        chain_lengths_hw_192f(result, msg32);
    } else {  // len1 == 64
        chain_lengths_hw_256f(result, msg32);
    }
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    if (cycles_out != NULL) {
        *cycles_out = cycles_hw;
    }
    
    printf("%s HW Cycles: %u\n", tv->name, cycles_hw);
    
    print_nibbles(result, tv->len, "HW Result");
    
    // Compare against expected
    return compare_results(result, tv->expected_output, tv->len, tv->name);
}

int main(void) {
    int total_errors = 0;
    
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);

    //printf("SPHINCS+ WOTS+ chain_lengths Test Suite\n");
    //printf("SOFTWARE + HARDWARE IMPLEMENTATION TESTS\n");

    for (uint32_t i = 0; i < NUM_TEST_VECTORS; i++) {
        uint32_t cycles_sw = 0;
        uint32_t cycles_hw = 0;
        total_errors += test_sw_variant(&test_vectors[i], &cycles_sw);
        total_errors += test_hw_variant(&test_vectors[i], i, &cycles_hw);
#if SW_TEST_ENABLED
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        if (cycles_sw > 0) {
            printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
        }
#endif
    }
    
    // ========================================================================
    // FINAL SUMMARY
    // ========================================================================
    printf("TEST SUMMARY\n");
    printf("Total tests: %d\n", NUM_TEST_VECTORS * 2);
    if (total_errors == 0) {
        printf("Result: ALL TESTS PASSED\n");
    } else {
        printf("Result: FAILED with %d errors\n", total_errors);
    }
    
    return total_errors;
}
