#ifndef TEST_PARAMS_H
#define TEST_PARAMS_H

/*
 * SPHINCS+ 128f-robust parameters for OP_THASH2 testing
 */

/* Hash output size in bytes */
#define SPX_N               16

/* Address size in bytes */
#define SPX_ADDR_BYTES      32

/* SHAKE256 rate in bytes */
#define SHAKE256_RATE       136

/* Address field offsets (SHAKE variant) */
#define SPX_OFFSET_LAYER        3
#define SPX_OFFSET_TREE         8
#define SPX_OFFSET_TYPE         19
#define SPX_OFFSET_KP_ADDR1     22
#define SPX_OFFSET_KP_ADDR2     20
#define SPX_OFFSET_CHAIN_ADDR   27
#define SPX_OFFSET_HASH_ADDR    31
#define SPX_OFFSET_TREE_HGT     27
#define SPX_OFFSET_TREE_INDEX   28

/* Address types */
#define SPX_ADDR_TYPE_WOTS      0
#define SPX_ADDR_TYPE_WOTSPK    1
#define SPX_ADDR_TYPE_HASHTREE  2
#define SPX_ADDR_TYPE_FORSTREE  3
#define SPX_ADDR_TYPE_FORSPK    4

/* THASH2: 2-block input (used for Merkle tree node hashing) */
#define THASH2_INBLOCKS         2
#define THASH2_INPUT_BYTES      (THASH2_INBLOCKS * SPX_N)   /* 32 bytes */

/* Register file layout for OP_THASH2:
 *   reg[0:3]   = pub_seed (16 bytes)
 *   reg[4:11]  = addr (32 bytes)
 *   reg[12:19] = input (32 bytes, 2 blocks)
 * Output:
 *   reg[0:3]   = output hash (16 bytes)
 */

#endif /* TEST_PARAMS_H */
