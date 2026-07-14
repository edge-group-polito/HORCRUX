#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"


typedef struct {
    uint32_t rd_lo; 
    uint32_t rd_hi;
} bfu_result_t;



#define ntt DILITHIUM_NAMESPACE(ntt)
void ntt(int32_t a[N]);

#define invntt_tomont DILITHIUM_NAMESPACE(invntt_tomont)
void invntt_tomont(int32_t a[N]);

#endif
