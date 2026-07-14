#ifndef BFU_H
#define BFU_H

#include <stdint.h>
#include "dilithium.h"

// Emulate a 32-bit register pair (rd_lo, rd_hi)
typedef struct {
    uint32_t rd_lo; 
    uint32_t rd_hi;
} bfu_result_t;

#endif
