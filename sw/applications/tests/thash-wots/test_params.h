#ifndef TEST_PARAMS_H
#define TEST_PARAMS_H

/*
 * SPHINCS+ 128f parameters for standalone testing
 */

/* Hash output length in bytes */
#define SPX_N 16

/* Address size in bytes */
#define SPX_ADDR_BYTES 32

/* Winternitz parameter */
#define SPX_WOTS_W 16
#define SPX_WOTS_LOGW 4

/* WOTS chain lengths */
#define SPX_WOTS_LEN1 32  /* 8 * SPX_N / SPX_WOTS_LOGW */
#define SPX_WOTS_LEN2 3
#define SPX_WOTS_LEN (SPX_WOTS_LEN1 + SPX_WOTS_LEN2)  /* 35 */

/* Address field offsets (SHAKE variant) */
#define SPX_OFFSET_LAYER      3
#define SPX_OFFSET_TREE       8
#define SPX_OFFSET_TYPE       19
#define SPX_OFFSET_KP_ADDR    20
#define SPX_OFFSET_CHAIN_ADDR 27
#define SPX_OFFSET_HASH_ADDR  31
#define SPX_OFFSET_TREE_HGT   27
#define SPX_OFFSET_TREE_INDEX 28

/* Address types */
#define SPX_ADDR_TYPE_WOTS     0
#define SPX_ADDR_TYPE_WOTSPK   1
#define SPX_ADDR_TYPE_HASHTREE 2
#define SPX_ADDR_TYPE_FORSTREE 3
#define SPX_ADDR_TYPE_FORSPK   4
#define SPX_ADDR_TYPE_WOTSPRF  5
#define SPX_ADDR_TYPE_FORSPRF  6

#endif /* TEST_PARAMS_H */
