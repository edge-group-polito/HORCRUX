/*
 * Wrapper for implementing the NIST API for the PQC standardization
 * process.
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "api.h"
#include "inner.h"
#include "csr.h"

#define NONCELEN   40

/*
 * If stack usage is an issue, define TEMPALLOC to static in order to
 * allocate temporaries in the data section instead of the stack. This
 * would make the crypto_sign_keypair(), crypto_sign(), and
 * crypto_sign_open() functions not reentrant and not thread-safe, so
 * this should be done only for testing purposes.
 */
#define TEMPALLOC

int crypto_sign_keypair(unsigned char *pk, unsigned char *sk, unsigned char *keypair_rnd)
{
	unsigned int cycles_function;
	TEMPALLOC union {
		uint8_t b[FALCON_KEYGEN_TEMP_9];
		uint64_t dummy_u64;
		fpr dummy_fpr;
	} tmp;
	TEMPALLOC int8_t f[512], g[512], F[512];
	TEMPALLOC uint16_t h[512];
	//TEMPALLOC unsigned char seed[48];
	TEMPALLOC inner_shake256_context rng;
	size_t u, v;


	/*
	 * Generate key pair.
	 */
	//randombytes(seed, sizeof seed);
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_init(&rng);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair inner_shake256_init cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_inject(&rng, keypair_rnd, 48);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair inner_shake256_inject cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_flip(&rng);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair inner_shake256_flip cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	Zf(keygen)(&rng, f, g, F, NULL, h, 9, tmp.b);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair Zf(keygen) cycles: %u\n", cycles_function);


	/*
	 * Encode private key.
	 */
	sk[0] = 0x50 + 9;
	u = 1;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(trim_i8_encode)(sk + u, CRYPTO_SECRETKEYBYTES - u,
		f, 9, Zf(max_fg_bits)[9]);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair trim_i8_encode(f) cycles: %u\n", cycles_function);
	if (v == 0) {
		return -1;
	}
	u += v;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(trim_i8_encode)(sk + u, CRYPTO_SECRETKEYBYTES - u,
		g, 9, Zf(max_fg_bits)[9]);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair trim_i8_encode(g) cycles: %u\n", cycles_function);
	if (v == 0) {
		return -1;
	}
	u += v;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(trim_i8_encode)(sk + u, CRYPTO_SECRETKEYBYTES - u,
		F, 9, Zf(max_FG_bits)[9]);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair trim_i8_encode(F) cycles: %u\n", cycles_function);
	if (v == 0) {
		return -1;
	}
	u += v;
	if (u != CRYPTO_SECRETKEYBYTES) {
		return -1;
	}

	/*
	 * Encode public key.
	 */
	pk[0] = 0x00 + 9;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(modq_encode)(pk + 1, CRYPTO_PUBLICKEYBYTES - 1, h, 9);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_keypair modq_encode cycles: %u\n", cycles_function);
	if (v != CRYPTO_PUBLICKEYBYTES - 1) {
		return -1;
	}

	return 0;
}

int crypto_sign(unsigned char *sm, unsigned long long *smlen,
	const unsigned char *m, unsigned long long mlen,
	const unsigned char *sk,
	const unsigned char *nonce,
	const unsigned char *sign_seed)
{
	unsigned int cycles_function;
	TEMPALLOC union {
		uint8_t b[72 * 512];
		uint64_t dummy_u64;
		fpr dummy_fpr;
	} tmp;
	TEMPALLOC int8_t f[512], g[512], F[512], G[512];
	TEMPALLOC union {
		int16_t sig[512];
		uint16_t hm[512];
	} r;
	TEMPALLOC unsigned char esig[CRYPTO_BYTES - 2 - sizeof nonce];
	TEMPALLOC inner_shake256_context sc;
	size_t u, v, sig_len;

	/*
	 * Decode the private key.
	 */
	if (sk[0] != 0x50 + 9) {
		return -1;
	}
	u = 1;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(trim_i8_decode)(f, 9, Zf(max_fg_bits)[9],
		sk + u, CRYPTO_SECRETKEYBYTES - u);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign trim_i8_decode(f) cycles: %u\n", cycles_function);
	if (v == 0) {
		return -1;
	}
	u += v;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(trim_i8_decode)(g, 9, Zf(max_fg_bits)[9],
		sk + u, CRYPTO_SECRETKEYBYTES - u);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign trim_i8_decode(g) cycles: %u\n", cycles_function);
	if (v == 0) {
		return -1;
	}
	u += v;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	v = Zf(trim_i8_decode)(F, 9, Zf(max_FG_bits)[9],
		sk + u, CRYPTO_SECRETKEYBYTES - u);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign trim_i8_decode(F) cycles: %u\n", cycles_function);
	if (v == 0) {
		return -1;
	}
	u += v;
	if (u != CRYPTO_SECRETKEYBYTES) {
		return -1;
	}
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	if (!Zf(complete_private)(G, f, g, F, 9, tmp.b)) {
		CSR_READ(CSR_REG_MCYCLE, &cycles_function);
		printf("crypto_sign complete_private(fail) cycles: %u\n", cycles_function);
		return -1;
	}
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign complete_private(pass) cycles: %u\n", cycles_function);

	/*
	 * Create a random nonce (40 bytes).
	 */
	//randombytes(nonce, sizeof nonce);

	/*
	 * Hash message nonce + message into a vector.
	 */
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_init(&sc);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign hash ctx init cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_inject(&sc, nonce, 40);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign inject nonce cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_inject(&sc, m, mlen);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign inject message cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_flip(&sc);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign hash ctx flip cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	Zf(hash_to_point_vartime)(&sc, r.hm, 9);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign hash_to_point_vartime cycles: %u\n", cycles_function);

	/*
	 * Initialize a RNG.
	 */
	//randombytes(seed, sizeof seed);
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_init(&sc);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign rng init cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_inject(&sc, sign_seed, 48);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign rng inject seed cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_flip(&sc);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign rng flip cycles: %u\n", cycles_function);


	/*
	 * Compute the signature.
	 */
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	Zf(sign_dyn)(r.sig, &sc, f, g, F, G, r.hm, 9, tmp.b);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign Zf(sign_dyn) cycles: %u\n", cycles_function);


	/*
	 * Encode the signature and bundle it with the message. Format is:
	 *   signature length     2 bytes, big-endian
	 *   nonce                40 bytes
	 *   message              mlen bytes
	 *   signature            slen bytes
	 */
	esig[0] = 0x20 + 9;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	sig_len = Zf(comp_encode)(esig + 1, (sizeof esig) - 1, r.sig, 9);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign comp_encode cycles: %u\n", cycles_function);
	if (sig_len == 0) {
		return -1;
	}
	sig_len ++;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	memmove(sm + 2 + NONCELEN, m, mlen);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign memmove message cycles: %u\n", cycles_function);
	sm[0] = (unsigned char)(sig_len >> 8);
	sm[1] = (unsigned char)sig_len;
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	memcpy(sm + 2, nonce, NONCELEN);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign memcpy nonce cycles: %u\n", cycles_function);
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	memcpy(sm + 2 + (NONCELEN) + mlen, esig, sig_len);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign memcpy esig cycles: %u\n", cycles_function);
	*smlen = 2 + (NONCELEN) + mlen + sig_len;
	return 0;
}

