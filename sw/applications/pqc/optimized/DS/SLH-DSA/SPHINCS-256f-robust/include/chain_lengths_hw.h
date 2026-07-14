#ifndef CHAIN_LENGTHS_HW_H
#define CHAIN_LENGTHS_HW_H

#include <stdint.h>

static inline uint32_t cl_load32_le(const uint8_t *x) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

// Hardware instruction encodings for chain_lengths variants
typedef enum {
    CL_128F = 0,  // funct7 = 0x1B
    CL_192F = 1,  // funct7 = 0x1C
    CL_256F = 2   // funct7 = 0x1D
} cl_variant_t;

/**
 * Load data into horcrux register file
 * Writes 2 consecutive 32-bit words to register file
 * Uses R4-type instruction format: rs3 (bits 31:27) = register index
 * 
 * @param rs_low Lower 32-bit word (rs1)
 * @param rs_high Upper 32-bit word (rs2)
 * @param rs_index Register file index (rs3, 0-49)
 */
static inline void cus_load(uint32_t rs_low, uint32_t rs_high, uint32_t rs_index) {
    asm volatile(
        ".insn r4 0x6b, 0x1, 0x3, x0, %0, %1, %2"
        :
        : "r"(rs_low), "r"(rs_high), "r"(rs_index)
    );
}

/**
 * Trigger chain_lengths computation for 128f variant (LEN1=32, LEN2=3)
 * funct7 = 0x1B (matches instr decode table in horcrux_pkg.sv)
 */
static inline void cl_compute_128f(void) {
    asm volatile(
        ".insn r 0x3b, 0x7, 0x1B, x0, x0, x0"
        :
        :
    );
}

/**
 * Trigger chain_lengths computation for 192f variant (LEN1=48, LEN2=3)
 * funct7 = 0x1C (matches instr decode table in horcrux_pkg.sv)
 */
static inline void cl_compute_192f(void) {
    asm volatile(
        ".insn r 0x3b, 0x7, 0x1C, x0, x0, x0"
        :
        :
    );
}

/**
 * Trigger chain_lengths computation for 256f variant (LEN1=64, LEN2=3)
 * funct7 = 0x1D (matches instr decode table in horcrux_pkg.sv)
 */
static inline void cl_compute_256f(void) {
    asm volatile(
        ".insn r 0x3b, 0x7, 0x1D, x0, x0, x0"
        :
        :
    );
}

/**
 * Read result from chain_lengths computation
 * funct7 = 0xF = store_horcrux (matches horcrux_pkg.sv: 32'b001111...)
 * 
 * @param rs_index Register file index to read from
 * @return 32-bit word from result array
 */
static inline uint32_t cus_store(uint32_t rs_index) {
    uint32_t result;
    asm volatile(
        ".insn r 0x3b, 0x7, 0xF, %0, %1, x0"
        : "=r"(result)
        : "r"(rs_index)
    );
    return result;
}

/**
 * Optimized hardware chain_lengths for 128f variant (16 bytes, 32 nibbles + 3 checksum)
 */
