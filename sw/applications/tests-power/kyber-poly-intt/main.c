#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "kyber.h"
#include "core_v_mini_mcu.h"
#include "csr.h"
#include "vcd_util.h"

#ifndef SW_TEST_ENABLED
#define SW_TEST_ENABLED 1
#endif

#define DEBUG 0

static void init_fixed_polyvec(kyber_polyvec *v, int16_t salt)
{
    for (int i = 0; i < KEM_K; i++) {
        for (int j = 0; j < KEM_N; j++) {
            int32_t x = (int32_t)(i * 257 + j * 73 + salt * 19 + 123);
            x %= (8 * KEM_Q);
            x -= (4 * KEM_Q);
            v->vec[i].coeffs[j] = (int16_t)x;
        }
    }
}

static int compare_polyvec(const char *test_name, const kyber_polyvec *got, const kyber_polyvec *exp)
{
    int ok = 1;
    for (int i = 0; i < KEM_K; i++) {
        for (int j = 0; j < KEM_N; j++) {
            int16_t g = got->vec[i].coeffs[j];
            int16_t e = exp->vec[i].coeffs[j];
            if (g != e) {
                ok = 0;
#if DEBUG
                printf("[%s] Mismatch vec=%d idx=%d got=%d exp=%d\n", test_name, i, j, g, e);
#else
                return 0;
#endif
            }
        }
    }
    return ok;
}

int main(void)
{
    unsigned int cycles_sw = 0;
    unsigned int cycles_hw = 0;
    int all_passed = 1;

    kyber_polyvec in_a;
    kyber_polyvec ntt_in;
    kyber_polyvec sw_out, hw_out, golden;

    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);

    init_fixed_polyvec(&in_a, 1);

    ntt_in = in_a;
    kyber_polyvec_ntt_sw(&ntt_in);

#if SW_TEST_ENABLED
    sw_out = ntt_in;
    if (vcd_init() != 0)
        return 1;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    vcd_enable();
    kyber_polyvec_invntt_sw(&sw_out);
    vcd_disable();
    CSR_READ(CSR_REG_MCYCLE, &cycles_sw);
    golden = sw_out;

    if (!compare_polyvec("polyvec_invntt SW->golden", &sw_out, &golden)) {
        all_passed = 0;
    }
#else
    golden = ntt_in;
#endif

    hw_out = ntt_in;
    if (vcd_init() != 0)
        return 1;
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    vcd_enable();
    kyber_polyvec_invntt_hw(&hw_out);
    vcd_disable();
    CSR_READ(CSR_REG_MCYCLE, &cycles_hw);

#if SW_TEST_ENABLED
    if (!compare_polyvec("polyvec_invntt HW->golden", &hw_out, &golden)) {
        all_passed = 0;
    }
#endif

    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
