#ifndef SPHINCS_REF_IMPL_H
#define SPHINCS_REF_IMPL_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SPX_ADDR_BYTES 32
#define SPX_WOTS_W 16
#define SPX_WOTS_LOGW 4

/* Low-level coprocessor hooks provided by keccak_coproc.S in each test app. */
extern void cus_load(uint32_t lo, uint32_t hi, uint32_t index);
extern uint32_t cus_store(uint32_t index);

/* =============================== Utilities =============================== */

static inline uint32_t spx_load32(const uint8_t *x) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) |
           ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

static inline void spx_store32(uint8_t *x, uint32_t v) {
    x[0] = (uint8_t)(v);
    x[1] = (uint8_t)(v >> 8);
    x[2] = (uint8_t)(v >> 16);
    x[3] = (uint8_t)(v >> 24);
}

static inline int spx_compare_arrays(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 1;
        }
    }
    return 0;
}

static inline void spx_fill_bytes(uint8_t *out, size_t len, uint32_t seed) {
    uint32_t x = seed ^ 0x9E3779B9u;
    for (size_t i = 0; i < len; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        out[i] = (uint8_t)(x & 0xFFu);
    }
}

/* ========================== SHAKE256 Reference ========================== */

#define SPX_SHAKE256_RATE 136
#define SPX_NROUNDS 24
#define SPX_ROL(a, offset) (((a) << (offset)) ^ ((a) >> (64 - (offset))))