static inline void chain_lengths_hw_128f(uint8_t *lengths, const uint8_t *msg) {
    // Unrolled loads - 4 words = 2 load instructions
    cus_load(cl_load32_le(msg + 0), cl_load32_le(msg + 4), 0);
    cus_load(cl_load32_le(msg + 8), cl_load32_le(msg + 12), 2);
    
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

/**
 * Optimized hardware chain_lengths for 192f variant (24 bytes, 48 nibbles + 3 checksum)
 */
static inline void chain_lengths_hw_192f(uint8_t *lengths, const uint8_t *msg) {
    // Unrolled loads - 6 words = 3 load instructions
    cus_load(cl_load32_le(msg + 0), cl_load32_le(msg + 4), 0);
    cus_load(cl_load32_le(msg + 8), cl_load32_le(msg + 12), 2);
    cus_load(cl_load32_le(msg + 16), cl_load32_le(msg + 20), 4);
    
    // Fence to ensure loads complete before compute
    asm volatile("fence" ::: "memory");
    
    cl_compute_192f();
    
    // Unrolled stores - 7 words for 51 nibbles
    uint32_t w0 = cus_store(0);
    uint32_t w1 = cus_store(1);
    uint32_t w2 = cus_store(2);
    uint32_t w3 = cus_store(3);
    uint32_t w4 = cus_store(4);
    uint32_t w5 = cus_store(5);
    uint32_t w6 = cus_store(6);
    
    // Unpack nibbles
    lengths[0] = w0 >> 28; lengths[1] = (w0 >> 24) & 0xF; lengths[2] = (w0 >> 20) & 0xF; lengths[3] = (w0 >> 16) & 0xF;
    lengths[4] = (w0 >> 12) & 0xF; lengths[5] = (w0 >> 8) & 0xF; lengths[6] = (w0 >> 4) & 0xF; lengths[7] = w0 & 0xF;
    
    lengths[8] = w1 >> 28; lengths[9] = (w1 >> 24) & 0xF; lengths[10] = (w1 >> 20) & 0xF; lengths[11] = (w1 >> 16) & 0xF;
    lengths[12] = (w1 >> 12) & 0xF; lengths[13] = (w1 >> 8) & 0xF; lengths[14] = (w1 >> 4) & 0xF; lengths[15] = w1 & 0xF;
    
    lengths[16] = w2 >> 28; lengths[17] = (w2 >> 24) & 0xF; lengths[18] = (w2 >> 20) & 0xF; lengths[19] = (w2 >> 16) & 0xF;
    lengths[20] = (w2 >> 12) & 0xF; lengths[21] = (w2 >> 8) & 0xF; lengths[22] = (w2 >> 4) & 0xF; lengths[23] = w2 & 0xF;
    
    lengths[24] = w3 >> 28; lengths[25] = (w3 >> 24) & 0xF; lengths[26] = (w3 >> 20) & 0xF; lengths[27] = (w3 >> 16) & 0xF;
    lengths[28] = (w3 >> 12) & 0xF; lengths[29] = (w3 >> 8) & 0xF; lengths[30] = (w3 >> 4) & 0xF; lengths[31] = w3 & 0xF;
    
    lengths[32] = w4 >> 28; lengths[33] = (w4 >> 24) & 0xF; lengths[34] = (w4 >> 20) & 0xF; lengths[35] = (w4 >> 16) & 0xF;
    lengths[36] = (w4 >> 12) & 0xF; lengths[37] = (w4 >> 8) & 0xF; lengths[38] = (w4 >> 4) & 0xF; lengths[39] = w4 & 0xF;
    
    lengths[40] = w5 >> 28; lengths[41] = (w5 >> 24) & 0xF; lengths[42] = (w5 >> 20) & 0xF; lengths[43] = (w5 >> 16) & 0xF;
    lengths[44] = (w5 >> 12) & 0xF; lengths[45] = (w5 >> 8) & 0xF; lengths[46] = (w5 >> 4) & 0xF; lengths[47] = w5 & 0xF;
    
    lengths[48] = w6 >> 28; lengths[49] = (w6 >> 24) & 0xF; lengths[50] = (w6 >> 20) & 0xF;
}

/**
 * Optimized hardware chain_lengths for 256f variant (32 bytes, 64 nibbles + 3 checksum)
 */
static inline void chain_lengths_hw_256f(uint8_t *lengths, const uint8_t *msg) {
    // Unrolled loads - 8 words = 4 load instructions
    cus_load(cl_load32_le(msg + 0), cl_load32_le(msg + 4), 0);
    cus_load(cl_load32_le(msg + 8), cl_load32_le(msg + 12), 2);
    cus_load(cl_load32_le(msg + 16), cl_load32_le(msg + 20), 4);
    asm volatile(
        "addi t0, %[rs1], 0\n"  
        "addi t1, %[rs2], 0\n"
        ".insn r 0x6b, 0x01, 0x03, x0, t0, t1, %[index]\n"
        :
        : [rs1] "r"(cl_load32_le(msg + 24)), [rs2] "r"(cl_load32_le(msg + 28)), [index] "r"(6)
    );
    //cus_load(msg32[6], msg32[7], 6);
    
    // Fence to ensure loads complete before compute
    asm volatile("fence" ::: "memory");
    cl_compute_256f();
    
    // Unrolled stores - 9 words for 67 nibbles
    uint32_t w0 = cus_store(0);
    uint32_t w1 = cus_store(1);
    uint32_t w2 = cus_store(2);
    uint32_t w3 = cus_store(3);
    uint32_t w4 = cus_store(4);
    uint32_t w5 = cus_store(5);
    uint32_t w6 = cus_store(6);
    uint32_t w7 = cus_store(7);
    uint32_t w8 = cus_store(8);
    
    // Unpack nibbles
    lengths[0] = w0 >> 28; lengths[1] = (w0 >> 24) & 0xF; lengths[2] = (w0 >> 20) & 0xF; lengths[3] = (w0 >> 16) & 0xF;
    lengths[4] = (w0 >> 12) & 0xF; lengths[5] = (w0 >> 8) & 0xF; lengths[6] = (w0 >> 4) & 0xF; lengths[7] = w0 & 0xF;
    
    lengths[8] = w1 >> 28; lengths[9] = (w1 >> 24) & 0xF; lengths[10] = (w1 >> 20) & 0xF; lengths[11] = (w1 >> 16) & 0xF;
    lengths[12] = (w1 >> 12) & 0xF; lengths[13] = (w1 >> 8) & 0xF; lengths[14] = (w1 >> 4) & 0xF; lengths[15] = w1 & 0xF;
    
    lengths[16] = w2 >> 28; lengths[17] = (w2 >> 24) & 0xF; lengths[18] = (w2 >> 20) & 0xF; lengths[19] = (w2 >> 16) & 0xF;
    lengths[20] = (w2 >> 12) & 0xF; lengths[21] = (w2 >> 8) & 0xF; lengths[22] = (w2 >> 4) & 0xF; lengths[23] = w2 & 0xF;
    
    lengths[24] = w3 >> 28; lengths[25] = (w3 >> 24) & 0xF; lengths[26] = (w3 >> 20) & 0xF; lengths[27] = (w3 >> 16) & 0xF;
    lengths[28] = (w3 >> 12) & 0xF; lengths[29] = (w3 >> 8) & 0xF; lengths[30] = (w3 >> 4) & 0xF; lengths[31] = w3 & 0xF;
    
    lengths[32] = w4 >> 28; lengths[33] = (w4 >> 24) & 0xF; lengths[34] = (w4 >> 20) & 0xF; lengths[35] = (w4 >> 16) & 0xF;
    lengths[36] = (w4 >> 12) & 0xF; lengths[37] = (w4 >> 8) & 0xF; lengths[38] = (w4 >> 4) & 0xF; lengths[39] = w4 & 0xF;
    
    lengths[40] = w5 >> 28; lengths[41] = (w5 >> 24) & 0xF; lengths[42] = (w5 >> 20) & 0xF; lengths[43] = (w5 >> 16) & 0xF;
    lengths[44] = (w5 >> 12) & 0xF; lengths[45] = (w5 >> 8) & 0xF; lengths[46] = (w5 >> 4) & 0xF; lengths[47] = w5 & 0xF;
    
    lengths[48] = w6 >> 28; lengths[49] = (w6 >> 24) & 0xF; lengths[50] = (w6 >> 20) & 0xF; lengths[51] = (w6 >> 16) & 0xF;
    lengths[52] = (w6 >> 12) & 0xF; lengths[53] = (w6 >> 8) & 0xF; lengths[54] = (w6 >> 4) & 0xF; lengths[55] = w6 & 0xF;
    
    lengths[56] = w7 >> 28; lengths[57] = (w7 >> 24) & 0xF; lengths[58] = (w7 >> 20) & 0xF; lengths[59] = (w7 >> 16) & 0xF;
    lengths[60] = (w7 >> 12) & 0xF; lengths[61] = (w7 >> 8) & 0xF; lengths[62] = (w7 >> 4) & 0xF; lengths[63] = w7 & 0xF;
    
    lengths[64] = w8 >> 28; lengths[65] = (w8 >> 24) & 0xF; lengths[66] = (w8 >> 20) & 0xF;
}

#endif // CHAIN_LENGTHS_HW_H
