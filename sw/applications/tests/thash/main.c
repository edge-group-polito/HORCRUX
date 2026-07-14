#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "core_v_mini_mcu.h"
#include "csr.h"
#include "../sphincs_ref_impl.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

typedef struct {
    const char *name;
    size_t n;
    int simple_mode;
} thash_variant_t;

static const thash_variant_t k_variants[] = {
    {"SPHINCS+-128f-robust", 16, 0},
    {"SPHINCS+-128f-simple", 16, 1},
    {"SPHINCS+-192f-robust", 24, 0},
    {"SPHINCS+-192f-simple", 24, 1},
    {"SPHINCS+-256f-robust", 32, 0},
    {"SPHINCS+-256f-simple", 32, 1},
};

static int run_thash1_case(const thash_variant_t *v, uint32_t *sw_cycles, uint32_t *hw_cycles, uint32_t seed) {
    uint8_t pub_seed[32] = {0};
    uint8_t addr[SPX_ADDR_BYTES] = {0};
    uint8_t in[32] = {0};
    uint8_t out_sw[32] = {0};
    uint8_t out_hw[32] = {0};
    uint8_t funct7 = spx_thash1_funct7_for_n(v->n);
    uint32_t c_sw = 0;
    uint32_t c_hw = 0;

    spx_fill_bytes(pub_seed, v->n, (uint32_t)(0x4000 + seed + v->n + (v->simple_mode * 11)));
    spx_fill_bytes(addr, SPX_ADDR_BYTES, (uint32_t)(0x5000 + seed + v->n + (v->simple_mode * 19)));
    spx_fill_bytes(in, v->n, (uint32_t)(0x6000 + seed + v->n + (v->simple_mode * 23)));

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    spx_ref_thash(out_sw, in, 1, pub_seed, addr, v->n, v->simple_mode);
    CSR_READ(CSR_REG_MCYCLE, &c_sw);
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    if (v->simple_mode) {
        __asm__ volatile("li t0, 1" ::: "t0", "memory");
    }
    spx_hw_exec(out_hw, pub_seed, addr, in, v->n, v->n, funct7, v->simple_mode);
    CSR_READ(CSR_REG_MCYCLE, &c_hw);

    if (sw_cycles != NULL) {
        *sw_cycles = c_sw;
    }
    if (hw_cycles != NULL) {
        *hw_cycles = c_hw;
    }

#if SW_TEST_ENABLED
    if (spx_compare_arrays(out_sw, out_hw, v->n) != 0) {
        return 1;
    }
#endif
    return 0;
}

int main(void) {
    int failures = 0;
    uint32_t sw_total = 0;
    uint32_t hw_total = 0;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);

    for (size_t i = 0; i < sizeof(k_variants) / sizeof(k_variants[0]); i++) {
        for (uint32_t tc = 0; tc < 2; tc++) {
            uint32_t c_sw = 0;
            uint32_t c_hw = 0;
            int err = run_thash1_case(&k_variants[i], &c_sw, &c_hw, tc);

            sw_total += c_sw;
            hw_total += c_hw;

            printf("%s thash1 case%u: %s", k_variants[i].name, tc + 1, err ? "FAIL" : "PASS");
#if SW_TEST_ENABLED
            printf(" | SW=%u HW=%u", c_sw, c_hw);
#else
            printf(" | HW=%u", c_hw);
#endif
            printf("\n");

            failures += err;
        }
    }

#if SW_TEST_ENABLED
    if (sw_total > 0 && hw_total > 0) {
        printf("SW Cycles: %u | HW Cycles: %u\n", sw_total, hw_total);
        printf("Speedup: %u.%02ux\n", sw_total / hw_total, ((sw_total * 100) / hw_total) % 100);
    }
#endif

    if (failures == 0) {
        printf("FINAL STATUS: ALL TESTS PASSED\n");
    } else {
        printf("FINAL STATUS: %d TEST(S) FAILED\n", failures);
    }

    return failures;
}
