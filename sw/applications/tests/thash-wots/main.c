#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "core_v_mini_mcu.h"
#include "csr.h"
#include "../sphincs_ref_impl.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define SPX_OFFSET_CHAIN_ADDR 27
#define SPX_OFFSET_HASH_ADDR  31

typedef struct {
    const char *name;
    size_t n;
    int simple_mode;
    size_t wots_len;
} wots_variant_t;

static const wots_variant_t k_variants[] = {
    {"SPHINCS+-128f-robust", 16, 0, 35},
    {"SPHINCS+-128f-simple", 16, 1, 35},
    {"SPHINCS+-192f-robust", 24, 0, 51},
    {"SPHINCS+-192f-simple", 24, 1, 51},
    {"SPHINCS+-256f-robust", 32, 0, 67},
    {"SPHINCS+-256f-simple", 32, 1, 67},
};

static int run_thash1_once(const wots_variant_t *v, uint32_t *sw_cycles, uint32_t *hw_cycles, uint32_t seed) {
    uint8_t pub_seed[32] = {0};
    uint8_t addr[SPX_ADDR_BYTES] = {0};
    uint8_t in[32] = {0};
    uint8_t out_sw[32] = {0};
    uint8_t out_hw[32] = {0};
    uint8_t funct7 = spx_thash1_funct7_for_n(v->n);
    uint32_t c_sw = 0;
    uint32_t c_hw = 0;

    spx_fill_bytes(pub_seed, v->n, (uint32_t)(0xA000 + seed + v->n + (v->simple_mode * 5)));
    spx_fill_bytes(addr, SPX_ADDR_BYTES, (uint32_t)(0xB000 + seed + v->n + (v->simple_mode * 7)));
    spx_fill_bytes(in, v->n, (uint32_t)(0xC000 + seed + v->n + (v->simple_mode * 11)));

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

static int run_wots_chain(const wots_variant_t *v, uint32_t *sw_cycles, uint32_t *hw_cycles) {
    uint8_t pub_seed[32] = {0};
    uint8_t addr_sw[SPX_ADDR_BYTES] = {0};
    uint8_t addr_hw[SPX_ADDR_BYTES] = {0};
    uint8_t state_sw[32] = {0};
    uint8_t state_hw[32] = {0};
    uint8_t funct7 = spx_thash1_funct7_for_n(v->n);
    uint32_t c_sw = 0;
    uint32_t c_hw = 0;
    const uint32_t chain_steps = 15;

    spx_fill_bytes(pub_seed, v->n, (uint32_t)(0xD000 + v->n + (v->simple_mode * 17)));
    spx_fill_bytes(addr_sw, SPX_ADDR_BYTES, (uint32_t)(0xE000 + v->n + (v->simple_mode * 19)));
    memcpy(addr_hw, addr_sw, SPX_ADDR_BYTES);
    spx_fill_bytes(state_sw, v->n, (uint32_t)(0xF000 + v->n + (v->simple_mode * 23)));
    memcpy(state_hw, state_sw, v->n);

    addr_sw[SPX_OFFSET_CHAIN_ADDR] = 3;
    addr_hw[SPX_OFFSET_CHAIN_ADDR] = 3;

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (uint32_t i = 0; i < chain_steps; i++) {
        addr_sw[SPX_OFFSET_HASH_ADDR] = (uint8_t)i;
        spx_ref_thash(state_sw, state_sw, 1, pub_seed, addr_sw, v->n, v->simple_mode);
    }
    CSR_READ(CSR_REG_MCYCLE, &c_sw);
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (uint32_t i = 0; i < chain_steps; i++) {
        addr_hw[SPX_OFFSET_HASH_ADDR] = (uint8_t)i;
        if (v->simple_mode) {
            __asm__ volatile("li t0, 1" ::: "t0", "memory");
        }
        spx_hw_exec(state_hw, pub_seed, addr_hw, state_hw, v->n, v->n, funct7, v->simple_mode);
    }
    CSR_READ(CSR_REG_MCYCLE, &c_hw);

    if (sw_cycles != NULL) {
        *sw_cycles = c_sw;
    }
    if (hw_cycles != NULL) {
        *hw_cycles = c_hw;
    }

#if SW_TEST_ENABLED
    if (spx_compare_arrays(state_sw, state_hw, v->n) != 0) {
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
        uint32_t sw1 = 0, hw1 = 0;
        uint32_t sw2 = 0, hw2 = 0;
        int e1 = run_thash1_once(&k_variants[i], &sw1, &hw1, 0);
        int e2 = run_wots_chain(&k_variants[i], &sw2, &hw2);

        sw_total += (sw1 + sw2);
        hw_total += (hw1 + hw2);
        failures += e1 + e2;

        printf("%s thash1: %s", k_variants[i].name, e1 ? "FAIL" : "PASS");
#if SW_TEST_ENABLED
        printf(" | SW=%u HW=%u", sw1, hw1);
#else
        printf(" | HW=%u", hw1);
#endif
        printf("\n");

        printf("%s wots-chain(%u): %s", k_variants[i].name, (unsigned)k_variants[i].wots_len, e2 ? "FAIL" : "PASS");
#if SW_TEST_ENABLED
        printf(" | SW=%u HW=%u", sw2, hw2);
#else
        printf(" | HW=%u", hw2);
#endif
        printf("\n");
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