int
crypto_sign_open(unsigned char *m, unsigned long long *mlen,
	const unsigned char *sm, unsigned long long smlen,
	const unsigned char *pk)
{
	unsigned int cycles_function;
	TEMPALLOC union {
		uint8_t b[2 * 512];
		uint64_t dummy_u64;
		fpr dummy_fpr;
	} tmp;
	const unsigned char *esig;
	TEMPALLOC uint16_t h[512], hm[512];
	TEMPALLOC int16_t sig[512];
	TEMPALLOC inner_shake256_context sc;
	size_t sig_len, msg_len;

	/*
	 * Decode public key.
	 */
	if (pk[0] != 0x00 + 9) {
		return -1;
	}
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	if (Zf(modq_decode)(h, 9, pk + 1, CRYPTO_PUBLICKEYBYTES - 1)
		!= CRYPTO_PUBLICKEYBYTES - 1)
	{
		CSR_READ(CSR_REG_MCYCLE, &cycles_function);
		printf("crypto_sign_open modq_decode(fail) cycles: %u\n", cycles_function);
		return -1;
	}
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open modq_decode(pass) cycles: %u\n", cycles_function);

	CSR_WRITE(CSR_REG_MCYCLE, 0);
	Zf(to_ntt_monty)(h, 9);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open to_ntt_monty cycles: %u\n", cycles_function);

	/*
	 * Find nonce, signature, message length.
	 */
	if (smlen < 2 + NONCELEN) {
		return -1;
	}
	sig_len = ((size_t)sm[0] << 8) | (size_t)sm[1];
	if (sig_len > (smlen - 2 - NONCELEN)) {
		return -1;
	}
	msg_len = smlen - 2 - NONCELEN - sig_len;

	/*
	 * Decode signature.
	 */
	esig = sm + 2 + NONCELEN + msg_len;
	if (sig_len < 1 || esig[0] != 0x20 + 9) {
		return -1;
	}
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	if (Zf(comp_decode)(sig, 9,
		esig + 1, sig_len - 1) != sig_len - 1)
	{
		CSR_READ(CSR_REG_MCYCLE, &cycles_function);
		printf("crypto_sign_open comp_decode(fail) cycles: %u\n", cycles_function);
		return -1;
	}
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open comp_decode(pass) cycles: %u\n", cycles_function);

	/*
	 * Hash nonce + message into a vector.
	 */
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_init(&sc);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open hash init cycles: %u\n", cycles_function);
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_inject(&sc, sm + 2, NONCELEN + msg_len);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open hash inject cycles: %u\n", cycles_function);
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	inner_shake256_flip(&sc);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open hash flip cycles: %u\n", cycles_function);
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	Zf(hash_to_point_vartime)(&sc, hm, 9);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open hash_to_point_vartime cycles: %u\n", cycles_function);

	/*
	 * Verify signature.
	 */
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	if (!Zf(verify_raw)(hm, sig, h, 9, tmp.b)) {
		CSR_READ(CSR_REG_MCYCLE, &cycles_function);
		printf("crypto_sign_open verify_raw(fail) cycles: %u\n", cycles_function);
		return -1;
	}
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open verify_raw(pass) cycles: %u\n", cycles_function);

	/*
	 * Return plaintext.
	 */
	CSR_WRITE(CSR_REG_MCYCLE, 0);
	memmove(m, sm + 2 + NONCELEN, msg_len);
	CSR_READ(CSR_REG_MCYCLE, &cycles_function);
	printf("crypto_sign_open memmove plaintext cycles: %u\n", cycles_function);
	*mlen = msg_len;
	return 0;
}
