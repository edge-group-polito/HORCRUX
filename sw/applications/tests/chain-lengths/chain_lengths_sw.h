#ifndef CHAIN_LENGTHS_SW_H
#define CHAIN_LENGTHS_SW_H

#include <stdint.h>

/**
 * Software implementation of SPHINCS+ WOTS+ chain_lengths function
 * 
 * @param lengths Output array of nibbles (4-bit values)
 * @param len1 Number of message nibbles
 * @param len2 Number of checksum nibbles
 * @param msg Input message bytes
 */
void chain_lengths_sw(uint8_t *lengths, uint32_t len1, uint32_t len2, const uint8_t *msg);

#endif // CHAIN_LENGTHS_SW_H
