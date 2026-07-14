#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "api.h"
#include "params.h"
#include "wots.h"
#include "fors.h"
#include "hash.h"
#include "thash.h"
#include "address.h"
#include "randombytes.h"
#include "utils.h"
#include "merkle.h"
#include "csr.h"


#include <stdio.h>
#include <stdint.h>
#include <string.h>   // for memset/explicit_bzero if available

#ifndef explicit_bzero
#define explicit_bzero(b,len) memset((b), 0, (len))
#endif



/*
 * Returns the length of a secret key, in bytes
 */
unsigned long long crypto_sign_secretkeybytes(void)
{
    return CRYPTO_SECRETKEYBYTES;
}

/*
 * Returns the length of a public key, in bytes
 */
unsigned long long crypto_sign_publickeybytes(void)
{
    return CRYPTO_PUBLICKEYBYTES;
}

/*
 * Returns the length of a signature, in bytes
 */
unsigned long long crypto_sign_bytes(void)
{
    return CRYPTO_BYTES;
}

/*
 * Returns the length of the seed required to generate a key pair, in bytes
 */
unsigned long long crypto_sign_seedbytes(void)
{
    return CRYPTO_SEEDBYTES;
}

/*
 * Generates an SPX key pair given a seed of length
 * Format sk: [SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [PUB_SEED || root]
 */
int crypto_sign_seed_keypair(unsigned char *pk, unsigned char *sk,
                             const unsigned char *seed)
{
    unsigned int cycles_function;
    spx_ctx ctx;

    /* Initialize SK_SEED, SK_PRF and PUB_SEED from seed. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(sk, seed, CRYPTO_SEEDBYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_seed_keypair memcpy(sk seed) cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(pk, sk + 2*SPX_N, SPX_N);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_seed_keypair memcpy(pk pub_seed) cycles: %u\n", cycles_function);

    memcpy(ctx.pub_seed, pk, SPX_N);
    memcpy(ctx.sk_seed, sk, SPX_N);

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    initialize_hash_function(&ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_seed_keypair initialize_hash_function cycles: %u\n", cycles_function);

    /* Compute root node of the top-most subtree. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    merkle_gen_root(sk + 3*SPX_N, &ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_seed_keypair merkle_gen_root cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(pk + SPX_N, sk + 3*SPX_N, SPX_N);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_seed_keypair memcpy(pk root) cycles: %u\n", cycles_function);

    return 0;
}

/*
 * Generates an SPX key pair.
 * Format sk: [SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [PUB_SEED || root]
 */
int crypto_sign_keypair(unsigned char *pk, unsigned char *sk, uint8_t *keypair_rnd)
{
  unsigned int cycles_function;
  //unsigned char seed[CRYPTO_SEEDBYTES];
  //randombytes(seed, CRYPTO_SEEDBYTES);
  CSR_WRITE(CSR_REG_MCYCLE, 0);
  crypto_sign_seed_keypair(pk, sk, keypair_rnd);
  CSR_READ(CSR_REG_MCYCLE, &cycles_function);
  printf("crypto_sign_keypair crypto_sign_seed_keypair cycles: %u\n", cycles_function);

  return 0;
}

/**
 * Returns an array containing a detached signature.
 */
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk, uint8_t* signature_rnd)
{
    unsigned int cycles_function;
    spx_ctx ctx;
    const unsigned char *sk_prf = sk + SPX_N;
    const unsigned char *pk = sk + 2*SPX_N;

    unsigned char optrand[SPX_N];
    unsigned char mhash[SPX_FORS_MSG_BYTES];
    unsigned char root[SPX_N];
    uint32_t i;
    uint64_t tree;
    uint32_t idx_leaf;
    uint32_t wots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};

    memcpy(ctx.sk_seed, sk, SPX_N);
    memcpy(ctx.pub_seed, pk, SPX_N);

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    initialize_hash_function(&ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature initialize_hash_function cycles: %u\n", cycles_function);

    set_type(wots_addr, SPX_ADDR_TYPE_WOTS);
    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);

    /* Optionally, signing can be made non-deterministic using optrand.
       This can help counter side-channel attacks that would benefit from
       getting a large number of traces when the signer uses the same nodes. */
    //randombytes(optrand, SPX_N);


    /* Compute the digest randomization value. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gen_message_random(sig, sk_prf, signature_rnd, m, mlen, &ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature gen_message_random cycles: %u\n", cycles_function);

    /* Derive the message digest and leaf index from R, PK and M. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_message(mhash, &tree, &idx_leaf, sig, pk, m, mlen, &ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature hash_message cycles: %u\n", cycles_function);
    sig += SPX_N;

    set_tree_addr(wots_addr, tree);
    set_keypair_addr(wots_addr, idx_leaf);

    /* Sign the message hash using FORS. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    fors_sign(sig, root, mhash, &ctx, wots_addr);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_signature fors_sign cycles: %u\n", cycles_function);
    sig += SPX_FORS_BYTES;

    for (i = 0; i < SPX_D; i++) {
        set_layer_addr(tree_addr, i);
        set_tree_addr(tree_addr, tree);

        copy_subtree_addr(wots_addr, tree_addr);
        set_keypair_addr(wots_addr, idx_leaf);

        CSR_WRITE(CSR_REG_MCYCLE, 0);
        merkle_sign(sig, root, &ctx, wots_addr, tree_addr, idx_leaf);
        CSR_READ(CSR_REG_MCYCLE, &cycles_function);
        printf("crypto_sign_signature merkle_sign[%u] cycles: %u\n", i, cycles_function);
        sig += SPX_WOTS_BYTES + SPX_TREE_HEIGHT * SPX_N;

        /* Update the indices for the next layer. */
        idx_leaf = (tree & ((1 << SPX_TREE_HEIGHT)-1));
        tree = tree >> SPX_TREE_HEIGHT;
    }

    *siglen = SPX_BYTES;

    return 0;
}

