#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fips202.h"
#include "core_v_mini_mcu.h"
#include "csr.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

static int verify_hash(const uint8_t *result, const uint8_t *expected, size_t len) {
    if (memcmp(result, expected, len) == 0) {
        return 1;
    }

    printf("[FAIL] SHAKE256 consistency\n");
    printf("  Expected: ");
    for (size_t i = 0; i < (len < 16 ? len : 16); i++) {
        printf("%02x", expected[i]);
    }
    if (len > 16) {
        printf("...");
    }
    printf("\n  Got:      ");
    for (size_t i = 0; i < (len < 16 ? len : 16); i++) {
        printf("%02x", result[i]);
    }
    if (len > 16) {
        printf("...");
    }
    printf("\n");
    return 0;
}

int main(void) {
    uint8_t msg[100];
    uint8_t sw_out[64];
    uint8_t hw_out[64];
    uint8_t sw_ref[64];
    uint32_t cycles_sw = 0;
    uint32_t cycles_hw = 0;
    int all_passed = 1;

    for (int i = 0; i < 100; i++) {
        msg[i] = (uint8_t)(i & 0xFF);
    }

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   KECCAK ABS SHAKE256 TEST\n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake256(sw_out, sizeof(sw_out), msg, sizeof(msg));
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("SW cycles: %u\n", cycles_sw);

    shake256(sw_ref, sizeof(sw_ref), msg, sizeof(msg));
    all_passed &= verify_hash(sw_out, sw_ref, sizeof(sw_out));
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake256_hw(hw_out, sizeof(hw_out), msg, sizeof(msg));
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("HW cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    all_passed &= verify_hash(hw_out, sw_out, sizeof(hw_out));
    printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
    if (cycles_sw > 0 && cycles_hw > 0) {
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100U) / cycles_hw) % 100U);
    }
#endif

    if (all_passed) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
        return EXIT_SUCCESS;
    }

    printf("FINAL STATUS: TEST FAILURE DETECTED\n");
    return 0;
}
