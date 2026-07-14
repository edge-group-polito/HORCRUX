///////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2025 PoliTO - @VLSI Lab
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// Auth: Alessandra Dolmeta, Valeria Piscopo
// Email: alessandra.dolmeta@polito.it, valeria.piscopo@polito.it
// Affiliation: Politecnico di Torino - @VLSI Lab
// Date: October 2025
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>



#include "core_v_mini_mcu.h"
#include "csr.h"

// Software test flag - set to 0 to skip SW tests, 1 to run SW+HW tests
#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif


#define NTESTS 20
#define NLANES 4

/* -------- CBD software (parametrizzato per eta) -------- */
// Generic η-style function rewritten explicitly (like cbd_eta1)
void cbd_eta4(int32_t d, int8_t out[NTESTS][4], int i) {
    for (int j = 0; j < 4; j++) {
        int32_t a = (d >> (8 * j + 0)) & 0xF;  // lower 4 bits
        int32_t b = (d >> (8 * j + 4)) & 0xF;  // upper 4 bits
        out[i][j] = (int8_t)(a - b);
    }
}


/* ===================== η = 4 (4 lanes) ====================== */
static const int32_t d_eta4[NTESTS] = {
    1343628325, -1437313772, 38830161, 1417980328, -1869176699,
    1344636249, 1147406601, 446211349, 352866393, 1159043162,
    -1548650083, 478163327, 107420369, -1110032133, 1181241943,
    1051802512, 958682846, 599310825, -1131847511, 440213415,
    -1388565139, -1113823565, -463085232, -1952635852, 373399426,
    -1758821271, 1812140441, 136505587, 127978094, 402418010,
    939042955, 999270936, -2124482863, -1709316540, 113971123,
    -1884438106, 854001193, -1219686479, -1503734903, -1282799476,
    -1954461450, 1801823908, 946785248, 1929338154, -1764090452,
    1194819984, -818489973, -561350837, 27911967, -1035914485,
    -833999939, 685731524, -1296481414, 1815115025, 1461364854,
    1193448329, 667779376, 924765563, -183768477, -1015784978,
    1445662585, 438989805, 398340369, 1631775357, 415393687,
    1541804686, -655006701, 1477278577, -1701983741, 1136108454,
    -828377729, 186618211, -1160793136, 1973214822, -1991885179,
    536124280, -115466932, -333738847, 1625792787, 338444264,
    -1923970831, 1259191105, -732701390, -1594979922, -1638445190,
    -491981009, -593964351, 1553210608, -1815258689, 825873196,
    -1268854288, 298737106, 196814233, -1454862951, 978815630,
    -974664054, 1242911821, -63473076, 342703921, -621405658,
};

static const int8_t golden_eta4[NTESTS][4] = {
    {3, 2, 5, -5},
    {3, 0, -1, 0},
    {-4, -8, -5, 2},
    {-2, -1, -4, -1},
    {-3, -10, -3, -9},
    {4, -3, 3, -5},
    {9, 9, -2, 0},
    {4, -5, -1, 9},
    {4, -5, 8, 4},
    {5, -1, 4, 1},
    {4, 2, -10, -7},
    {8, -2, -8, 11},
    {-12, 9, 1, 6},
    {-4, -4, -7, 2},
    {2, -3, 2, 2},
    {-9, 8, -10, 11},
    {1, 1, 2, 6},
    {-5, -11, -3, 1},
    {-1, -6, 1, 1},
    {-3, 14, 10, 9},
    {7, 11, 9, 3},
    {-8, 0, 3, 2},
    {-5, -13, -1, -10},
    {1, 2, 4, 3},
    {-6, 6, -3, 5},
    {3, -4, 8, -2},
    {0, 0, 3, 6},
    {-12, -6, 0, 8},
    {8, -2, -10, 7},
    {5, 3, -3, 6},
    {3, -2, -7, 4},
    {7, 0, 7, 8},
    {-12, -9, 9, -7},
    {0, -8, 12, 1},
    {-8, 15, -1, 6},
    {-4, -11, 3, 7},
    {7, 6, -7, -1},
    {-10, 15, 9, -4},
    {1, -10, 9, -4},
    {4, 8, 2, -8},
    {-9, 4, -7, 3},
    {-6, -4, -1, 5},
    {-14, -1, 8, 5},
    {8, 8, 0, -5},
    {2, 12, -3, -3},
    {-9, -7, 4, 3},
    {3, -8, 3, 3},
    {7, 0, 2, 1},
    {14, -7, -1, 1},
    {11, 0, -3, -10},
    {2, 9, 6, 2},
    {-8, 8, 2, 6},
    {3, -1, -2, -9},
    {0, -2, -3, 6},
    {-1, -2, 9, 2},
    {1, -6, 0, 3},
    {-3, -7, 1, 5},
    {4, 1, 13, 4},
    {-3, -4, 11, -10},
    {0, 4, -3, -9},
    {2, 15, 9, 1},
    {-1, -4, 8, 9},
    {0, -2, 3, 6},
    {6, -4, -2, -5},
    {-2, 1, -10, 7},
    {6, 1, -8, 6},
    {2, -2, -10, -5},
    {-6, 4, 13, 3},
    {3, -2, 5, 1},
    {-4, -7, -4, -1},
    {8, -10, 6, 2},
    {-3, -8, 14, 11},
    {-13, -8, 3, -1},
    {0, 1, 3, -2},
    {-3, 11, 2, 1},
    {1, 2, -11, 14},
    {8, 12, 13, -6},
    {-9, 4, 10, -2},
    {2, -9, -7, -6},
    {-6, 12, 10, 3},
    {-14, 0, -3, 5},
    {-3, 0, 13, 7},
    {-1, 0, -2, -9},
    {4, 1, 0, -10},
    {3, 11, 2, 5},
    {13, -8, 2, -12},
    {-11, -11, -1, -1},
    {-15, 11, -5, 7},
    {4, 4, 1, -6},
    {10, -10, 6, -2},
    {-15, -12, 9, -7},
    {-11, 8, 2, 0},
    {0, 3, 0, 11},
    {0, 5, 4, -1},
    {6, 2, 2, 7},
    {2, 2, -7, -7},
    {9, -1, 4, 6},
    {8, 3, 4, -3},
    {-2, 12, 7, 3},
    {4, 9, -9, -3}
    };


