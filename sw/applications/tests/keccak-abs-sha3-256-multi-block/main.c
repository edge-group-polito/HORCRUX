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

int main(void) {
    uint8_t msg[200];
    uint8_t sw_hash[32];
    uint8_t hw_hash[32];
    uint32_t cycles_sw = 0;
    uint32_t cycles_hw = 0;
    int all_passed = 1;

    for (int i = 0; i < 200; i++) {
        msg[i] = (uint8_t)(i & 0xFF);
    }

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   KECCAK ABS SHA3-256 MULTI BLOCK TEST\n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    sha3_256(sw_hash, msg, sizeof(msg));
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("SW cycles: %u\n", cycles_sw);
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    sha3_256_hw(hw_hash, msg, sizeof(msg));
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("HW cycles: %u\n", cycles_hw);

#if SW_TEST_ENABLED
    if (memcmp(hw_hash, sw_hash, sizeof(hw_hash)) != 0) {
        printf("[FAIL] SHA3-256 multi-block SW/HW mismatch\n");
        all_passed = 0;
    }
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