/**
 * Verifies a detached signature and message under a given public key.
 */
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk)
{
    unsigned int cycles_function;
    spx_ctx ctx;
    const unsigned char *pub_root = pk + SPX_N;
    unsigned char mhash[SPX_FORS_MSG_BYTES];
    unsigned char wots_pk[SPX_WOTS_BYTES];
    unsigned char root[SPX_N];
    unsigned char leaf[SPX_N];
    unsigned int i;
    uint64_t tree;
    uint32_t idx_leaf;
    uint32_t wots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};
    uint32_t wots_pk_addr[8] = {0};

    if (siglen != SPX_BYTES) {
        return -1;
    }

    memcpy(ctx.pub_seed, pk, SPX_N);

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    initialize_hash_function(&ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_verify initialize_hash_function cycles: %u\n", cycles_function);

    set_type(wots_addr, SPX_ADDR_TYPE_WOTS);
    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);
    set_type(wots_pk_addr, SPX_ADDR_TYPE_WOTSPK);

    /* Derive the message digest and leaf index from R || PK || M. */
    /* The additional SPX_N is a result of the hash domain separator. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_message(mhash, &tree, &idx_leaf, sig, pk, m, mlen, &ctx);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_verify hash_message cycles: %u\n", cycles_function);
    sig += SPX_N;

    /* Layer correctly defaults to 0, so no need to set_layer_addr */
    set_tree_addr(wots_addr, tree);
    set_keypair_addr(wots_addr, idx_leaf);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    fors_pk_from_sig(root, sig, mhash, &ctx, wots_addr);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_verify fors_pk_from_sig cycles: %u\n", cycles_function);
    sig += SPX_FORS_BYTES;

    /* For each subtree.. */
    for (i = 0; i < SPX_D; i++) {
        set_layer_addr(tree_addr, i);
        set_tree_addr(tree_addr, tree);

        copy_subtree_addr(wots_addr, tree_addr);
        set_keypair_addr(wots_addr, idx_leaf);

        copy_keypair_addr(wots_pk_addr, wots_addr);

        /* The WOTS public key is only correct if the signature was correct. */
        /* Initially, root is the FORS pk, but on subsequent iterations it is
           the root of the subtree below the currently processed subtree. */
        CSR_WRITE(CSR_REG_MCYCLE, 0);
        wots_pk_from_sig(wots_pk, sig, root, &ctx, wots_addr);
        CSR_READ(CSR_REG_MCYCLE, &cycles_function);
        printf("crypto_sign_verify wots_pk_from_sig[%u] cycles: %u\n", i, cycles_function);
        sig += SPX_WOTS_BYTES;

        /* Compute the leaf node using the WOTS public key. */
        CSR_WRITE(CSR_REG_MCYCLE, 0);
        thash(leaf, wots_pk, SPX_WOTS_LEN, &ctx, wots_pk_addr);
        CSR_READ(CSR_REG_MCYCLE, &cycles_function);
        printf("crypto_sign_verify thash[%u] cycles: %u\n", i, cycles_function);

        /* Compute the root node of this subtree. */
        CSR_WRITE(CSR_REG_MCYCLE, 0);
        compute_root(root, leaf, idx_leaf, 0, sig, SPX_TREE_HEIGHT,
                     &ctx, tree_addr);
        CSR_READ(CSR_REG_MCYCLE, &cycles_function);
        printf("crypto_sign_verify compute_root[%u] cycles: %u\n", i, cycles_function);
        sig += SPX_TREE_HEIGHT * SPX_N;

        /* Update the indices for the next layer. */
        idx_leaf = (tree & ((1 << SPX_TREE_HEIGHT)-1));
        tree = tree >> SPX_TREE_HEIGHT;
    }

    /* Check if the root node equals the root node in the public key. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    if (memcmp(root, pub_root, SPX_N)) {
        CSR_READ(CSR_REG_MCYCLE, &cycles_function);
        printf("crypto_sign_verify memcmp(root,fail) cycles: %u\n", cycles_function);
        return -1;
    }
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_verify memcmp(root,pass) cycles: %u\n", cycles_function);

    return 0;
}


/**
 * Returns an array containing the signature followed by the message.
 */
