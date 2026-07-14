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
// Description: Test for OP_UNPACK_Z - ML-DSA gamma1-range coefficient unpacking
//              for ExpandMask (masking vector y sampling).
//              
//              Input: rs1 = 32-bit packed data
//                     rs2[1:0] = extraction selector (0-3)
//              Output: rd = signed 32-bit coefficient in [-(GAMMA1-1), GAMMA1]
//                      where GAMMA1 = 2^17 = 131072 for ML-DSA-44
//              
//              For ML-DSA-44: 4 coefficients are packed in 9 bytes (18 bits each)
//              Transform: coeff = GAMMA1 - extracted_value
//              Range: [-(GAMMA1-1), GAMMA1] = [-131071, 131072]
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "core_v_mini_mcu.h"
#include "csr.h"

// ML-DSA-44 gamma1 parameter
#define GAMMA1 131072  // 2^17

// Software test flag
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define NTESTS 20

// Software reference: unpack_z extracts 18-bit value and transforms
// coeff = GAMMA1 - packed_value
// Note: This simplified version assumes data is pre-aligned in rs1
static inline int32_t unpack_z_sw(uint32_t data, uint32_t selector) {
    uint32_t raw18;
    
    switch (selector & 0x3) {
        case 0: raw18 = data & 0x3FFFF;         break;  // bits [17:0]
        case 1: raw18 = (data >> 2) & 0x3FFFF;  break;  // bits [19:2]
        case 2: raw18 = (data >> 4) & 0x3FFFF;  break;  // bits [21:4]
        case 3: raw18 = (data >> 6) & 0x3FFFF;  break;  // bits [23:6]
        default: raw18 = 0;
    }
    
    // Transform: coeff = GAMMA1 - raw_value
    // GAMMA1 = 131072, raw in [0, 262143]
    // Result should be in [-(GAMMA1-1), GAMMA1] = [-131071, 131072]
    return (int32_t)GAMMA1 - (int32_t)raw18;
}

