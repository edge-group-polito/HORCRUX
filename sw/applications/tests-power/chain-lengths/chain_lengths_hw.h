#ifndef CHAIN_LENGTHS_HW_H
#define CHAIN_LENGTHS_HW_H

#include <stdint.h>

/**
 * Load data into horcrux register file
 * Writes 2 consecutive 32-bit words to register file
 * 
 * @param rs_low Lower 32-bit word
 * @param rs_high Upper 32-bit word
 * @param rs_index Register file index (0-49)
 */
static inline void cus_load(uint32_t rs_low, uint32_t rs_high, uint32_t rs_index) {
    asm volatile(
        ".insn r 0x6b, 0x01, 0x03, x0, %0, %1, %2"
        :
        : "r"(rs_low), "r"(rs_high), "r"(rs_index)
    );
}

/**
 * Trigger chain_lengths computation for 128f variant (LEN1=32, LEN2=3)
 */
static inline void cl_compute_128f(void) {
    asm volatile(
        ".insn r 0x3b, 0x7, 0x1B, x0, x0, x0"
        :
        :
    );
}

/**
 * Read result from chain_lengths computation
 * 
 * @param rs_index Register file index to read from
 * @return 32-bit word from result array
 */
static inline uint32_t cus_store(uint32_t rs_index) {
    uint32_t result;
    asm volatile(
        ".insn r 0x3b, 0x7, 0xF, %0, %1, x0"
        : "=&r"(result)
        : "r"(rs_index)
    );
    return result;
}

/**
 * Optimized hardware chain_lengths for 128f variant (16 bytes, 32 nibbles + 3 checksum)
 */
static inline void chain_lengths_hw_128f(uint8_t *lengths, const uint32_t *msg32) {
    // Unrolled loads - 4 words = 2 load instructions
    cus_load(msg32[0], msg32[1], 0);
    cus_load(msg32[2], msg32[3], 2);
    
    // Fence to ensure loads complete before compute
    asm volatile("fence" ::: "memory");
    
    cl_compute_128f();
    
    // Unrolled stores with optimized unpacking - 5 words for 35 nibbles
    uint32_t w0 = cus_store(0);
    uint32_t w1 = cus_store(1);
    uint32_t w2 = cus_store(2);
    uint32_t w3 = cus_store(3);
    uint32_t w4 = cus_store(4);
    
    // Unpack nibbles (8 per word, MSB first)
    lengths[0] = w0 >> 28; lengths[1] = (w0 >> 24) & 0xF; lengths[2] = (w0 >> 20) & 0xF; lengths[3] = (w0 >> 16) & 0xF;
    lengths[4] = (w0 >> 12) & 0xF; lengths[5] = (w0 >> 8) & 0xF; lengths[6] = (w0 >> 4) & 0xF; lengths[7] = w0 & 0xF;
    
    lengths[8] = w1 >> 28; lengths[9] = (w1 >> 24) & 0xF; lengths[10] = (w1 >> 20) & 0xF; lengths[11] = (w1 >> 16) & 0xF;
    lengths[12] = (w1 >> 12) & 0xF; lengths[13] = (w1 >> 8) & 0xF; lengths[14] = (w1 >> 4) & 0xF; lengths[15] = w1 & 0xF;
    
    lengths[16] = w2 >> 28; lengths[17] = (w2 >> 24) & 0xF; lengths[18] = (w2 >> 20) & 0xF; lengths[19] = (w2 >> 16) & 0xF;
    lengths[20] = (w2 >> 12) & 0xF; lengths[21] = (w2 >> 8) & 0xF; lengths[22] = (w2 >> 4) & 0xF; lengths[23] = w2 & 0xF;
    
    lengths[24] = w3 >> 28; lengths[25] = (w3 >> 24) & 0xF; lengths[26] = (w3 >> 20) & 0xF; lengths[27] = (w3 >> 16) & 0xF;
    lengths[28] = (w3 >> 12) & 0xF; lengths[29] = (w3 >> 8) & 0xF; lengths[30] = (w3 >> 4) & 0xF; lengths[31] = w3 & 0xF;
    
    lengths[32] = w4 >> 28; lengths[33] = (w4 >> 24) & 0xF; lengths[34] = (w4 >> 20) & 0xF;
}

#endif // CHAIN_LENGTHS_HW_H