int crypto_sign(unsigned char *sm, unsigned long long *smlen,
                const unsigned char *m, unsigned long long mlen,
                const unsigned char *sk, uint8_t *signature_rnd)
{
    unsigned int cycles_function;
    size_t siglen;

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    crypto_sign_signature(sm, &siglen, m, (size_t)mlen, sk, signature_rnd);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign crypto_sign_signature cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memmove(sm + SPX_BYTES, m, mlen);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign memmove message cycles: %u\n", cycles_function);
    *smlen = siglen + mlen;

    return 0;
}

/**
 * Verifies a given signature-message pair under a given public key.
 */
int crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk)
{
    unsigned int cycles_function;
    /* The API caller does not necessarily know what size a signature should be
       but SPHINCS+ signatures are always exactly SPX_BYTES. */
    if (smlen < SPX_BYTES) {
        memset(m, 0, smlen);
        *mlen = 0;
        return -1;
    }

    *mlen = smlen - SPX_BYTES;

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    if (crypto_sign_verify(sm, SPX_BYTES, sm + SPX_BYTES, (size_t)*mlen, pk)) {
        CSR_READ(CSR_REG_MCYCLE, &cycles_function);
        printf("crypto_sign_open crypto_sign_verify(fail) cycles: %u\n", cycles_function);
        memset(m, 0, smlen);
        *mlen = 0;
        return -1;
    }
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_open crypto_sign_verify(pass) cycles: %u\n", cycles_function);

    /* If verification was successful, move the message to the right place. */
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memmove(m, sm + SPX_BYTES, *mlen);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_sign_open memmove message cycles: %u\n", cycles_function);

    return 0;
}
