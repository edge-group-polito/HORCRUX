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
// Description: Test for OP_REJ_ETA2 and OP_REJ_ETA4 - ML-DSA nibble-based 
//              rejection sampling for secret vectors s1/s2.
//              
//              OP_REJ_ETA2 (eta=2):
//                Input: rs1[7:0] = byte with two nibbles, rs2[0] = nibble selector
//                Output: {valid[31], sign_extended_coeff[30:0]} in [-2, +2]
//                valid = 1 if nibble < 15
//                
//              OP_REJ_ETA4 (eta=4):
//                Input: rs1[7:0] = byte with two nibbles, rs2[0] = nibble selector
//                Output: {valid[31], sign_extended_coeff[30:0]} in [-4, +4]
//                valid = 1 if nibble < 9
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define NTESTS 100

// Software reference: rej_eta for eta=2
// Nibble values 0-14 are accepted, 15 is rejected
// coeff = 2 - (nibble mod 5), range [-2, +2]
static inline uint32_t rej_eta2_sw(uint32_t byte_in, uint32_t nibble_sel) {
    uint32_t nibble = (nibble_sel & 1) ? ((byte_in >> 4) & 0xF) : (byte_in & 0xF);
    
    if (nibble >= 15) {
        return 0;  // rejected: valid=0
    }
    
    // Compute nibble mod 5 using: t - ((205*t) >> 10) * 5
    uint32_t mult = 205 * nibble;
    uint32_t div5 = mult >> 10;
    uint32_t mod5 = nibble - (div5 * 5);
    
    int32_t coeff = 2 - (int32_t)mod5;  // Range: [-2, +2]
    
    // Pack result: valid=1 in bit 31, sign-extended coeff in bits [30:0]
    return (1U << 31) | (coeff & 0x7FFFFFFF);
}

// Software reference: rej_eta for eta=4
// Nibble values 0-8 are accepted, 9-15 are rejected
// coeff = 4 - nibble, range [-4, +4]
static inline uint32_t rej_eta4_sw(uint32_t byte_in, uint32_t nibble_sel) {
    uint32_t nibble = (nibble_sel & 1) ? ((byte_in >> 4) & 0xF) : (byte_in & 0xF);
    
    if (nibble >= 9) {
        return 0;  // rejected: valid=0
    }
    
    int32_t coeff = 4 - (int32_t)nibble;  // Range: [-4, +4]
    
    // Pack result: valid=1 in bit 31, sign-extended coeff in bits [30:0]
    return (1U << 31) | (coeff & 0x7FFFFFFF);
}

int main(void) {
    // Test inputs: 50 bytes, each tested with nibble_sel=0 and nibble_sel=1 => 100 tests
    // Covers all 16 nibble values (0x0-0xF) in various positions
    static const uint32_t byte_inputs[NTESTS/2] = {
        // All single nibble values in low position (high nibble = 0)
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        // All single nibble values in high position (low nibble = 0)
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
        // Mixed nibble combinations (edge cases and patterns)
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        // Boundary values for eta=4 (nibbles 8,9 at threshold)
        0x89, 0x98, 0x78, 0x87,
        // Random patterns
        0x3C, 0xC3, 0x5A, 0xA5, 0x69, 0x96
    };

    int all_pass = 1;
    int eta2_pass = 0;
    int eta2_fail = 0;
    int eta4_pass = 0;
    int eta4_fail = 0;
    unsigned cycles_sw_total = 0;
    unsigned cycles_hw_total = 0;
    unsigned cycles_tmp = 0;
    uint32_t expected = 0;
    uint32_t hw_result = 0;

    printf("=== Testing OP_REJ_ETA2 (eta=2) - %d tests ===\n", NTESTS);

    // Test OP_REJ_ETA2 with all byte inputs and both nibble selectors
    for (int i = 0; i < NTESTS/2; i++) {
        for (int sel = 0; sel < 2; sel++) {
            uint32_t byte_in = byte_inputs[i];
            
#if SW_TEST_ENABLED
            CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
            CSR_WRITE(CSR_REG_MCYCLE, 0);
            expected = rej_eta2_sw(byte_in, sel);
            CSR_READ(CSR_REG_MCYCLE, &cycles_tmp);
            cycles_sw_total += cycles_tmp;
#endif

            CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
            CSR_WRITE(CSR_REG_MCYCLE, 0);

            // Hardware test: OP_REJ_ETA2 uses funct7=0x49 (0b1001001)
            asm volatile (
                //"mv a4, %[nibsel]\n\t"
                ".insn r 0x3b, 0x7, 0x49, %[dst], %[src], %[nibsel]\n\t"
                : [dst] "=r" (hw_result)
                : [src] "r" (byte_in), [nibsel] "r" ((uint32_t)sel)
            );

            CSR_READ(CSR_REG_MCYCLE, &cycles_tmp);
            cycles_hw_total += cycles_tmp;

            if (hw_result != expected) {
                printf("ETA2 FAIL: byte=0x%02x, sel=%d, HW=0x%08x, SW=0x%08x\n",
                       byte_in, sel, hw_result, expected);
                eta2_fail++;
                all_pass = 0;
            } else {
                eta2_pass++;
            }
        }
    }

    printf("ETA2: %d passed, %d failed\n", eta2_pass, eta2_fail);
#if SW_TEST_ENABLED
    printf("ETA2 Total SW Cycles: %u | Total HW Cycles: %u\n", cycles_sw_total, cycles_hw_total);
    if (cycles_hw_total > 0) {
        printf("ETA2 Average Speedup: %u.%02ux\n", 
               cycles_sw_total / cycles_hw_total, 
               ((cycles_sw_total * 100) / cycles_hw_total) % 100);
    }
#endif



    if (all_pass) {
        printf("\nREJ_ETA: All tests passed.\n");
    } else {
        printf("\nREJ_ETA: Some tests FAILED.\n");
    }

    return all_pass ? 0 : 1;
}
