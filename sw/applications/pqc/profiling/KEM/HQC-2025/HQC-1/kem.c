/**
 * @file kem.c
 * @brief Implementation of api.h
 */

#include <stdint.h>
#include <string.h>
#include "api.h"
#include "crypto_memset.h"
#include "hqc.h"
#include "parameters.h"
#include "parsing.h"
#include "symmetric.h"
#include "vector.h"
#include "csr.h"

#include <stdio.h>


static void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
        if ((i + 1) % 16 == 0) printf("\n    "); // wrap every 16 bytes
    }
    printf("\n");
}

/**
 * @brief Generates a keypair for the KEM (Key Encapsulation Mechanism) scheme.
 *
 * This function generates a public/private keypair used for key encapsulation and decapsulation.
 * The encapsulation key (`ek`) is used to encapsulate a shared secret, while the decapsulation key (`dk`)
 * is used to recover it.
 *
 * @param[out] ek_kem Pointer to the output buffer where the encapsulation key will be stored.
 * @param[out] dk_kem Pointer to the output buffer where the decapsulation key will be stored.
 *
 * @return 0 on success.
 *
 * @pre The PRNG **must be seeded** with ::prng_init() before calling this function.
 * @warning This function calls ::prng_get_bytes() to sample `seed_kem`. If the PRNG has not been
 *          properly seeded beforehand, the generated keys will be insecure/predictable.
 * @note An example of correct seeding is provided in `main_hqc.c` (see `init_randomness()`), which
 *       seeds the PRNG using `syscall(SYS_getrandom, ...)` (32 bytes) by default..
 * @see prng_init, prng_get_bytes, main_hqc.c
 */
