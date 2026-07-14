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
    uint8_t msg[50];
    uint8_t sw_output[500];
    uint8_t hw_output[500];
    uint32_t cycles_sw = 0;
    uint32_t cycles_hw = 0;
    int all_passed = 1;

    for (int i = 0; i < 50; i++) {
        msg[i] = (uint8_t)(i & 0xFF);
    }

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    printf("   KECCAK ABS SHAKE128 MULTI-SQUEEZE TEST\n");

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake128(sw_output, sizeof(sw_output), msg, sizeof(msg));
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("SW cycles: %u\n", cycles_sw);

    {
        int sw_non_zero = 0;
        for (int i = 0; i < (int)sizeof(sw_output); i++) {
            if (sw_output[i] != 0) {
                sw_non_zero = 1;
                break;
            }
        }
        if (!sw_non_zero) {
            printf("[FAIL] SW SHAKE128 multi-squeeze - all zeros output\n");
            all_passed = 0;
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n");
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    shake128_hw(hw_output, sizeof(hw_output), msg, sizeof(msg));
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("HW cycles: %u\n", cycles_hw);

    {
        int hw_non_zero = 0;
        for (int i = 0; i < (int)sizeof(hw_output); i++) {
            if (hw_output[i] != 0) {
                hw_non_zero = 1;
                break;
            }
        }
        if (!hw_non_zero) {
            printf("[FAIL] HW SHAKE128 multi-squeeze - all zeros output\n");
            all_passed = 0;
        }
    }

#if SW_TEST_ENABLED
    if (memcmp(hw_output, sw_output, sizeof(hw_output)) != 0) {
        printf("[FAIL] SHAKE128 multi-squeeze SW/HW mismatch\n");
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