static const uint64_t spx_KeccakF_RoundConstants[SPX_NROUNDS] = {
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

static inline void spx_KeccakF1600_StatePermute(uint64_t *state) {
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

    for (round = 0; round < SPX_NROUNDS; round += 2) {
        BCa = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
        BCe = Abe ^ Age ^ Ake ^ Ame ^ Ase;
        BCi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
        BCo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
        BCu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

        Da = BCu ^ SPX_ROL(BCe, 1); De = BCa ^ SPX_ROL(BCi, 1); Di = BCe ^ SPX_ROL(BCo, 1);
        Do = BCi ^ SPX_ROL(BCu, 1); Du = BCo ^ SPX_ROL(BCa, 1);

        Aba ^= Da; BCa = Aba;           Age ^= De; BCe = SPX_ROL(Age, 44);
        Aki ^= Di; BCi = SPX_ROL(Aki, 43);  Amo ^= Do; BCo = SPX_ROL(Amo, 21);
        Asu ^= Du; BCu = SPX_ROL(Asu, 14);
        Eba = BCa ^ ((~BCe) & BCi); Eba ^= spx_KeccakF_RoundConstants[round];
        Ebe = BCe ^ ((~BCi) & BCo); Ebi = BCi ^ ((~BCo) & BCu);
        Ebo = BCo ^ ((~BCu) & BCa); Ebu = BCu ^ ((~BCa) & BCe);

        Abo ^= Do; BCa = SPX_ROL(Abo, 28);  Agu ^= Du; BCe = SPX_ROL(Agu, 20);
        Aka ^= Da; BCi = SPX_ROL(Aka, 3);   Ame ^= De; BCo = SPX_ROL(Ame, 45);
        Asi ^= Di; BCu = SPX_ROL(Asi, 61);
        Ega = BCa ^ ((~BCe) & BCi); Ege = BCe ^ ((~BCi) & BCo);
        Egi = BCi ^ ((~BCo) & BCu); Ego = BCo ^ ((~BCu) & BCa); Egu = BCu ^ ((~BCa) & BCe);

        Abe ^= De; BCa = SPX_ROL(Abe, 1);   Agi ^= Di; BCe = SPX_ROL(Agi, 6);
        Ako ^= Do; BCi = SPX_ROL(Ako, 25);  Amu ^= Du; BCo = SPX_ROL(Amu, 8);
        Asa ^= Da; BCu = SPX_ROL(Asa, 18);
        Eka = BCa ^ ((~BCe) & BCi); Eke = BCe ^ ((~BCi) & BCo);
        Eki = BCi ^ ((~BCo) & BCu); Eko = BCo ^ ((~BCu) & BCa); Eku = BCu ^ ((~BCa) & BCe);

        Abu ^= Du; BCa = SPX_ROL(Abu, 27);  Aga ^= Da; BCe = SPX_ROL(Aga, 36);
        Ake ^= De; BCi = SPX_ROL(Ake, 10);  Ami ^= Di; BCo = SPX_ROL(Ami, 15);
        Aso ^= Do; BCu = SPX_ROL(Aso, 56);
        Ema = BCa ^ ((~BCe) & BCi); Eme = BCe ^ ((~BCi) & BCo);
        Emi = BCi ^ ((~BCo) & BCu); Emo = BCo ^ ((~BCu) & BCa); Emu = BCu ^ ((~BCa) & BCe);

        Abi ^= Di; BCa = SPX_ROL(Abi, 62);  Ago ^= Do; BCe = SPX_ROL(Ago, 55);
        Aku ^= Du; BCi = SPX_ROL(Aku, 39);  Ama ^= Da; BCo = SPX_ROL(Ama, 41);
        Ase ^= De; BCu = SPX_ROL(Ase, 2);
        Esa = BCa ^ ((~BCe) & BCi); Ese = BCe ^ ((~BCi) & BCo);
        Esi = BCi ^ ((~BCo) & BCu); Eso = BCo ^ ((~BCu) & BCa); Esu = BCu ^ ((~BCa) & BCe);

        BCa = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
        BCe = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
        BCi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
        BCo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
        BCu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

        Da = BCu ^ SPX_ROL(BCe, 1); De = BCa ^ SPX_ROL(BCi, 1); Di = BCe ^ SPX_ROL(BCo, 1);
        Do = BCi ^ SPX_ROL(BCu, 1); Du = BCo ^ SPX_ROL(BCa, 1);

        Eba ^= Da; BCa = Eba;           Ege ^= De; BCe = SPX_ROL(Ege, 44);
        Eki ^= Di; BCi = SPX_ROL(Eki, 43);  Emo ^= Do; BCo = SPX_ROL(Emo, 21);
        Esu ^= Du; BCu = SPX_ROL(Esu, 14);
        Aba = BCa ^ ((~BCe) & BCi); Aba ^= spx_KeccakF_RoundConstants[round + 1];
        Abe = BCe ^ ((~BCi) & BCo); Abi = BCi ^ ((~BCo) & BCu);
        Abo = BCo ^ ((~BCu) & BCa); Abu = BCu ^ ((~BCa) & BCe);

        Ebo ^= Do; BCa = SPX_ROL(Ebo, 28);  Egu ^= Du; BCe = SPX_ROL(Egu, 20);
        Eka ^= Da; BCi = SPX_ROL(Eka, 3);   Eme ^= De; BCo = SPX_ROL(Eme, 45);
        Esi ^= Di; BCu = SPX_ROL(Esi, 61);
        Aga = BCa ^ ((~BCe) & BCi); Age = BCe ^ ((~BCi) & BCo);
        Agi = BCi ^ ((~BCo) & BCu); Ago = BCo ^ ((~BCu) & BCa); Agu = BCu ^ ((~BCa) & BCe);

        Ebe ^= De; BCa = SPX_ROL(Ebe, 1);   Egi ^= Di; BCe = SPX_ROL(Egi, 6);
        Eko ^= Do; BCi = SPX_ROL(Eko, 25);  Emu ^= Du; BCo = SPX_ROL(Emu, 8);
        Esa ^= Da; BCu = SPX_ROL(Esa, 18);
        Aka = BCa ^ ((~BCe) & BCi); Ake = BCe ^ ((~BCi) & BCo);
        Aki = BCi ^ ((~BCo) & BCu); Ako = BCo ^ ((~BCu) & BCa); Aku = BCu ^ ((~BCa) & BCe);

        Ebu ^= Du; BCa = SPX_ROL(Ebu, 27);  Ega ^= Da; BCe = SPX_ROL(Ega, 36);
        Eke ^= De; BCi = SPX_ROL(Eke, 10);  Emi ^= Di; BCo = SPX_ROL(Emi, 15);
        Eso ^= Do; BCu = SPX_ROL(Eso, 56);
        Ama = BCa ^ ((~BCe) & BCi); Ame = BCe ^ ((~BCi) & BCo);
        Ami = BCi ^ ((~BCo) & BCu); Amo = BCo ^ ((~BCu) & BCa); Amu = BCu ^ ((~BCa) & BCe);

        Ebi ^= Di; BCa = SPX_ROL(Ebi, 62);  Ego ^= Do; BCe = SPX_ROL(Ego, 55);
        Eku ^= Du; BCi = SPX_ROL(Eku, 39);  Ema ^= Da; BCo = SPX_ROL(Ema, 41);
        Ese ^= De; BCu = SPX_ROL(Ese, 2);
        Asa = BCa ^ ((~BCe) & BCi); Ase = BCe ^ ((~BCi) & BCo);
        Asi = BCi ^ ((~BCo) & BCu); Aso = BCo ^ ((~BCu) & BCa); Asu = BCu ^ ((~BCa) & BCe);
    }

    state[0] = Aba;  state[1] = Abe;  state[2] = Abi;  state[3] = Abo;  state[4] = Abu;
    state[5] = Aga;  state[6] = Age;  state[7] = Agi;  state[8] = Ago;  state[9] = Agu;
    state[10] = Aka; state[11] = Ake; state[12] = Aki; state[13] = Ako; state[14] = Aku;
    state[15] = Ama; state[16] = Ame; state[17] = Ami; state[18] = Amo; state[19] = Amu;
    state[20] = Asa; state[21] = Ase; state[22] = Asi; state[23] = Aso; state[24] = Asu;
}

static inline void spx_shake256(uint8_t *output, size_t outlen,
                                const uint8_t *input, size_t inlen) {
    uint64_t s[25] = {0};
    uint8_t *p = (uint8_t *)s;
    size_t i;

    while (inlen >= SPX_SHAKE256_RATE) {
        for (i = 0; i < SPX_SHAKE256_RATE; i++) {
            p[i] ^= input[i];
        }
        spx_KeccakF1600_StatePermute(s);
        input += SPX_SHAKE256_RATE;
        inlen -= SPX_SHAKE256_RATE;
    }

    for (i = 0; i < inlen; i++) {
        p[i] ^= input[i];
    }
    p[inlen] ^= 0x1F;
    p[SPX_SHAKE256_RATE - 1] ^= 0x80;

    spx_KeccakF1600_StatePermute(s);

    while (outlen > 0) {
        size_t block = (outlen < SPX_SHAKE256_RATE) ? outlen : SPX_SHAKE256_RATE;
        for (i = 0; i < block; i++) {
            output[i] = p[i];
        }
        output += block;
        outlen -= block;
        if (outlen > 0) {
            spx_KeccakF1600_StatePermute(s);
        }
    }
}

/* ============================ SPHINCS Reference ============================ */

static inline void spx_ref_prf(uint8_t *out,
                               const uint8_t *pub_seed,
                               const uint8_t *sk_seed,
                               const uint8_t *addr,
                               size_t n) {
    uint8_t buf[2 * 32 + SPX_ADDR_BYTES];
    memcpy(buf, pub_seed, n);
    memcpy(buf + n, addr, SPX_ADDR_BYTES);
    memcpy(buf + n + SPX_ADDR_BYTES, sk_seed, n);
    spx_shake256(out, n, buf, 2 * n + SPX_ADDR_BYTES);
}

static inline void spx_ref_thash(uint8_t *out,
                                 const uint8_t *in,
                                 unsigned int inblocks,
                                 const uint8_t *pub_seed,
                                 const uint8_t *addr,
                                 size_t n,
                                 int simple_mode) {
    size_t inlen = (size_t)inblocks * n;
    uint8_t prefix[32 + SPX_ADDR_BYTES];
    uint8_t buf[32 + SPX_ADDR_BYTES + 2 * 32];
    uint8_t bitmask[2 * 32];

    memcpy(prefix, pub_seed, n);
    memcpy(prefix + n, addr, SPX_ADDR_BYTES);

    if (simple_mode) {
        memcpy(buf, prefix, n + SPX_ADDR_BYTES);
        memcpy(buf + n + SPX_ADDR_BYTES, in, inlen);
        spx_shake256(out, n, buf, n + SPX_ADDR_BYTES + inlen);
        return;
    }

    spx_shake256(bitmask, inlen, prefix, n + SPX_ADDR_BYTES);
    memcpy(buf, prefix, n + SPX_ADDR_BYTES);
    for (size_t i = 0; i < inlen; i++) {
        buf[n + SPX_ADDR_BYTES + i] = in[i] ^ bitmask[i];
    }
    spx_shake256(out, n, buf, n + SPX_ADDR_BYTES + inlen);
}

/* ============================== HW Invocation ============================== */

static inline void spx_hw_wait(void) {
    for (int i = 0; i < 80; i++) {
        __asm__ volatile("nop");
    }
}

static inline void spx_hw_trigger(uint8_t funct7, int simple_mode) {
    switch (funct7) {
        case 0x30:
            if (simple_mode) {
                __asm__ volatile("li t0, 1\n\t.insn r 0x3b, 0x7, 0x30, x0, x0, t0" ::: "t0", "memory");
            } else {
                __asm__ volatile(".insn r 0x3b, 0x7, 0x30, x0, x0, x0" ::: "memory");
            }
            break;
        case 0x31:
            if (simple_mode) {
                __asm__ volatile("li t0, 1\n\t.insn r 0x3b, 0x7, 0x31, x0, x0, t0" ::: "t0", "memory");
            } else {
                __asm__ volatile(".insn r 0x3b, 0x7, 0x31, x0, x0, x0" ::: "memory");
            }
            break;
        case 0x32:
            __asm__ volatile(".insn r 0x3b, 0x7, 0x32, x0, x0, x0" ::: "memory");
            break;
        case 0x3C:
            if (simple_mode) {
                __asm__ volatile("li t0, 1\n\t.insn r 0x3b, 0x7, 0x3C, x0, x0, t0" ::: "t0", "memory");
            } else {
                __asm__ volatile(".insn r 0x3b, 0x7, 0x3C, x0, x0, x0" ::: "memory");
            }
            break;
        case 0x3D:
            if (simple_mode) {
                __asm__ volatile("li t0, 1\n\t.insn r 0x3b, 0x7, 0x3D, x0, x0, t0" ::: "t0", "memory");
            } else {
                __asm__ volatile(".insn r 0x3b, 0x7, 0x3D, x0, x0, x0" ::: "memory");
            }
            break;
        case 0x3E:
            if (simple_mode) {
                __asm__ volatile("li t0, 1\n\t.insn r 0x3b, 0x7, 0x3E, x0, x0, t0" ::: "t0", "memory");
            } else {
                __asm__ volatile(".insn r 0x3b, 0x7, 0x3E, x0, x0, x0" ::: "memory");
            }
            break;
        case 0x3F:
            if (simple_mode) {
                __asm__ volatile("li t0, 1\n\t.insn r 0x3b, 0x7, 0x3F, x0, x0, t0" ::: "t0", "memory");
            } else {
                __asm__ volatile(".insn r 0x3b, 0x7, 0x3F, x0, x0, x0" ::: "memory");
            }
            break;
        case 0x40:
            __asm__ volatile(".insn r 0x3b, 0x7, 0x40, x0, x0, x0" ::: "memory");
            break;
        case 0x41:
            __asm__ volatile(".insn r 0x3b, 0x7, 0x41, x0, x0, x0" ::: "memory");
            break;
        default:
            break;
    }
}

static inline void spx_hw_exec(uint8_t *out,
                               const uint8_t *pub_seed,
                               const uint8_t *addr,
                               const uint8_t *payload,
                               size_t payload_len,
                               size_t n,
                               uint8_t funct7,
                               int simple_mode) {
    uint32_t idx = 0;
    size_t i;

    for (i = 0; i < n; i += 8) {
        cus_load(spx_load32(pub_seed + i), spx_load32(pub_seed + i + 4), idx);
        idx += 2;
    }

    for (i = 0; i < SPX_ADDR_BYTES; i += 8) {
        cus_load(spx_load32(addr + i), spx_load32(addr + i + 4), idx);
        idx += 2;
    }

    for (i = 0; i < payload_len; i += 8) {
        cus_load(spx_load32(payload + i), spx_load32(payload + i + 4), idx);
        idx += 2;
    }

    __asm__ volatile("fence" ::: "memory");
    spx_hw_trigger(funct7, simple_mode);
    spx_hw_wait();

    for (i = 0; i < n; i += 4) {
        uint32_t w = cus_store((uint32_t)(i / 4));
        spx_store32(out + i, w);
    }
}

static inline uint8_t spx_prf_funct7_for_n(size_t n) {
    if (n == 16) {
        return 0x32;
    }
    if (n == 24) {
        return 0x40;
    }
    return 0x41;
}

static inline uint8_t spx_thash1_funct7_for_n(size_t n) {
    if (n == 16) {
        return 0x30;
    }
    if (n == 24) {
        return 0x3C;
    }
    return 0x3E;
}

static inline uint8_t spx_thash2_funct7_for_n(size_t n) {
    if (n == 16) {
        return 0x31;
    }
    if (n == 24) {
        return 0x3D;
    }
    return 0x3F;
}

#endif