int crypto_kem_keypair(uint8_t *ek_kem, uint8_t *dk_kem) {
#ifdef VERBOSE
    printf("\n\n\n### KEYGEN ###");
#endif
    unsigned int cycles_function;
    uint8_t seed_kem[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t seed_pke[SEED_BYTES] = {0};
    shake256_xof_ctx ctx_kem;

    uint8_t ek_pke[PUBLIC_KEY_BYTES] = {0};
    uint8_t dk_pke[SEED_BYTES] = {0};

    // Sample seed_kem
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    prng_get_bytes(seed_kem, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair prng_get_bytes(seed_kem) cycles: %u\n", cycles_function);
    //print_hex("seed_kem", seed_kem, SEED_BYTES);

    // Compute seed_pke and randomness sigma
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    xof_init(&ctx_kem, seed_kem, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair xof_init cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    xof_get_bytes(&ctx_kem, seed_pke, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair xof_get_bytes(seed_pke) cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    xof_get_bytes(&ctx_kem, sigma, PARAM_SECURITY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair xof_get_bytes(sigma) cycles: %u\n", cycles_function);
    //print_hex("seed_pke", seed_pke, SEED_BYTES);
    //print_hex("sigma", sigma, PARAM_SECURITY_BYTES);

    // Compute HQC-PKE keypair
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hqc_pke_keygen(ek_pke, dk_pke, seed_pke);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair hqc_pke_keygen cycles: %u\n", cycles_function);
    //print_hex("ek_pke", ek_pke, PUBLIC_KEY_BYTES);
    //print_hex("dk_pke", dk_pke, SEED_BYTES);

    // Compute HQC-KEM keypair
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(ek_kem, ek_pke, PUBLIC_KEY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair memcpy ek_kem cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(dk_kem, ek_kem, PUBLIC_KEY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair memcpy dk_kem pub cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(dk_kem + PUBLIC_KEY_BYTES, dk_pke, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair memcpy dk_pke cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(dk_kem + PUBLIC_KEY_BYTES + SEED_BYTES, sigma, PARAM_SECURITY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair memcpy sigma cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(dk_kem + PUBLIC_KEY_BYTES + SEED_BYTES + PARAM_SECURITY_BYTES, seed_kem, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_keypair memcpy seed_kem cycles: %u\n", cycles_function);

#ifdef VERBOSE
    printf("\n\nseed_kem: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_kem[i]);
    printf("\n\nseed_pke: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_pke[i]);
    printf("\n\nsigma: ");
    for (int i = 0; i < PARAM_SECURITY_BYTES; ++i) printf("%02x", sigma[i]);
#endif

    // Zeroize sensitive data
    memset_zero(seed_kem, sizeof seed_kem);
    memset_zero(sigma, sizeof sigma);
    memset_zero(seed_pke, sizeof seed_pke);
    memset_zero(dk_pke, sizeof dk_pke);

    return 0;
}

/**
 * @brief Performs key encapsulation using the KEM scheme.
 *
 * This function uses the encapsulation key (`ek`) to generate a ciphertext (`c_kem`) and a shared secret (`K`)..
 *
 * @param[out] c_kem   Pointer to the output buffer where the KEM ciphertext will be stored.
 * @param[out] K       Pointer to the output buffer where the shared secret will be stored.
 * @param[in]  ek_kem      Pointer to the encapsulation key.
 *
 * @return Returns 0 on success.
 *
 * @pre The PRNG **must be seeded** with ::prng_init() before calling this function.
 * @warning This function calls ::prng_get_bytes() to sample `seed_kem`. If the PRNG has not been
 *          properly seeded beforehand, the generated keys will be insecure/predictable.
 * @note An example of correct seeding is provided in `main_hqc.c` (see `init_randomness()`), which
 *       seeds the PRNG using `syscall(SYS_getrandom, ...)` (32 bytes) by default..
 * @see prng_init, prng_get_bytes, main_hqc.c
 */
int crypto_kem_enc(uint8_t *c_kem, uint8_t *K, const uint8_t *ek_kem) {
#ifdef VERBOSE
    printf("\n\n\n\n### ENCAPS ###");
#endif

    unsigned int cycles_function;

    uint8_t m[PARAM_SECURITY_BYTES] = {0};
    uint8_t K_theta[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t theta[SEED_BYTES] = {0};
    uint8_t hash_ek_kem[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem_t = {0};

    // Sample message m and salt
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    prng_get_bytes(m, PARAM_SECURITY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc prng_get_bytes(m) cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    prng_get_bytes(c_kem_t.salt, SALT_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc prng_get_bytes(salt) cycles: %u\n", cycles_function);

    // Compute shared key K and ciphertext c_kem
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_h(hash_ek_kem, ek_kem);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc hash_h cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_g(K_theta, hash_ek_kem, m, c_kem_t.salt);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc hash_g cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(theta, K_theta + SEED_BYTES, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc memcpy theta cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hqc_pke_encrypt(&c_kem_t.c_pke, ek_kem, (uint64_t *)m, theta);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc hqc_pke_encrypt cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hqc_c_kem_to_string(c_kem, &c_kem_t);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc hqc_c_kem_to_string cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(K, K_theta, SHARED_SECRET_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_enc memcpy shared secret cycles: %u\n", cycles_function);

#ifdef VERBOSE
    printf("\n\nek_kem: ");
    for (int i = 0; i < PUBLIC_KEY_BYTES; ++i) printf("%02x", ek_kem[i]);
    printf("\n\nm: ");
    vect_print((uint64_t *)m, PARAM_SECURITY_BYTES);
    printf("\n\nsalt: ");
    for (int i = 0; i < SALT_BYTES; ++i) printf("%02x", c_kem_t.salt[i]);
    printf("\n\nH(ek_kem): ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", hash_ek_kem[i]);
    printf("\n\ntheta: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", theta[i]);
    printf("\n\nc_kem: ");
    for (int i = 0; i < CIPHERTEXT_BYTES; ++i) printf("%02x", c_kem[i]);
    printf("\n\nK: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", K[i]);
#endif

    // Zeroize sensitive data
    memset_zero(m, sizeof m);
    memset_zero(K_theta, sizeof K_theta);
    memset_zero(theta, sizeof theta);

    return 0;
}

/**
 * @brief Performs key decapsulation using the KEM scheme.
 *
 * This function uses the decapsulation key (`dk`) to recover the shared secret (`K_prime`)
 * from the given KEM ciphertext (`c_kem`), which was generated during encapsulation.
 *
 * @param[out] K_prime   Pointer to the output buffer where the recovered shared secret will be stored.
 * @param[in]  c_kem     Pointer to the input KEM ciphertext.
 * @param[in]  dk_kem    Pointer to the decapsulation key.
 *
 * @return Returns 0 on success.
 */
int crypto_kem_dec(uint8_t *K_prime, const uint8_t *c_kem, const uint8_t *dk_kem) {
#ifdef VERBOSE
    printf("\n\n\n\n### DECAPS ###");
#endif

    unsigned int cycles_function;

    uint8_t ek_pke[PUBLIC_KEY_BYTES] = {0};
    uint8_t dk_pke[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t m_prime[PARAM_SECURITY_BYTES] = {0};
    uint8_t hash_ek_kem[SEED_BYTES] = {0};
    uint8_t K_theta_prime[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t K_bar[SHARED_SECRET_BYTES] = {0};
    uint8_t theta_prime[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem_t = {0};
    ciphertext_kem_t c_kem_prime_t = {0};
    uint8_t result;

    // Parse decapsulation key dk_kem
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(ek_pke, dk_kem, PUBLIC_KEY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec memcpy ek_pke cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(dk_pke, dk_kem + PUBLIC_KEY_BYTES, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec memcpy dk_pke cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(sigma, dk_kem + PUBLIC_KEY_BYTES + SEED_BYTES, PARAM_SECURITY_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec memcpy sigma cycles: %u\n", cycles_function);

    // Parse ciphertext c_kem
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hqc_c_kem_from_string(&c_kem_t.c_pke, c_kem_t.salt, c_kem);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec hqc_c_kem_from_string cycles: %u\n", cycles_function);

    // Compute message m_prime
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    result = hqc_pke_decrypt((uint64_t *)m_prime, dk_pke, &c_kem_t.c_pke);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec hqc_pke_decrypt cycles: %u\n", cycles_function);

    // Compute shared key K_prime and ciphertext c_kem_prime
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_h(hash_ek_kem, ek_pke);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec hash_h cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_g(K_theta_prime, hash_ek_kem, m_prime, c_kem_t.salt);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec hash_g cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(K_prime, K_theta_prime, SHARED_SECRET_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec memcpy K_prime cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(theta_prime, K_theta_prime + SHARED_SECRET_BYTES, SEED_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec memcpy theta_prime cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hqc_pke_encrypt(&c_kem_prime_t.c_pke, ek_pke, (uint64_t *)m_prime, theta_prime);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec hqc_pke_encrypt(recompute) cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    memcpy(c_kem_prime_t.salt, c_kem_t.salt, SALT_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec memcpy salt cycles: %u\n", cycles_function);

    // Compute rejection key K_bar
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    hash_j(K_bar, hash_ek_kem, sigma, &c_kem_t);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec hash_j cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    result |= vect_compare((uint8_t *)c_kem_t.c_pke.u, (uint8_t *)c_kem_prime_t.c_pke.u, VEC_N_SIZE_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec vect_compare(u) cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    result |= vect_compare((uint8_t *)c_kem_t.c_pke.v, (uint8_t *)c_kem_prime_t.c_pke.v, VEC_N1N2_SIZE_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec vect_compare(v) cycles: %u\n", cycles_function);

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    result |= vect_compare(c_kem_t.salt, c_kem_prime_t.salt, SALT_BYTES);
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec vect_compare(salt) cycles: %u\n", cycles_function);

    result -= 1;

    CSR_WRITE(CSR_REG_MCYCLE, 0);
    for (size_t i = 0; i < SHARED_SECRET_BYTES; ++i) {
        K_prime[i] = (K_prime[i] & result) ^ (K_bar[i] & ~result);
    }
    CSR_READ(CSR_REG_MCYCLE, &cycles_function);
    printf("crypto_kem_dec conditional move loop cycles: %u\n", cycles_function);

#ifdef VERBOSE
    printf("\n\nek_pke: ");
    for (int i = 0; i < PUBLIC_KEY_BYTES; ++i) printf("%02x", ek_pke[i]);
    printf("\n\ndk_pke: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", dk_pke[i]);
    printf("\n\nc_kem: ");
    for (int i = 0; i < CIPHERTEXT_BYTES; ++i) printf("%02x", c_kem[i]);
    printf("\n\nm_prime: ");
    vect_print((uint64_t *)m_prime, PARAM_SECURITY_BYTES);
    printf("\n\nH(ek_kem): ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", hash_ek_kem[i]);
    printf("\n\ntheta_prime: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", theta_prime[i]);
    printf("\n\n\n# Checking Ciphertext - Begin #");
    printf("\n\nc_kem_prime_t.c_pke.u: ");
    vect_print(c_kem_prime_t.c_pke.u, VEC_N_SIZE_BYTES);
    printf("\n\nc_kem_prime_t.c_pke.v: ");
    vect_print(c_kem_prime_t.c_pke.v, VEC_N1N2_SIZE_BYTES);
    printf("\n\nsalt: ");
    for (int i = 0; i < SALT_BYTES; ++i) printf("%02x", c_kem_prime_t.salt[i]);
    printf("\n\n# Checking Ciphertext - End #\n");
    printf("\n\nK_prime: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", K_prime[i]);
#endif

    // Zeroize sensitive data
    memset_zero(dk_pke, sizeof dk_pke);
    memset_zero(sigma, sizeof sigma);
    memset_zero(m_prime, sizeof m_prime);
    memset_zero(K_theta_prime, sizeof K_theta_prime);
    memset_zero(K_bar, sizeof K_bar);
    memset_zero(theta_prime, sizeof theta_prime);

    return 0;
}
