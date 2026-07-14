#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fips202.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

int main(void) {
    uint8_t msg[200];
    uint8_t sw_hash[32];
    uint8_t hw_hash[32];

    int all_passed = 1;

    for (int i = 0; i < 200; i++) {
        msg[i] = (uint8_t)(i & 0xFF);
    }



#if SW_TEST_ENABLED
    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    sha3_256(sw_hash, msg, sizeof(msg));
    vcd_disable();
#else

#endif

    if (vcd_init() != 0)
    return 1;

    vcd_enable();
    sha3_256_hw(hw_hash, msg, sizeof(msg));
    vcd_disable();


    return 0;
}
