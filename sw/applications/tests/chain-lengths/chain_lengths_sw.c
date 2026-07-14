#include "chain_lengths_sw.h"

#define SPX_WOTS_W 16
#define SPX_WOTS_LOGW 4

/**
 * base_w algorithm for W = 16
 * Extracts 4-bit nibbles from input message
 * 
 * @param output Output array for nibbles
 * @param out_len Number of nibbles to extract
 * @param input Input message bytes
 */
static void base_w(uint8_t *output, uint32_t out_len, const uint8_t *input) {
    uint32_t in = 0;
    uint32_t out = 0;
    uint32_t total = 0;
    uint32_t bits = 0;
    uint32_t consumed;

    for (consumed = 0; consumed < out_len; consumed++) {
        if (bits == 0) {
            total = input[in];
            in++;
            bits += 8;
        }
        bits -= SPX_WOTS_LOGW;
        output[out] = (uint8_t)((total >> bits) & (SPX_WOTS_W - 1));
        out++;
    }
}

/**
 * Computes checksum over nibbles and writes checksum nibbles
 * 
 * @param csum_output Output array for checksum nibbles (len2 elements)
 * @param lengths Input array of nibbles to checksum (len1 elements)
 * @param len1 Number of nibbles in input
 * @param len2 Number of checksum nibbles to produce
 */
static void wots_checksum(uint8_t *csum_output, const uint8_t *lengths, uint32_t len1, uint32_t len2) {
    uint32_t csum = 0;

    // Compute sum
    for (uint32_t i = 0; i < len1; i++) {
        csum += SPX_WOTS_W - 1 - lengths[i];
    }

    // Shift checksum left by 4 bits (LEN2 == 3 for all our variants)
    csum = csum << (8 - ((len2 * SPX_WOTS_LOGW) % 8));

    // Convert checksum to base_w using len2 nibbles
    uint32_t len_2_bytes = (len2 * SPX_WOTS_LOGW + 7) / 8;
    uint8_t csum_bytes[4];
    
    csum_bytes[0] = (uint8_t)(csum >> 24);
    csum_bytes[1] = (uint8_t)(csum >> 16);
    csum_bytes[2] = (uint8_t)(csum >> 8);
    csum_bytes[3] = (uint8_t)(csum);

    // base_w reads from the beginning, so pass pointer to where actual data starts
    base_w(csum_output, len2, csum_bytes + (4 - len_2_bytes));
}

/**
 * Software implementation of SPHINCS+ WOTS+ chain_lengths function
 * 
 * @param lengths Output array of nibbles (4-bit values)
 * @param len1 Number of message nibbles
 * @param len2 Number of checksum nibbles
 * @param msg Input message bytes
 */
void chain_lengths_sw(uint8_t *lengths, uint32_t len1, uint32_t len2, const uint8_t *msg) {
    // Extract nibbles from message
    base_w(lengths, len1, msg);
    
    // Compute and append checksum nibbles
    wots_checksum(lengths + len1, lengths, len1, len2);
}