int main(void) {
    int8_t out_sw[NTESTS][NLANES] = {0};
    int8_t out_hw[NTESTS][NLANES] = {0};

    int all_pass_sw = 1;
    int all_pass_hw = 1;

    unsigned cycles_cbd_sw = 0;
#if SW_TEST_ENABLED
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    for (int i = 0; i < NTESTS; i++) {
        cbd_eta4(d_eta4[i], out_sw, i);
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_cbd_sw);
    printf("Software Cycles: %u\n", cycles_cbd_sw);

    for (int i = 0; i < NTESTS; i++) {
        for (int j = 0; j < NLANES; j++) {
            if (out_sw[i][j] != golden_eta4[i][j]) {
                printf("  [SW][%2d][%d] FAIL: d=%d, exp=%d, got=%d\n", i, j, d_eta4[i], golden_eta4[i][j], out_sw[i][j]);
                all_pass_sw = 0;
            }
        }
    }
#else
    printf("--- SOFTWARE TEST SKIPPED ---\n\n");
#endif

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    unsigned cycles_cbd_hw;
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    for (int i = 0; i < NTESTS; i++) {
        asm volatile (
            "mv a3, %[src]\n\t"
            ".insn r 0x3b, 0x7, 0x3, %[dst0], %[src], %[x0]\r\n"
            ".insn r 0x3b, 0x7, 0x3, %[dst1], %[src], %[x1]\r\n"
            :   [dst0] "=r" (out_hw[i][0]),
                [dst1] "=r" (out_hw[i][1])
            :   [src] "r" (d_eta4[i]),
                [x0] "r" (0),
                [x1] "r" (1)
            :   "a3", "cc");
        asm volatile (".insn r 0x3b, 0x7, 0x3, %[dst], %[src], %[x]\r\n" : [dst] "=r" (out_hw[i][2]) : [src] "r" (d_eta4[i]), [x] "r" (2) : );
        asm volatile (".insn r 0x3b, 0x7, 0x3, %[dst], %[src], %[x]\r\n" : [dst] "=r" (out_hw[i][3]) : [src] "r" (d_eta4[i]), [x] "r" (3) : );
    }

    CSR_READ(CSR_REG_MCYCLE, &cycles_cbd_hw);
    printf("Hardware Cycles: %u\n", cycles_cbd_hw);


    for (int i = 0; i < NTESTS; i++) {
        for (int j = 0; j < NLANES; j++) {
            if (out_hw[i][j] != golden_eta4[i][j]) {
                printf("  [HW][%2d][%d] FAIL: d=%d, exp=%d, got=%d\n", i, j, d_eta4[i], golden_eta4[i][j], out_hw[i][j]);
                all_pass_hw = 0;
            }
        }
    }

    if (cycles_cbd_sw > 0 && cycles_cbd_hw > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", cycles_cbd_sw, cycles_cbd_hw);
        printf("Speedup: %u.%02ux\n", cycles_cbd_sw / cycles_cbd_hw, ((cycles_cbd_sw * 100) / cycles_cbd_hw) % 100);
    }

    if (all_pass_sw && all_pass_hw) {
        printf("All suites passed.\n");
    } else {
        printf("Some tests FAILED.\n");
    }

    return 0;
}
