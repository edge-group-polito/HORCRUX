/*
 * Software reference implementation of prf_addr
 * for SPHINCS+ shake256 variant (FIPS 205 compliant).
 * 
 * prf_addr(pub_seed, sk_seed, addr) = SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 *
 * This is a self-contained implementation with embedded SHAKE256.
 */

#include "prf_sw.h"
#include <string.h>

/* ========================================================================
 * Embedded SHAKE256 implementation (Keccak-f[1600])
 * ======================================================================== */

#define NROUNDS 24
#define ROL(a, offset) (((a) << (offset)) ^ ((a) >> (64 - (offset))))

/* Keccak round constants */
static const uint64_t KeccakF_RoundConstants[NROUNDS] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

static void KeccakF1600_StatePermute(uint64_t *state) {
    int round;
    uint64_t Aba, Abe, Abi, Abo, Abu;
    uint64_t Aga, Age, Agi, Ago, Agu;
    uint64_t Aka, Ake, Aki, Ako, Aku;
    uint64_t Ama, Ame, Ami, Amo, Amu;
    uint64_t Asa, Ase, Asi, Aso, Asu;
    uint64_t BCa, BCe, BCi, BCo, BCu;
    uint64_t Da, De, Di, Do, Du;
    uint64_t Eba, Ebe, Ebi, Ebo, Ebu;
    uint64_t Ega, Ege, Egi, Ego, Egu;
    uint64_t Eka, Eke, Eki, Eko, Eku;
    uint64_t Ema, Eme, Emi, Emo, Emu;
    uint64_t Esa, Ese, Esi, Eso, Esu;

    Aba = state[0];  Abe = state[1];  Abi = state[2];  Abo = state[3];  Abu = state[4];
    Aga = state[5];  Age = state[6];  Agi = state[7];  Ago = state[8];  Agu = state[9];
    Aka = state[10]; Ake = state[11]; Aki = state[12]; Ako = state[13]; Aku = state[14];
    Ama = state[15]; Ame = state[16]; Ami = state[17]; Amo = state[18]; Amu = state[19];
    Asa = state[20]; Ase = state[21]; Asi = state[22]; Aso = state[23]; Asu = state[24];

    for (round = 0; round < NROUNDS; round += 2) {
        BCa = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
        BCe = Abe ^ Age ^ Ake ^ Ame ^ Ase;
        BCi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
        BCo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
        BCu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

        Da = BCu ^ ROL(BCe, 1); De = BCa ^ ROL(BCi, 1); Di = BCe ^ ROL(BCo, 1);
        Do = BCi ^ ROL(BCu, 1); Du = BCo ^ ROL(BCa, 1);

        Aba ^= Da; BCa = Aba;           Age ^= De; BCe = ROL(Age, 44);
        Aki ^= Di; BCi = ROL(Aki, 43);  Amo ^= Do; BCo = ROL(Amo, 21);
        Asu ^= Du; BCu = ROL(Asu, 14);
        Eba = BCa ^ ((~BCe) & BCi); Eba ^= KeccakF_RoundConstants[round];
        Ebe = BCe ^ ((~BCi) & BCo); Ebi = BCi ^ ((~BCo) & BCu);
        Ebo = BCo ^ ((~BCu) & BCa); Ebu = BCu ^ ((~BCa) & BCe);

        Abo ^= Do; BCa = ROL(Abo, 28);  Agu ^= Du; BCe = ROL(Agu, 20);
        Aka ^= Da; BCi = ROL(Aka, 3);   Ame ^= De; BCo = ROL(Ame, 45);
        Asi ^= Di; BCu = ROL(Asi, 61);
        Ega = BCa ^ ((~BCe) & BCi); Ege = BCe ^ ((~BCi) & BCo);
        Egi = BCi ^ ((~BCo) & BCu); Ego = BCo ^ ((~BCu) & BCa); Egu = BCu ^ ((~BCa) & BCe);

        Abe ^= De; BCa = ROL(Abe, 1);   Agi ^= Di; BCe = ROL(Agi, 6);
        Ako ^= Do; BCi = ROL(Ako, 25);  Amu ^= Du; BCo = ROL(Amu, 8);
        Asa ^= Da; BCu = ROL(Asa, 18);
        Eka = BCa ^ ((~BCe) & BCi); Eke = BCe ^ ((~BCi) & BCo);
        Eki = BCi ^ ((~BCo) & BCu); Eko = BCo ^ ((~BCu) & BCa); Eku = BCu ^ ((~BCa) & BCe);

        Abu ^= Du; BCa = ROL(Abu, 27);  Aga ^= Da; BCe = ROL(Aga, 36);
        Ake ^= De; BCi = ROL(Ake, 10);  Ami ^= Di; BCo = ROL(Ami, 15);
        Aso ^= Do; BCu = ROL(Aso, 56);
        Ema = BCa ^ ((~BCe) & BCi); Eme = BCe ^ ((~BCi) & BCo);
        Emi = BCi ^ ((~BCo) & BCu); Emo = BCo ^ ((~BCu) & BCa); Emu = BCu ^ ((~BCa) & BCe);

        Abi ^= Di; BCa = ROL(Abi, 62);  Ago ^= Do; BCe = ROL(Ago, 55);
        Aku ^= Du; BCi = ROL(Aku, 39);  Ama ^= Da; BCo = ROL(Ama, 41);
        Ase ^= De; BCu = ROL(Ase, 2);
        Esa = BCa ^ ((~BCe) & BCi); Ese = BCe ^ ((~BCi) & BCo);
        Esi = BCi ^ ((~BCo) & BCu); Eso = BCo ^ ((~BCu) & BCa); Esu = BCu ^ ((~BCa) & BCe);

        /* Round 2 */
        BCa = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
        BCe = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
        BCi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
        BCo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
        BCu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

        Da = BCu ^ ROL(BCe, 1); De = BCa ^ ROL(BCi, 1); Di = BCe ^ ROL(BCo, 1);
        Do = BCi ^ ROL(BCu, 1); Du = BCo ^ ROL(BCa, 1);

        Eba ^= Da; BCa = Eba;           Ege ^= De; BCe = ROL(Ege, 44);
        Eki ^= Di; BCi = ROL(Eki, 43);  Emo ^= Do; BCo = ROL(Emo, 21);
        Esu ^= Du; BCu = ROL(Esu, 14);
        Aba = BCa ^ ((~BCe) & BCi); Aba ^= KeccakF_RoundConstants[round + 1];
        Abe = BCe ^ ((~BCi) & BCo); Abi = BCi ^ ((~BCo) & BCu);
        Abo = BCo ^ ((~BCu) & BCa); Abu = BCu ^ ((~BCa) & BCe);

        Ebo ^= Do; BCa = ROL(Ebo, 28);  Egu ^= Du; BCe = ROL(Egu, 20);
        Eka ^= Da; BCi = ROL(Eka, 3);   Eme ^= De; BCo = ROL(Eme, 45);
        Esi ^= Di; BCu = ROL(Esi, 61);
        Aga = BCa ^ ((~BCe) & BCi); Age = BCe ^ ((~BCi) & BCo);
        Agi = BCi ^ ((~BCo) & BCu); Ago = BCo ^ ((~BCu) & BCa); Agu = BCu ^ ((~BCa) & BCe);

        Ebe ^= De; BCa = ROL(Ebe, 1);   Egi ^= Di; BCe = ROL(Egi, 6);
        Eko ^= Do; BCi = ROL(Eko, 25);  Emu ^= Du; BCo = ROL(Emu, 8);
        Esa ^= Da; BCu = ROL(Esa, 18);
        Aka = BCa ^ ((~BCe) & BCi); Ake = BCe ^ ((~BCi) & BCo);
        Aki = BCi ^ ((~BCo) & BCu); Ako = BCo ^ ((~BCu) & BCa); Aku = BCu ^ ((~BCa) & BCe);

        Ebu ^= Du; BCa = ROL(Ebu, 27);  Ega ^= Da; BCe = ROL(Ega, 36);
        Eke ^= De; BCi = ROL(Eke, 10);  Emi ^= Di; BCo = ROL(Emi, 15);
        Eso ^= Do; BCu = ROL(Eso, 56);
        Ama = BCa ^ ((~BCe) & BCi); Ame = BCe ^ ((~BCi) & BCo);
        Ami = BCi ^ ((~BCo) & BCu); Amo = BCo ^ ((~BCu) & BCa); Amu = BCu ^ ((~BCa) & BCe);

        Ebi ^= Di; BCa = ROL(Ebi, 62);  Ego ^= Do; BCe = ROL(Ego, 55);
        Eku ^= Du; BCi = ROL(Eku, 39);  Ema ^= Da; BCo = ROL(Ema, 41);
        Ese ^= De; BCu = ROL(Ese, 2);
        Asa = BCa ^ ((~BCe) & BCi); Ase = BCe ^ ((~BCi) & BCo);
        Asi = BCi ^ ((~BCo) & BCu); Aso = BCo ^ ((~BCu) & BCa); Asu = BCu ^ ((~BCa) & BCe);
    }

    state[0] = Aba;  state[1] = Abe;  state[2] = Abi;  state[3] = Abo;  state[4] = Abu;
    state[5] = Aga;  state[6] = Age;  state[7] = Agi;  state[8] = Ago;  state[9] = Agu;
    state[10] = Aka; state[11] = Ake; state[12] = Aki; state[13] = Ako; state[14] = Aku;
    state[15] = Ama; state[16] = Ame; state[17] = Ami; state[18] = Amo; state[19] = Amu;
    state[20] = Asa; state[21] = Ase; state[22] = Asi; state[23] = Aso; state[24] = Asu;
}

