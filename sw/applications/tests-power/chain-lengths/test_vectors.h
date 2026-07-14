#ifndef TEST_VECTORS_H
#define TEST_VECTORS_H

#include <stdint.h>

// Test vector structure
typedef struct {
    const char* name;
    uint32_t len1;
    uint32_t len2;
    uint32_t len;
    uint32_t msg_bytes;
    const uint8_t* msg;
    const uint8_t* expected_output;
} test_vector_t;

// SPHINCS+-128f-simple
static const uint8_t msg_128f_simple[] __attribute__((aligned(4))) = {
    0x31, 0x26, 0x79, 0x08, 0xdb, 0xdb, 0xe6, 0x7c,
    0x59, 0x1b, 0x21, 0x79, 0x8d, 0xd9, 0x82, 0x2c
};
static const uint8_t expected_128f_simple[] = {
    3, 1, 2, 6, 7, 9, 0, 8, 13, 11, 13, 11, 14, 6, 7, 12,
    5, 9, 1, 11, 2, 1, 7, 9, 8, 13, 13, 9, 8, 2, 2, 12,
    0, 15, 5
};

static const test_vector_t test_vector_128f_simple = {
    "SPHINCS+-128f-simple", 32, 3, 35, 16, msg_128f_simple, expected_128f_simple
};

#endif // TEST_VECTORS_H
