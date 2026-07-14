///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Auth: Alessandra Dolmeta, Valeria Piscopo
// Email: alessandra.dolmeta@polito.it, valeria.piscopo@polito.it
// Affiliation: Politecnico di Torino - @VLSI Lab
// Date: April 2026
//
// Description: Test for OP_REJ_UNIFORM - ML-DSA rejection sampling for matrix A
//              Input: 3 bytes (24 bits) from SHAKE128 stream
//              Output: {valid[31], 8'b0, coeff[22:0]}
//              valid = 1 if coeff < Q (8380417), else 0
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "core_v_mini_mcu.h"
#include "csr.h"

// ML-DSA-44 modulus
#define Q_MLDSA 8380417

// Software test flag
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define NTESTS 100

// Software reference: rejection sampling for uniform coefficients
// Returns: {valid[31], coeff[22:0]} packed in 32-bit word
static inline uint32_t rej_uniform_sw(uint32_t input_3bytes) {
    uint32_t t = input_3bytes & 0x7FFFFF;  // Mask to 23 bits
    if (t < Q_MLDSA) {
        return (1U << 31) | t;  // valid=1, coeff=t
    } else {
        return 0;  // valid=0, coeff=0
    }
}

int main(void) {
    // Test inputs: 24-bit values covering accepted (<Q) and rejected (>=Q) cases
    // Q = 8380417 = 0x7FE001
    static const uint32_t inputs[NTESTS] = {
        // === Values < Q (should be ACCEPTED) ===
        // Zero and small values
        0x000000,    // 0
        0x000001,    // 1
        0x000002,    // 2
        0x00000F,    // 15
        0x0000FF,    // 255
        0x000FFF,    // 4095
        0x00FFFF,    // 65535
        // Powers of 2
        0x000010,    // 16
        0x000100,    // 256
        0x001000,    // 4096
        0x010000,    // 65536
        0x100000,    // 1048576
        0x200000,    // 2097152
        0x400000,    // 4194304
        // Repeating nibble patterns (all < Q)
        0x111111,    // 1118481
        0x222222,    // 2236962
        0x333333,    // 3355443
        0x444444,    // 4473924
        0x555555,    // 5592405
        0x666666,    // 6710886
        0x777777,    // 7829367
        // Random-looking accepted values
        0x123456,    // 1193046
        0x234567,    // 2311527
        0x345678,    // 3430008
        0x456789,    // 4548489
        0x567890,    // 5666960
        0x654321,    // 6636321
        0x765432,    // 7754802
        0x0ABCDE,    // 703710
        0x1ABCDE,    // 1752286
        0x2ABCDE,    // 2800862
        0x3ABCDE,    // 3849438
        0x4ABCDE,    // 4898014
        0x5ABCDE,    // 5946590
        0x6ABCDE,    // 6995166
        // Values approaching Q from below
        0x7F0000,    // 8323072
        0x7FA000,    // 8363008
        0x7FC000,    // 8372224
        0x7FD000,    // 8376320
        0x7FDE00,    // 8379904
        0x7FDF00,    // 8380160
        0x7FDFF0,    // 8380400
        0x7FDFFC,    // 8380412
        0x7FDFFD,    // 8380413
        0x7FDFFE,    // 8380414
        0x7FDFFF,    // 8380415
        0x7FE000,    // 8380416 = Q-1 (last accepted value)
        // Midrange values
        0x3FFFFF,    // 4194303
        0x4FFFFF,    // 5242879
        0x5FFFFF,    // 6291455
        0x6FFFFF,    // 7340031
        0x0FFFFF,    // 1048575
        0x1FFFFF,    // 2097151
        0x2FFFFF,    // 3145727
        // Byte boundary tests
        0x0000FE,    // 254
        0x00FF00,    // 65280
        0x00FFFE,    // 65534
        0xFF0000,    // masked to 0x7F0000 = 8323072 (accepted)
        0xFE0000,    // masked to 0x7E0000 = 8257536 (accepted)
        
        // === Values >= Q (should be REJECTED) ===
        // Q and values just above Q
        0x7FE001,    // 8380417 = Q (first rejected value)
        0x7FE002,    // 8380418 = Q+1
        0x7FE003,    // 8380419 = Q+2
        0x7FE00F,    // 8380431 = Q+14
        0x7FE010,    // 8380432 = Q+15
        0x7FE0FF,    // 8380671 = Q+254
        0x7FE100,    // 8380672 = Q+255
        0x7FE1FF,    // 8380927
        0x7FE200,    // 8380928
        0x7FEF00,    // 8384256
        0x7FEFFF,    // 8384511
        0x7FF000,    // 8384512
        0x7FF100,    // 8384768
        0x7FFFFF,    // 8388607 (max 23-bit value)
        // Values with high bit set (masked to 23 bits, still rejected)
        0x800001,    // masked to 0x000001 = 1 (ACCEPTED after masking!)
        0x8FE001,    // masked to 0x0FE001 = 1040385 (accepted)
        0xFFE001,    // masked to 0x7FE001 = Q (rejected)
        0xFFE002,    // masked to 0x7FE002 = Q+1 (rejected)
        0xFFFFFF,    // masked to 0x7FFFFF = 8388607 (rejected)
        // Upper range rejected values
        0x7FF800,    // 8386560
        0x7FFC00,    // 8387584
        0x7FFE00,    // 8388096
        0x7FFF00,    // 8388352
        0x7FFFF0,    // 8388592
        0x7FFFFC,    // 8388604
        0x7FFFFD,    // 8388605
        0x7FFFFE,    // 8388606
        // More rejected patterns
        0x7FE080,    // Q + 127
        0x7FE800,    // Q + 2047
        0x7FF080,    // 8384640
        // Additional edge cases near Q
        0x7FE004,    // Q+3
        0x7FE005,    // Q+4
        0x7FE006,    // Q+5
        0x7FE007,    // Q+6
        0x7FE008,    // Q+7
    };

    uint32_t out_sw[NTESTS] = {0};
    uint32_t out_hw[NTESTS] = {0};

    int all_pass_sw = 1;
    int all_pass_hw = 1;

    // Generate golden values
    uint32_t golden[NTESTS];
    for (int i = 0; i < NTESTS; i++) {
        golden[i] = rej_uniform_sw(inputs[i]);
    }

    unsigned cycles_sw = 0;
#if SW_TEST_ENABLED
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    for (int i = 0; i < NTESTS; i++) {
        out_sw[i] = rej_uniform_sw(inputs[i]);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("Software Cycles: %u\n", cycles_sw);

    for (int i = 0; i < NTESTS; i++) {
        if (out_sw[i] != golden[i]) {
            printf("  [SW][%2d] FAIL: in=0x%06x, exp=0x%08x, got=0x%08x\n", 
                   i, inputs[i], golden[i], out_sw[i]);
            all_pass_sw = 0;
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    unsigned cycles_hw;
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    // Hardware test: OP_REJ_UNIFORM uses funct7=0x48 (0b1000000)
    // Instruction encoding: .insn r 0x3b, 0x7, 0x48, rd, rs1, x0
    for (int i = 0; i < NTESTS; i++) {
        asm volatile (
            "mv a3, %[src]\n\t"
            ".insn r 0x3b, 0x7, 0x48, %[dst], a3, x0\n\t"
            : [dst] "=r" (out_hw[i])
            : [src] "r" (inputs[i])
            : "a3"
        );
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

    for (int i = 0; i < NTESTS; i++) {
        if (out_hw[i] != golden[i]) {
            uint32_t valid_exp = (golden[i] >> 31) & 1;
            uint32_t coeff_exp = golden[i] & 0x7FFFFF;
            uint32_t valid_got = (out_hw[i] >> 31) & 1;
            uint32_t coeff_got = out_hw[i] & 0x7FFFFF;
            printf("  [HW][%2d] FAIL: in=0x%06x, exp={v=%u,c=%u}, got={v=%u,c=%u}\n",
                   i, inputs[i], valid_exp, coeff_exp, valid_got, coeff_got);
            all_pass_hw = 0;
        }
    }

    if (cycles_sw > 0 && cycles_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }

    if (all_pass_sw && all_pass_hw) {
        printf("REJ_UNIFORM: All tests passed.\n");
    } else {
        printf("REJ_UNIFORM: Some tests FAILED.\n");
    }

    return (all_pass_sw && all_pass_hw) ? 0 : 1;
}