/* SHAKE256: simplified one-shot for our use case */
static void shake256(uint8_t *output, size_t outlen,
                     const uint8_t *input, size_t inlen) {
    uint64_t s[25] = {0};
    uint8_t *p = (uint8_t *)s;
    size_t i;

    /* Absorb input */
    while (inlen >= SHAKE256_RATE) {
        for (i = 0; i < SHAKE256_RATE; i++)
            p[i] ^= input[i];
        KeccakF1600_StatePermute(s);
        input += SHAKE256_RATE;
        inlen -= SHAKE256_RATE;
    }

    /* Absorb remaining + padding */
    for (i = 0; i < inlen; i++)
        p[i] ^= input[i];
    p[inlen] ^= 0x1F;  /* SHAKE domain separator */
    p[SHAKE256_RATE - 1] ^= 0x80;  /* Final bit */

    KeccakF1600_StatePermute(s);

    /* Squeeze output */
    while (outlen > 0) {
        size_t block = (outlen < SHAKE256_RATE) ? outlen : SHAKE256_RATE;
        for (i = 0; i < block; i++)
            output[i] = p[i];
        output += block;
        outlen -= block;
        if (outlen > 0)
            KeccakF1600_StatePermute(s);
    }
}

/* ========================================================================
 * prf_addr implementation
 * ======================================================================== */

void spx_ctx_init(spx_ctx *ctx, const uint8_t *pub_seed, const uint8_t *sk_seed) {
    memcpy(ctx->pub_seed, pub_seed, SPX_N);
    memcpy(ctx->sk_seed, sk_seed, SPX_N);
}

/*
 * prf_addr_sw: Software reference implementation (FIPS 205)
 * 
 * Computes: SHAKE256(pub_seed || addr || sk_seed)[0:SPX_N]
 */
void prf_addr_sw(uint8_t *out, const spx_ctx *ctx, const uint8_t *addr) {
    uint8_t buf[2*SPX_N + SPX_ADDR_BYTES];
    
    /* Build input: pub_seed || addr || sk_seed */
    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);
    memcpy(buf + 2*SPX_N, ctx->sk_seed, SPX_N);
    
    /* Compute SHAKE256 */
    shake256(out, SPX_N, buf, 2*SPX_N + SPX_ADDR_BYTES);
}
