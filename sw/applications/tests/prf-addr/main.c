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
} prf_variant_t;

static const prf_variant_t k_variants[] = {
    {"SPHINCS+-128f-robust", 16, 0},
    {"SPHINCS+-128f-simple", 16, 1},
    {"SPHINCS+-192f-robust", 24, 0},
    {"SPHINCS+-192f-simple", 24, 1},
    {"SPHINCS+-256f-robust", 32, 0},
    {"SPHINCS+-256f-simple", 32, 1},
};

static int run_prf_case(const prf_variant_t *v, uint32_t *sw_cycles, uint32_t *hw_cycles) {
    uint8_t pub_seed[32] = {0};
    uint8_t sk_seed[32] = {0};
    uint8_t addr[SPX_ADDR_BYTES] = {0};
    uint8_t out_sw[32] = {0};
    uint8_t out_hw[32] = {0};
    uint8_t payload[32] = {0};
    uint8_t funct7 = spx_prf_funct7_for_n(v->n);
    uint32_t c_sw = 0;
    uint32_t c_hw = 0;

    spx_fill_bytes(pub_seed, v->n, (uint32_t)(0x1000 + v->n + (v->simple_mode * 17)));
    spx_fill_bytes(sk_seed, v->n, (uint32_t)(0x2000 + v->n + (v->simple_mode * 31)));
    spx_fill_bytes(addr, SPX_ADDR_BYTES, (uint32_t)(0x3000 + v->n + (v->simple_mode * 13)));
    memcpy(payload, sk_seed, v->n);

#if SW_TEST_ENABLED
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    spx_ref_prf(out_sw, pub_seed, sk_seed, addr, v->n);
    CSR_READ(CSR_REG_MCYCLE, &c_sw);
#endif

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    spx_hw_exec(out_hw, pub_seed, addr, payload, v->n, v->n, funct7, 0);
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
        uint32_t c_sw = 0;
        uint32_t c_hw = 0;
        int err = run_prf_case(&k_variants[i], &c_sw, &c_hw);

        sw_total += c_sw;
        hw_total += c_hw;

        printf("%s: %s", k_variants[i].name, err ? "FAIL" : "PASS");
#if SW_TEST_ENABLED
        printf(" | SW=%u HW=%u", c_sw, c_hw);
#else
        printf(" | HW=%u", c_hw);
#endif
        printf("\n");

        failures += err;
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
