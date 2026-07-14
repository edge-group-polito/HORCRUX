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

// SPHINCS+-128f-robust
static const uint8_t msg_128f_robust[] __attribute__((aligned(4))) = {
    0x64, 0xe6, 0xb1, 0xb7, 0x34, 0x24, 0xba, 0x5f,
    0xd5, 0x3e, 0x7a, 0x95, 0x60, 0x31, 0x1e, 0xe9
};
static const uint8_t expected_128f_robust[] = {
    6, 4, 14, 6, 11, 1, 11, 7, 3, 4, 2, 4, 11, 10, 5, 15,
    13, 5, 3, 14, 7, 10, 9, 5, 6, 0, 3, 1, 1, 14, 14, 9,
    0, 15, 12
};

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

// SPHINCS+-192f-simple
static const uint8_t msg_192f_simple[] __attribute__((aligned(4))) = {
    0x50, 0x02, 0x75, 0xb7, 0xd9, 0xda, 0x9c, 0xba,
    0xf7, 0x71, 0xa8, 0x60, 0xc2, 0x47, 0xf9, 0xb3,
    0x42, 0xfc, 0xf7, 0xea, 0x45, 0xe9, 0xca, 0x1d
};
static const uint8_t expected_192f_simple[] = {
    5, 0, 0, 2, 7, 5, 11, 7, 13, 9, 13, 10, 9, 12, 11, 10,
    15, 7, 7, 1, 10, 8, 6, 0, 12, 2, 4, 7, 15, 9, 11, 3,
    4, 2, 15, 12, 15, 7, 14, 10, 4, 5, 14, 9, 12, 10, 1, 13,
    1, 4, 12
};

// SPHINCS+-192f-robust
static const uint8_t msg_192f_robust[] __attribute__((aligned(4))) = {
    0xfa, 0x37, 0x64, 0xb4, 0x12, 0x47, 0xc8, 0x62,
    0x38, 0x64, 0xc0, 0x22, 0x82, 0x82, 0xf3, 0xf4,
    0xe5, 0x21, 0x48, 0x2c, 0x89, 0xf7, 0xe7, 0x7c
};
static const uint8_t expected_192f_robust[] = {
    15, 10, 3, 7, 6, 4, 11, 4, 1, 2, 4, 7, 12, 8, 6, 2,
    3, 8, 6, 4, 12, 0, 2, 2, 8, 2, 8, 2, 15, 3, 15, 4,
    14, 5, 2, 1, 4, 8, 2, 12, 8, 9, 15, 7, 14, 7, 7, 12,
    1, 8, 13
};

// SPHINCS+-256f-robust
static const uint8_t msg_256f_robust[] __attribute__((aligned(4))) = {
    0x7e, 0x45, 0xff, 0xd7, 0x22, 0x22, 0x1a, 0xcb,
    0xfb, 0xcf, 0x77, 0x88, 0x49, 0x21, 0x6d, 0xdc,
    0x01, 0xf7, 0x94, 0x59, 0x36, 0x48, 0xd6, 0x19,
    0x4f, 0xd6, 0xcf, 0x5f, 0x2a, 0xab, 0x8d, 0xa1
};
static const uint8_t expected_256f_robust[] = {
    7, 14, 4, 5, 15, 15, 13, 7, 2, 2, 2, 2, 1, 10, 12, 11,
    15, 11, 12, 15, 7, 7, 8, 8, 4, 9, 2, 1, 6, 13, 13, 12,
    0, 1, 15, 7, 9, 4, 5, 9, 3, 6, 4, 8, 13, 6, 1, 9,
    4, 15, 13, 6, 12, 15, 5, 15, 2, 10, 10, 11, 8, 13, 10, 1,
    1, 11, 13
};

// SPHINCS+-256f-simple  
static const uint8_t msg_256f_simple[] __attribute__((aligned(4))) = {
    0x32, 0x0e, 0xc5, 0x6f, 0xfe, 0xd6, 0xf0, 0xf9,
    0xf5, 0x51, 0xfa, 0xa1, 0x4b, 0x4b, 0xff, 0xa4,
    0xcd, 0x1f, 0xf9, 0x7b, 0xbc, 0xb5, 0xa1, 0x63,
    0x09, 0xc2, 0xfb, 0xad, 0x0e, 0x44, 0xfb, 0xff
};
static const uint8_t expected_256f_simple[] = {
    3, 2, 0, 14, 12, 5, 6, 15, 15, 14, 13, 6, 15, 0, 15, 9,
    15, 5, 5, 1, 15, 10, 10, 1, 4, 11, 4, 11, 15, 15, 10, 4,
    12, 13, 1, 15, 15, 9, 7, 11, 11, 12, 11, 5, 10, 1, 6, 3,
    0, 9, 12, 2, 15, 11, 10, 13, 0, 14, 4, 4, 15, 11, 15, 15,
    1, 8, 4
};

// Test vector array
static const test_vector_t test_vectors[] = {
    {"SPHINCS+-128f-robust", 32, 3, 35, 16, msg_128f_robust, expected_128f_robust},
    {"SPHINCS+-128f-simple", 32, 3, 35, 16, msg_128f_simple, expected_128f_simple},
    {"SPHINCS+-192f-robust", 48, 3, 51, 24, msg_192f_robust, expected_192f_robust},
    {"SPHINCS+-192f-simple", 48, 3, 51, 24, msg_192f_simple, expected_192f_simple},
    {"SPHINCS+-256f-robust", 64, 3, 67, 32, msg_256f_robust, expected_256f_robust},
    {"SPHINCS+-256f-simple", 64, 3, 67, 32, msg_256f_simple, expected_256f_simple},
};

#define NUM_TEST_VECTORS (sizeof(test_vectors) / sizeof(test_vector_t))

#endif // TEST_VECTORS_H
