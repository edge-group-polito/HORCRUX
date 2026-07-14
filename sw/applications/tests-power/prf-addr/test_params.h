#ifndef TEST_PARAMS_H
#define TEST_PARAMS_H

/*
 * SPHINCS+ 128f-robust parameters for OP_PRF_ADDR testing
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

/* WOTS chain lengths */
#define SPX_WOTS_LEN1 32  /* 8 * SPX_N / SPX_WOTS_LOGW */
#define SPX_WOTS_LEN2 3
#define SPX_WOTS_LEN (SPX_WOTS_LEN1 + SPX_WOTS_LEN2)  /* 35 */

/* PRF_ADDR: pseudo-random function for secret key derivation
 * 
 * prf_addr(sk_seed, addr) = SHAKE256(sk_seed || addr)[0:SPX_N]
 *
 * Register file layout for OP_PRF_ADDR:
 *   reg[0:3]   = sk_seed (16 bytes)
 *   reg[4:11]  = addr (32 bytes)
 * Output:
 *   reg[0:3]   = PRF output (16 bytes)
 */

#endif /* TEST_PARAMS_H */