int main(void) {
    // Test inputs: 32-bit packed data words
    // These simulate the packed byte stream from SHAKE256
    static const uint32_t data_inputs[NTESTS] = {
        0x00000000,  // All zeros -> coeff = GAMMA1 - 0 = 131072
        0x00020000,  // 2^17 in bits [17:0] -> coeff = 131072 - 131072 = 0
        0x0003FFFF,  // 2^18-1 in bits [17:0] -> coeff = 131072 - 262143 = -131071
        0x00010000,  // 65536 -> coeff = 131072 - 65536 = 65536
        0x00008000,  // 32768 -> coeff = 131072 - 32768 = 98304
        0x0001FFFF,  // 131071 -> coeff = 131072 - 131071 = 1
        0x00020001,  // 131073 -> coeff = 131072 - 131073 = -1
        0x00000001,  // 1 -> coeff = 131072 - 1 = 131071
        0x00012345,  // 74565 -> coeff = 131072 - 74565 = 56507
        0x0002AAAA,  // 174762 -> coeff = 131072 - 174762 = -43690
        // Test different selectors - shift data to test extraction
        0x00000004,  // sel=0: 4, sel=1: 1, sel=2: 0, sel=3: 0
        0x00000010,  // sel=0: 16, sel=1: 4, sel=2: 1, sel=3: 0
        0x00000100,  // sel=0: 256, sel=1: 64, sel=2: 16, sel=3: 4
        0x00001000,  // sel=0: 4096, sel=1: 1024, sel=2: 256, sel=3: 64
        0x00010000,  // sel=0: 65536, sel=1: 16384, sel=2: 4096, sel=3: 1024
        0x00100000,  // sel=0: 0 (masked), sel=1: 65536, sel=2: 16384, sel=3: 4096
        0x01000000,  // larger value testing
        0x10000000,  // larger value testing
        0xFFFFFFFF,  // All ones (masked appropriately)
        0x55555555,  // Alternating pattern
    };

    int32_t out_sw[NTESTS][4] = {0};
    int32_t out_hw[NTESTS][4] = {0};

    int all_pass_sw = 1;
    int all_pass_hw = 1;

    // Generate golden values
    int32_t golden[NTESTS][4];
    for (int i = 0; i < NTESTS; i++) {
        for (int sel = 0; sel < 4; sel++) {
            golden[i][sel] = unpack_z_sw(data_inputs[i], sel);
        }
    }

    unsigned cycles_sw = 0;
#if SW_TEST_ENABLED
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    for (int i = 0; i < NTESTS; i++) {
        for (int sel = 0; sel < 4; sel++) {
            out_sw[i][sel] = unpack_z_sw(data_inputs[i], sel);
        }
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    printf("Software Cycles: %u\n", cycles_sw);

    for (int i = 0; i < NTESTS; i++) {
        for (int sel = 0; sel < 4; sel++) {
            if (out_sw[i][sel] != golden[i][sel]) {
                printf("  [SW][%2d][sel=%d] FAIL: data=0x%08x, exp=%d, got=%d\n",
                       i, sel, data_inputs[i], golden[i][sel], out_sw[i][sel]);
                all_pass_sw = 0;
            }
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    unsigned cycles_hw;
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    // Hardware test: OP_UNPACK_Z uses funct7=0x4B (0b1000011)
    // Instruction encoding: .insn r 0x3b, 0x7, 0x4B, rd, rs1, rs2
    for (int i = 0; i < NTESTS; i++) {
        asm volatile (
            "mv a3, %[src]\n\t"
            "mv a4, %[sel]\n\t"
            ".insn r 0x3b, 0x7, 0x4B, %[dst], a3, a4\n\t"
            : [dst] "=r" (out_hw[i][0])
            : [src] "r" (data_inputs[i]), [sel] "r" (0)
            : "a3", "a4"
        );
        asm volatile (
            "mv a3, %[src]\n\t"
            "mv a4, %[sel]\n\t"
            ".insn r 0x3b, 0x7, 0x4B, %[dst], a3, a4\n\t"
            : [dst] "=r" (out_hw[i][1])
            : [src] "r" (data_inputs[i]), [sel] "r" (1)
            : "a3", "a4"
        );
        asm volatile (
            "mv a3, %[src]\n\t"
            "mv a4, %[sel]\n\t"
            ".insn r 0x3b, 0x7, 0x4B, %[dst], a3, a4\n\t"
            : [dst] "=r" (out_hw[i][2])
            : [src] "r" (data_inputs[i]), [sel] "r" (2)
            : "a3", "a4"
        );
        asm volatile (
            "mv a3, %[src]\n\t"
            "mv a4, %[sel]\n\t"
            ".insn r 0x3b, 0x7, 0x4B, %[dst], a3, a4\n\t"
            : [dst] "=r" (out_hw[i][3])
            : [src] "r" (data_inputs[i]), [sel] "r" (3)
            : "a3", "a4"
        );
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);
    printf("Hardware Cycles: %u\n", cycles_hw);

    for (int i = 0; i < NTESTS; i++) {
        for (int sel = 0; sel < 4; sel++) {
            if (out_hw[i][sel] != golden[i][sel]) {
                printf("  [HW][%2d][sel=%d] FAIL: data=0x%08x, exp=%d, got=%d\n",
                       i, sel, data_inputs[i], golden[i][sel], out_hw[i][sel]);
                all_pass_hw = 0;
            }
        }
    }

    if (cycles_sw > 0 && cycles_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_sw, cycles_hw);
        printf("Speedup: %u.%02ux\n", cycles_sw / cycles_hw, ((cycles_sw * 100) / cycles_hw) % 100);
    }

    // Print sample results
    printf("\n=== Sample Results ===\n");
    printf("Data       | Sel | Raw18  | Coeff (HW)  | Expected\n");
    printf("-----------|-----|--------|-------------|----------\n");
    for (int i = 0; i < 5; i++) {
        for (int sel = 0; sel < 4; sel++) {
            uint32_t raw18;
            switch (sel) {
                case 0: raw18 = data_inputs[i] & 0x3FFFF; break;
                case 1: raw18 = (data_inputs[i] >> 2) & 0x3FFFF; break;
                case 2: raw18 = (data_inputs[i] >> 4) & 0x3FFFF; break;
                case 3: raw18 = (data_inputs[i] >> 6) & 0x3FFFF; break;
                default: raw18 = 0;
            }
            printf("0x%08x |  %d  | %6u | %11d | %d\n",
                   data_inputs[i], sel, raw18, out_hw[i][sel], golden[i][sel]);
        }
    }

    if (all_pass_sw && all_pass_hw) {
        printf("\nUNPACK_Z: All tests passed.\n");
    } else {
        printf("\nUNPACK_Z: Some tests FAILED.\n");
    }

    return (all_pass_sw && all_pass_hw) ? 0 : 1;
}
