// Implementation from PQClean project

/* Based on the public domain implementation in
 * crypto_hash/keccakc512/simple/ from http://bench.cr.yp.to/supercop.html
 * by Ronny Van Keer
 * and the public domain "TweetFips202" implementation
 * from https://twitter.com/tweetfips202
 * by Gilles Van Assche, Daniel J. Bernstein, and Peter Schwabe */

#include <stddef.h>
#include <stdint.h>

#include "fips202.h"

#define NROUNDS        24
#define ROL(a, offset) (((a) << (offset)) ^ ((a) >> (64 - (offset))))

/*************************************************
 * Name:        load64
 *
 * Description: Load 8 bytes into uint64_t in little-endian order
 *
 * Arguments:   - const uint8_t *x: pointer to input byte array
 *
 * Returns the loaded 64-bit unsigned integer
 **************************************************/
static uint64_t load64(const uint8_t *x) {
    uint64_t r = 0;
    for (size_t i = 0; i < 8; ++i) {
        r |= (uint64_t)x[i] << 8 * i;
    }

    return r;
}

/*************************************************
 * Name:        store64
 *
 * Description: Store a 64-bit integer to a byte array in little-endian order
 *
 * Arguments:   - uint8_t *x: pointer to the output byte array
 *              - uint64_t u: input 64-bit unsigned integer
 **************************************************/
static void store64(uint8_t *x, uint64_t u) {
    for (size_t i = 0; i < 8; ++i) {
        x[i] = (uint8_t)(u >> 8 * i);
    }
}

/* Keccak round constants */
static const uint32_t KeccakF1600RoundConstants32[2 * 24] = {
    0x00000000, 0x00000001,
    0x00000000, 0x00008082,
    0x80000000, 0x0000808a,
    0x80000000, 0x80008000,
    0x00000000, 0x0000808b,
    0x00000000, 0x80000001,
    0x80000000, 0x80008081,
    0x80000000, 0x00008009,
    0x00000000, 0x0000008a,
    0x00000000, 0x00000088,
    0x00000000, 0x80008009,
    0x00000000, 0x8000000a,
    0x00000000, 0x8000808b,
    0x80000000, 0x0000008b,
    0x80000000, 0x00008089,
    0x80000000, 0x00008003,
    0x80000000, 0x00008002,
    0x80000000, 0x00000080,
    0x00000000, 0x0000800a,
    0x80000000, 0x8000000a,
    0x80000000, 0x80008081,
    0x80000000, 0x00008080,
    0x00000000, 0x80000001,
    0x80000000, 0x80008008
};

/*************************************************
* Name:        KeccakF1600_StatePermute
*
* Description: The Keccak F1600 Permutation
*
* Arguments:   - uint64_t *state: pointer to input/output Keccak state
**************************************************/
void rol32_FIXED(uint32_t a, uint32_t b, unsigned int offset, uint32_t *result1, uint32_t *result2) {
        uint64_t leftShifted;
        uint64_t rightShifted;
        uint64_t result;
        uint64_t ab;

        ab = (((uint64_t)a << 32) | (uint64_t)b);
        // Perform left shift
        leftShifted = ab << offset;
        // Perform right shift
        rightShifted = ab >> (64 - offset);
        // Combine the results
        result = leftShifted ^ rightShifted;

        *result1 = (uint32_t)(result >> 32); // MSB (upper 32 bits)
        *result2 = (uint32_t)(result & 0xFFFFFFFF); // LSB (lower 32 bits)
    }


void KeccakF1600_StatePermute(uint32_t *state){

    uint32_t Aba0, Abe0, Abi0, Abo0, Abu0;
    uint32_t Aba1, Abe1, Abi1, Abo1, Abu1;
    uint32_t Aga0, Age0, Agi0, Ago0, Agu0;
    uint32_t Aga1, Age1, Agi1, Ago1, Agu1;
    uint32_t Aka0, Ake0, Aki0, Ako0, Aku0;
    uint32_t Aka1, Ake1, Aki1, Ako1, Aku1;
    uint32_t Ama0, Ame0, Ami0, Amo0, Amu0;
    uint32_t Ama1, Ame1, Ami1, Amo1, Amu1;
    uint32_t Asa0, Ase0, Asi0, Aso0, Asu0;
    uint32_t Asa1, Ase1, Asi1, Aso1, Asu1;
    uint32_t BCa0, BCe0, BCi0, BCo0, BCu0;
    uint32_t BCa1, BCe1, BCi1, BCo1, BCu1;
    uint32_t Da0, De0, Di0, Do0, Du0;
    uint32_t Da1, De1, Di1, Do1, Du1;
    uint32_t Eba0, Ebe0, Ebi0, Ebo0, Ebu0;
    uint32_t Eba1, Ebe1, Ebi1, Ebo1, Ebu1;
    uint32_t Ega0, Ege0, Egi0, Ego0, Egu0;
    uint32_t Ega1, Ege1, Egi1, Ego1, Egu1;
    uint32_t Eka0, Eke0, Eki0, Eko0, Eku0;
    uint32_t Eka1, Eke1, Eki1, Eko1, Eku1;
    uint32_t Ema0, Eme0, Emi0, Emo0, Emu0;
    uint32_t Ema1, Eme1, Emi1, Emo1, Emu1;
    uint32_t Esa0, Ese0, Esi0, Eso0, Esu0;
    uint32_t Esa1, Ese1, Esi1, Eso1, Esu1;

    //copyFromState(A, state)
        Aba0 = state[ 0];
        Aba1 = state[ 1];
        Abe0 = state[ 2];
        Abe1 = state[ 3];
        Abi0 = state[ 4];
        Abi1 = state[ 5];
        Abo0 = state[ 6];
        Abo1 = state[ 7];
        Abu0 = state[ 8];
        Abu1 = state[ 9];
        Aga0 = state[10];
        Aga1 = state[11];
        Age0 = state[12];
        Age1 = state[13];
        Agi0 = state[14];
        Agi1 = state[15];
        Ago0 = state[16];
        Ago1 = state[17];
        Agu0 = state[18];
        Agu1 = state[19];
        Aka0 = state[20];
        Aka1 = state[21];
        Ake0 = state[22];
        Ake1 = state[23];
        Aki0 = state[24];
        Aki1 = state[25];
        Ako0 = state[26];
        Ako1 = state[27];
        Aku0 = state[28];
        Aku1 = state[29];
        Ama0 = state[30];
        Ama1 = state[31];
        Ame0 = state[32];
        Ame1 = state[33];
        Ami0 = state[34];
        Ami1 = state[35];
        Amo0 = state[36];
        Amo1 = state[37];
        Amu0 = state[38];
        Amu1 = state[39];
        Asa0 = state[40];
        Asa1 = state[41];
        Ase0 = state[42];
        Ase1 = state[43];
        Asi0 = state[44];
        Asi1 = state[45];
        Aso0 = state[46];
        Aso1 = state[47];
        Asu0 = state[48];
        Asu1 = state[49];

        uint32_t Da0a, Da0b;
        uint32_t De0a, De0b;
        uint32_t Di0a, Di0b;
        uint32_t Do0a, Do0b;
        uint32_t Du0a, Du0b;
        uint32_t BCa0a, BCa0b;
        uint32_t BCe0a, BCe0b;
        uint32_t BCi0a, BCi0b;
        uint32_t BCo0a, BCo0b;
        uint32_t BCu0a, BCu0b;


        for(int round = 0; round < 24; round += 2 )
        {

            BCa0 = Aba0^Aga0^Aka0^Ama0^Asa0;
            BCa1 = Aba1^Aga1^Aka1^Ama1^Asa1;
            
            BCe0 = Abe0^Age0^Ake0^Ame0^Ase0;
            BCe1 = Abe1^Age1^Ake1^Ame1^Ase1;

            BCi0 = Abi0^Agi0^Aki0^Ami0^Asi0;
            BCi1 = Abi1^Agi1^Aki1^Ami1^Asi1;

            BCo0 = Abo0^Ago0^Ako0^Amo0^Aso0;
            BCo1 = Abo1^Ago1^Ako1^Amo1^Aso1;

            BCu0 = Abu0^Agu0^Aku0^Amu0^Asu0;
            BCu1 = Abu1^Agu1^Aku1^Amu1^Asu1;

            rol32_FIXED(BCe1, BCe0, 1, &Da0a, &Da0b);
            Da0 = BCu0^Da0b;
            Da1 = BCu1^Da0a;

            rol32_FIXED(BCi1, BCi0, 1, &De0a, &De0b);
            De0 = BCa0^De0b;
            De1 = BCa1^De0a;

            rol32_FIXED(BCo1, BCo0, 1, &Di0a, &Di0b);
            Di0 = BCe0^Di0b;
            Di1 = BCe1^Di0a;

            rol32_FIXED(BCu1, BCu0, 1, &Do0a, &Do0b);
            Do0 = BCi0^Do0b;
            Do1 = BCi1^Do0a;

            rol32_FIXED(BCa1, BCa0, 1, &Du0a, &Du0b);
            Du0 = BCo0^Du0b;
            Du1 = BCo1^Du0a;

            Aba0 ^= Da0;
            Aba1 ^= Da1;
            BCa0 = Aba0;
            BCa1 = Aba1;
            Age0 ^= De0;
            Age1 ^= De1;

            rol32_FIXED(Age1, Age0, 44, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;

            Aki1 ^= Di1;
            Aki0 ^= Di0;

            rol32_FIXED(Aki1, Aki0, 43, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Amo1 ^= Do1;
            Amo0 ^= Do0;

            rol32_FIXED(Amo1, Amo0, 21, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Asu0 ^= Du0;
            Asu1 ^= Du1;

            rol32_FIXED(Asu1, Asu0, 14, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Eba0 =   BCa0 ^((~BCe0)&  BCi0 );
            Eba0 ^= KeccakF1600RoundConstants32[round*2+1];

            Eba1 =   BCa1 ^((~BCe1)&  BCi1 );
            Eba1 ^= KeccakF1600RoundConstants32[round*2+0];

            Ebe0 =   BCe0 ^((~BCi0)&  BCo0 );
            Ebe1 =   BCe1 ^((~BCi1)&  BCo1 );

            Ebi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Ebi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Ebo0 =   BCo0 ^((~BCu0)&  BCa0 );
            Ebo1 =   BCo1 ^((~BCu1)&  BCa1 );

            Ebu0 =   BCu0 ^((~BCa0)&  BCe0 );
            Ebu1 =   BCu1 ^((~BCa1)&  BCe1 );
    
            Abo0 ^= Do0;
            Abo1 ^= Do1;

            rol32_FIXED(Abo1, Abo0, 28, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;

            Agu0 ^= Du0;
            Agu1 ^= Du1;

            rol32_FIXED(Agu1, Agu0, 20, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;

            Aka1 ^= Da1;
            Aka0 ^= Da0;

            rol32_FIXED(Aka1, Aka0, 3, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Ame1 ^= De1;
            Ame0 ^= De0;

            rol32_FIXED(Ame1, Ame0, 45, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Asi1 ^= Di1;
            Asi0 ^= Di0;


            rol32_FIXED(Asi1, Asi0, 61, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Ega0 =   BCa0 ^((~BCe0)&  BCi0 );
            Ega1 =   BCa1 ^((~BCe1)&  BCi1 );

            Ege0 =   BCe0 ^((~BCi0)&  BCo0 );
            Ege1 =   BCe1 ^((~BCi1)&  BCo1 );
            
            Egi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Egi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Ego0 =   BCo0 ^((~BCu0)&  BCa0 );
            Ego1 =   BCo1 ^((~BCu1)&  BCa1 );

            Egu0 =   BCu0 ^((~BCa0)&  BCe0 );
            Egu1 =   BCu1 ^((~BCa1)&  BCe1 );

            Abe1 ^= De1;
            Abe0 ^= De0;

            rol32_FIXED(Abe1, Abe0, 1, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;

            Agi0 ^= Di0;
            Agi1 ^= Di1;

            rol32_FIXED(Agi1, Agi0, 6, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;

            Ako1 ^= Do1;
            Ako0 ^= Do0;
          
            rol32_FIXED(Ako1, Ako0, 25, &BCi0a, &BCi0b);
            
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Amu0 ^= Du0;
            Amu1 ^= Du1;

            rol32_FIXED(Amu1, Amu0, 8, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Asa0 ^= Da0;
            Asa1 ^= Da1;

            rol32_FIXED(Asa1, Asa0, 18, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Eka0 =   BCa0 ^((~BCe0)&  BCi0 );
            Eka1 =   BCa1 ^((~BCe1)&  BCi1 );

            Eke0 =   BCe0 ^((~BCi0)&  BCo0 );
            Eke1 =   BCe1 ^((~BCi1)&  BCo1 );

            Eki0 =   BCi0 ^((~BCo0)&  BCu0 );
            Eki1 =   BCi1 ^((~BCo1)&  BCu1 );

            Eko0 =   BCo0 ^((~BCu0)&  BCa0 );
            Eko1 =   BCo1 ^((~BCu1)&  BCa1 );

            Eku0 =   BCu0 ^((~BCa0)&  BCe0 );
            Eku1 =   BCu1 ^((~BCa1)&  BCe1 );
    
            Abu1 ^= Du1;
            Abu0 ^= Du0;

            rol32_FIXED(Abu1, Abu0, 27, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;

            Aga0 ^= Da0;
            Aga1 ^= Da1;

            rol32_FIXED(Aga1, Aga0, 36, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;

            Ake0 ^= De0;
            Ake1 ^= De1;

            rol32_FIXED(Ake1, Ake0, 10, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Ami1 ^= Di1;
            Ami0 ^= Di0;

            rol32_FIXED(Ami1, Ami0, 15, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Aso0 ^= Do0;
            Aso1 ^= Do1;

            rol32_FIXED(Aso1, Aso0, 56, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Ema0 =   BCa0 ^((~BCe0)&  BCi0 );
            Ema1 =   BCa1 ^((~BCe1)&  BCi1 );

            Eme0 =   BCe0 ^((~BCi0)&  BCo0 );
            Eme1 =   BCe1 ^((~BCi1)&  BCo1 );

            Emi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Emi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Emo0 =   BCo0 ^((~BCu0)&  BCa0 );
            Emo1 =   BCo1 ^((~BCu1)&  BCa1 );

            Emu0 =   BCu0 ^((~BCa0)&  BCe0 );       
            Emu1 =   BCu1 ^((~BCa1)&  BCe1 );

    
            Abi0 ^= Di0;
            Abi1 ^= Di1;

            rol32_FIXED(Abi1, Abi0, 62, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;

            Ago1 ^= Do1;
            Ago0 ^= Do0;

            rol32_FIXED(Ago1, Ago0, 55, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;

            Aku1 ^= Du1;
            Aku0 ^= Du0;

            rol32_FIXED(Aku1, Aku0, 39, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Ama1 ^= Da1;
            Ama0 ^= Da0;

            rol32_FIXED(Ama1, Ama0, 41, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Ase0 ^= De0;
            Ase1 ^= De1;

            rol32_FIXED(Ase1, Ase0, 2, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Esa0 =   BCa0 ^((~BCe0)&  BCi0 );
            Esa1 =   BCa1 ^((~BCe1)&  BCi1 );

            Ese0 =   BCe0 ^((~BCi0)&  BCo0 );
            Ese1 =   BCe1 ^((~BCi1)&  BCo1 );

            Esi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Esi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Eso0 =   BCo0 ^((~BCu0)&  BCa0 );
            Eso1 =   BCo1 ^((~BCu1)&  BCa1 );

            Esu0 =   BCu0 ^((~BCa0)&  BCe0 );       
            Esu1 =   BCu1 ^((~BCa1)&  BCe1 );

            //    prepareTheta
            BCa0 = Eba0^Ega0^Eka0^Ema0^Esa0;
            BCa1 = Eba1^Ega1^Eka1^Ema1^Esa1;
            BCe0 = Ebe0^Ege0^Eke0^Eme0^Ese0;
            BCe1 = Ebe1^Ege1^Eke1^Eme1^Ese1;
            BCi0 = Ebi0^Egi0^Eki0^Emi0^Esi0;
            BCi1 = Ebi1^Egi1^Eki1^Emi1^Esi1;
            BCo0 = Ebo0^Ego0^Eko0^Emo0^Eso0;
            BCo1 = Ebo1^Ego1^Eko1^Emo1^Eso1;
            BCu0 = Ebu0^Egu0^Eku0^Emu0^Esu0;
            BCu1 = Ebu1^Egu1^Eku1^Emu1^Esu1;

            //thetaRhoPiChiIota(round+1, E, A)
            rol32_FIXED(BCe1, BCe0, 1, &BCe0a, &BCe0b);
            Da0 = BCe0b ^ BCu0;
            Da1 = BCe0a ^ BCu1;

            rol32_FIXED(BCi1, BCi0, 1, &BCi0a, &BCi0b);
            De0 = BCi0b ^ BCa0;
            De1 = BCi0a ^ BCa1;

            rol32_FIXED(BCo1, BCo0, 1, &BCo0a, &BCo0b);
            Di0 = BCo0b ^ BCe0;
            Di1 = BCo0a ^ BCe1;

            rol32_FIXED(BCu1, BCu0, 1, &BCu0a, &BCu0b);
            Do0 = BCu0b ^ BCi0;
            Do1 = BCu0a ^ BCi1;

            rol32_FIXED(BCa1, BCa0, 1, &BCa0a, &BCa0b);
            Du0 = BCa0b ^ BCo0;
            Du1 = BCa0a ^ BCo1;

            Eba0 ^= Da0;
            Eba1 ^= Da1;

            BCa0 = Eba0;
            BCa1 = Eba1;

            Ege0 ^= De0;
            Ege1 ^= De1;

            rol32_FIXED(Ege1, Ege0, 44, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;

            Eki1 ^= Di1;
            Eki0 ^= Di0;

            rol32_FIXED(Eki1, Eki0, 43, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Emo1 ^= Do1;
            Emo0 ^= Do0;

            rol32_FIXED(Emo1, Emo0, 21, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Esu0 ^= Du0;
            Esu1 ^= Du1;

            rol32_FIXED(Esu1, Esu0, 14, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Aba0 =   BCa0 ^((~BCe0)&  BCi0 );
            Aba1 =   BCa1 ^((~BCe1)&  BCi1 );

            Aba0 ^= KeccakF1600RoundConstants32[round*2+3];
            Aba1 ^= KeccakF1600RoundConstants32[round*2+2];

            Abe0 =   BCe0 ^((~BCi0)&  BCo0 );
            Abe1 =   BCe1 ^((~BCi1)&  BCo1 );

            Abi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Abi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Abo0 =   BCo0 ^((~BCu0)&  BCa0 );
            Abo1 =   BCo1 ^((~BCu1)&  BCa1 );

            Abu0 =   BCu0 ^((~BCa0)&  BCe0 );
            Abu1 =   BCu1 ^((~BCa1)&  BCe1 );
    
            Ebo0 ^= Do0;
            Ebo1 ^= Do1;
            
            rol32_FIXED(Ebo1, Ebo0, 28, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;

            Egu0 ^= Du0;
            Egu1 ^= Du1;

            rol32_FIXED(Egu1, Egu0, 20, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;


            Eka1 ^= Da1;
            Eka0 ^= Da0;

            rol32_FIXED(Eka1, Eka0, 3, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;

            Eme1 ^= De1;
            Eme0 ^= De0;

            rol32_FIXED(Eme1, Eme0, 45, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Esi1 ^= Di1;
            Esi0 ^= Di0;

            rol32_FIXED(Esi1, Esi0, 61, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;

            Aga0 =   BCa0 ^((~BCe0)&  BCi0 );
            Aga1 =   BCa1 ^((~BCe1)&  BCi1 );

            Age0 =   BCe0 ^((~BCi0)&  BCo0 );
            Age1 =   BCe1 ^((~BCi1)&  BCo1 );

            Agi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Agi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Ago0 =   BCo0 ^((~BCu0)&  BCa0 );
            Ago1 =   BCo1 ^((~BCu1)&  BCa1 );

            Agu0 =   BCu0 ^((~BCa0)&  BCe0 );       
            Agu1 =   BCu1 ^((~BCa1)&  BCe1 );


            Ebe1 ^= De1;
            Ebe0 ^= De0;

            rol32_FIXED(Ebe1, Ebe0, 1, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;

            Egi0 ^= Di0;
            Egi1 ^= Di1;
            
            rol32_FIXED(Egi1, Egi0, 6, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a;
            
            Eko1 ^= Do1;
            Eko0 ^= Do0;
            
            rol32_FIXED(Eko1, Eko0, 25, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a;
            
            Emu0 ^= Du0;
            Emu1 ^= Du1;

            rol32_FIXED(Emu1, Emu0, 8, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a;

            Esa0 ^= Da0;
            Esa1 ^= Da1;

            rol32_FIXED(Esa1, Esa0, 18, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a;         
            
            Aka0 =   BCa0 ^((~BCe0)&  BCi0 );
            Aka1 =   BCa1 ^((~BCe1)&  BCi1 );

            Ake0 =   BCe0 ^((~BCi0)&  BCo0 );
            Ake1 =   BCe1 ^((~BCi1)&  BCo1 );

            Aki0 =   BCi0 ^((~BCo0)&  BCu0 );
            Aki1 =   BCi1 ^((~BCo1)&  BCu1 );

            Ako0 =   BCo0 ^((~BCu0)&  BCa0 );
            Ako1 =   BCo1 ^((~BCu1)&  BCa1 );

            Aku0 =   BCu0 ^((~BCa0)&  BCe0 );
            Aku1 =   BCu1 ^((~BCa1)&  BCe1 );

            Ebu1 ^= Du1;
            Ebu0 ^= Du0;

            rol32_FIXED(Ebu1, Ebu0, 27, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a;   
            
            Ega0 ^= Da0;
            Ega1 ^= Da1;
            
            rol32_FIXED(Ega1, Ega0, 36, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a; 
            
            Eke0 ^= De0;
            Eke1 ^= De1;

            rol32_FIXED(Eke1, Eke0, 10, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a; 
            
            Emi1 ^= Di1;
            Emi0 ^= Di0;
            
            rol32_FIXED(Emi1, Emi0, 15, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a; 
            
            Eso0 ^= Do0;
            Eso1 ^= Do1;

            rol32_FIXED(Eso1, Eso0, 56, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a; 
            
            Ama0 =   BCa0 ^((~BCe0)&  BCi0 );
            Ama1 =   BCa1 ^((~BCe1)&  BCi1 );
            
            Ame0 =   BCe0 ^((~BCi0)&  BCo0 );
            Ame1 =   BCe1 ^((~BCi1)&  BCo1 );
            
            Ami0 =   BCi0 ^((~BCo0)&  BCu0 );
            Ami1 =   BCi1 ^((~BCo1)&  BCu1 );
            
            Amo0 =   BCo0 ^((~BCu0)&  BCa0 );
            Amo1 =   BCo1 ^((~BCu1)&  BCa1 );
            
            Amu0 =   BCu0 ^((~BCa0)&  BCe0 );       
            Amu1 =   BCu1 ^((~BCa1)&  BCe1 );

            Ebi0 ^= Di0;
            Ebi1 ^= Di1;

            rol32_FIXED(Ebi1, Ebi0, 62, &BCa0a, &BCa0b);
            BCa0 = BCa0b;
            BCa1 = BCa0a; 

            Ego1 ^= Do1;
            Ego0 ^= Do0;

            rol32_FIXED(Ego1, Ego0, 55, &BCe0a, &BCe0b);
            BCe0 = BCe0b;
            BCe1 = BCe0a; 

            Eku1 ^= Du1;
            Eku0 ^= Du0;

            rol32_FIXED(Eku1, Eku0, 39, &BCi0a, &BCi0b);
            BCi0 = BCi0b;
            BCi1 = BCi0a; 

            Ema1 ^= Da1;
            Ema0 ^= Da0;

            rol32_FIXED(Ema1, Ema0, 41, &BCo0a, &BCo0b);
            BCo0 = BCo0b;
            BCo1 = BCo0a; 

            Ese0 ^= De0;
            Ese1 ^= De1;

            rol32_FIXED(Ese1, Ese0, 2, &BCu0a, &BCu0b);
            BCu0 = BCu0b;
            BCu1 = BCu0a; 

            Asa0 =   BCa0 ^((~BCe0)&  BCi0 );
            Asa1 =   BCa1 ^((~BCe1)&  BCi1 );

            Ase0 =   BCe0 ^((~BCi0)&  BCo0 );
            Ase1 =   BCe1 ^((~BCi1)&  BCo1 );

            Asi0 =   BCi0 ^((~BCo0)&  BCu0 );
            Asi1 =   BCi1 ^((~BCo1)&  BCu1 );

            Aso0 =   BCo0 ^((~BCu0)&  BCa0 );
            Aso1 =   BCo1 ^((~BCu1)&  BCa1 );

            Asu0 =   BCu0 ^((~BCa0)&  BCe0 );       
            Asu1 =   BCu1 ^((~BCa1)&  BCe1 );
            
        }

        //copyToState(state, A)
        state[ 0] = Aba0;
        state[ 1] = Aba1;
        state[ 2] = Abe0;
        state[ 3] = Abe1;
        state[ 4] = Abi0;
        state[ 5] = Abi1;
        state[ 6] = Abo0;
        state[ 7] = Abo1;
        state[ 8] = Abu0;
        state[ 9] = Abu1;
        state[10] = Aga0;
        state[11] = Aga1;
        state[12] = Age0;
        state[13] = Age1;
        state[14] = Agi0;
        state[15] = Agi1;
        state[16] = Ago0;
        state[17] = Ago1;
        state[18] = Agu0;
        state[19] = Agu1;
        state[20] = Aka0;
        state[21] = Aka1;
        state[22] = Ake0;
        state[23] = Ake1;
        state[24] = Aki0;
        state[25] = Aki1;
        state[26] = Ako0;
        state[27] = Ako1;
        state[28] = Aku0;
        state[29] = Aku1;
        state[30] = Ama0;
        state[31] = Ama1;
        state[32] = Ame0;
        state[33] = Ame1;
        state[34] = Ami0;
        state[35] = Ami1;
        state[36] = Amo0;
        state[37] = Amo1;
        state[38] = Amu0;
        state[39] = Amu1;
        state[40] = Asa0;
        state[41] = Asa1;
        state[42] = Ase0;
        state[43] = Ase1;
        state[44] = Asi0;
        state[45] = Asi1;
        state[46] = Aso0;
        state[47] = Aso1;
        state[48] = Asu0;
        state[49] = Asu1;

}
/*************************************************
 * Name:        keccak_absorb
 *
 * Description: Absorb step of Keccak;
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - uint64_t *s: pointer to (uninitialized) output Keccak state
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 *              - const uint8_t *m: pointer to input to be absorbed into s
 *              - size_t mlen: length of input in bytes
 *              - uint8_t p: domain-separation byte for different
 *                                 Keccak-derived functions
 **************************************************/
static void keccak_absorb(uint64_t *s, uint32_t r, const uint8_t *m, size_t mlen, uint8_t p) {
    size_t i;
    uint8_t t[200];

    /* Zero state */
    for (i = 0; i < 25; ++i) {
        s[i] = 0;
    }

    while (mlen >= r) {
        for (i = 0; i < r / 8; ++i) {
            s[i] ^= load64(m + 8 * i);
        }

        KeccakF1600_StatePermute(s);
        mlen -= r;
        m += r;
    }

    for (i = 0; i < r; ++i) {
        t[i] = 0;
    }
    for (i = 0; i < mlen; ++i) {
        t[i] = m[i];
    }
    t[i] = p;
    t[r - 1] |= 128;
    for (i = 0; i < r / 8; ++i) {
        s[i] ^= load64(t + 8 * i);
    }
}

/*************************************************
 * Name:        keccak_squeezeblocks
 *
 * Description: Squeeze step of Keccak. Squeezes full blocks of r bytes each.
 *              Modifies the state. Can be called multiple times to keep
 *              squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *h: pointer to output blocks
 *              - size_t nblocks: number of blocks to be
 *                                                squeezed (written to h)
 *              - uint64_t *s: pointer to input/output Keccak state
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 **************************************************/
static void keccak_squeezeblocks(uint8_t *h, size_t nblocks, uint64_t *s, uint32_t r) {
    while (nblocks > 0) {
        KeccakF1600_StatePermute(s);
        for (size_t i = 0; i < (r >> 3); i++) {
            store64(h + 8 * i, s[i]);
        }
        h += r;
        nblocks--;
    }
}

/*************************************************
 * Name:        keccak_inc_init
 *
 * Description: Initializes the incremental Keccak state to zero.
 *
 * Arguments:   - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 **************************************************/
static void keccak_inc_init(uint64_t *s_inc) {
    size_t i;

    for (i = 0; i < 25; ++i) {
        s_inc[i] = 0;
    }
    s_inc[25] = 0;
}

/*************************************************
 * Name:        keccak_inc_absorb
 *
 * Description: Incremental keccak absorb
 *              Preceded by keccak_inc_init, succeeded by keccak_inc_finalize
 *
 * Arguments:   - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 *              - const uint8_t *m: pointer to input to be absorbed into s
 *              - size_t mlen: length of input in bytes
 **************************************************/
static void keccak_inc_absorb(uint64_t *s_inc, uint32_t r, const uint8_t *m, size_t mlen) {
    size_t i;

    /* Recall that s_inc[25] is the non-absorbed bytes xored into the state */
    while (mlen + s_inc[25] >= r) {
        for (i = 0; i < r - (uint32_t)s_inc[25]; i++) {
            /* Take the i'th byte from message
               xor with the s_inc[25] + i'th byte of the state; little-endian */
            s_inc[(s_inc[25] + i) >> 3] ^= (uint64_t)m[i] << (8 * ((s_inc[25] + i) & 0x07));
        }
        mlen -= (size_t)(r - s_inc[25]);
        m += r - s_inc[25];
        s_inc[25] = 0;

        KeccakF1600_StatePermute(s_inc);
    }

    for (i = 0; i < mlen; i++) {
        s_inc[(s_inc[25] + i) >> 3] ^= (uint64_t)m[i] << (8 * ((s_inc[25] + i) & 0x07));
    }
    s_inc[25] += mlen;
}

/*************************************************
 * Name:        keccak_inc_finalize
 *
 * Description: Finalizes Keccak absorb phase, prepares for squeezing
 *
 * Arguments:   - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 *              - uint8_t p: domain-separation byte for different
 *                                 Keccak-derived functions
 **************************************************/
static void keccak_inc_finalize(uint64_t *s_inc, uint32_t r, uint8_t p) {
    /* After keccak_inc_absorb, we are guaranteed that s_inc[25] < r,
       so we can always use one more byte for p in the current state. */
    s_inc[s_inc[25] >> 3] ^= (uint64_t)p << (8 * (s_inc[25] & 0x07));
    s_inc[(r - 1) >> 3] ^= (uint64_t)128 << (8 * ((r - 1) & 0x07));
    s_inc[25] = 0;
}

/*************************************************
 * Name:        keccak_inc_squeeze
 *
 * Description: Incremental Keccak squeeze; can be called on byte-level
 *
 * Arguments:   - uint8_t *h: pointer to output bytes
 *              - size_t outlen: number of bytes to be squeezed
 *              - uint64_t *s_inc: pointer to input/output incremental state
 *                First 25 values represent Keccak state.
 *                26th value represents either the number of absorbed bytes
 *                that have not been permuted, or not-yet-squeezed bytes.
 *              - uint32_t r: rate in bytes (e.g., 168 for SHAKE128)
 **************************************************/
static void keccak_inc_squeeze(uint8_t *h, size_t outlen, uint64_t *s_inc, uint32_t r) {
    size_t i;

    /* First consume any bytes we still have sitting around */
    for (i = 0; i < outlen && i < s_inc[25]; i++) {
        /* There are s_inc[25] bytes left, so r - s_inc[25] is the first
           available byte. We consume from there, i.e., up to r. */
        h[i] = (uint8_t)(s_inc[(r - s_inc[25] + i) >> 3] >> (8 * ((r - s_inc[25] + i) & 0x07)));
    }
    h += i;
    outlen -= i;
    s_inc[25] -= i;

    /* Then squeeze the remaining necessary blocks */
    while (outlen > 0) {
        KeccakF1600_StatePermute(s_inc);

        for (i = 0; i < outlen && i < r; i++) {
            h[i] = (uint8_t)(s_inc[i >> 3] >> (8 * (i & 0x07)));
        }
        h += i;
        outlen -= i;
        s_inc[25] = r - i;
    }
}

void shake128_inc_init(shake128incctx *state) {
    keccak_inc_init(state->ctx);
}

void shake128_inc_absorb(shake128incctx *state, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(state->ctx, SHAKE128_RATE, input, inlen);
}

void shake128_inc_finalize(shake128incctx *state) {
    keccak_inc_finalize(state->ctx, SHAKE128_RATE, 0x1F);
}

void shake128_inc_squeeze(uint8_t *output, size_t outlen, shake128incctx *state) {
    keccak_inc_squeeze(output, outlen, state->ctx, SHAKE128_RATE);
}

void shake256_inc_init(shake256incctx *state) {
    keccak_inc_init(state->ctx);
}

void shake256_inc_absorb(shake256incctx *state, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(state->ctx, SHAKE256_RATE, input, inlen);
}

void shake256_inc_finalize(shake256incctx *state) {
    keccak_inc_finalize(state->ctx, SHAKE256_RATE, 0x1F);
}

void shake256_inc_squeeze(uint8_t *output, size_t outlen, shake256incctx *state) {
    keccak_inc_squeeze(output, outlen, state->ctx, SHAKE256_RATE);
}

/*************************************************
 * Name:        shake128_absorb
 *
 * Description: Absorb step of the SHAKE128 XOF.
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - uint64_t *s: pointer to (uninitialized) output Keccak state
 *              - const uint8_t *input: pointer to input to be absorbed
 *                                            into s
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake128_absorb(shake128ctx *state, const uint8_t *input, size_t inlen) {
    keccak_absorb(state->ctx, SHAKE128_RATE, input, inlen, 0x1F);
}

/*************************************************
 * Name:        shake128_squeezeblocks
 *
 * Description: Squeeze step of SHAKE128 XOF. Squeezes full blocks of
 *              SHAKE128_RATE bytes each. Modifies the state. Can be called
 *              multiple times to keep squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *output: pointer to output blocks
 *              - size_t nblocks: number of blocks to be squeezed
 *                                            (written to output)
 *              - shake128ctx *state: pointer to input/output Keccak state
 **************************************************/
void shake128_squeezeblocks(uint8_t *output, size_t nblocks, shake128ctx *state) {
    keccak_squeezeblocks(output, nblocks, state->ctx, SHAKE128_RATE);
}

/*************************************************
 * Name:        shake256_absorb
 *
 * Description: Absorb step of the SHAKE256 XOF.
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - shake256ctx *state: pointer to (uninitialized) output Keccak state
 *              - const uint8_t *input: pointer to input to be absorbed
 *                                            into s
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256_absorb(shake256ctx *state, const uint8_t *input, size_t inlen) {
    keccak_absorb(state->ctx, SHAKE256_RATE, input, inlen, 0x1F);
}

/*************************************************
 * Name:        shake256_squeezeblocks
 *
 * Description: Squeeze step of SHAKE256 XOF. Squeezes full blocks of
 *              SHAKE256_RATE bytes each. Modifies the state. Can be called
 *              multiple times to keep squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *output: pointer to output blocks
 *              - size_t nblocks: number of blocks to be squeezed
 *                                (written to output)
 *              - shake256ctx *state: pointer to input/output Keccak state
 **************************************************/
void shake256_squeezeblocks(uint8_t *output, size_t nblocks, shake256ctx *state) {
    keccak_squeezeblocks(output, nblocks, state->ctx, SHAKE256_RATE);
}

/*************************************************
 * Name:        shake128
 *
 * Description: SHAKE128 XOF with non-incremental API
 *
 * Arguments:   - uint8_t *output: pointer to output
 *              - size_t outlen: requested output length in bytes
 *              - const uint8_t *input: pointer to input
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake128(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen) {
    size_t nblocks = outlen / SHAKE128_RATE;
    uint8_t t[SHAKE128_RATE];
    shake128ctx s;

    shake128_absorb(&s, input, inlen);
    shake128_squeezeblocks(output, nblocks, &s);

    output += nblocks * SHAKE128_RATE;
    outlen -= nblocks * SHAKE128_RATE;

    if (outlen) {
        shake128_squeezeblocks(t, 1, &s);
        for (size_t i = 0; i < outlen; ++i) {
            output[i] = t[i];
        }
    }
}

/*************************************************
 * Name:        shake256
 *
 * Description: SHAKE256 XOF with non-incremental API
 *
 * Arguments:   - uint8_t *output: pointer to output
 *              - size_t outlen: requested output length in bytes
 *              - const uint8_t *input: pointer to input
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen) {
    size_t nblocks = outlen / SHAKE256_RATE;
    uint8_t t[SHAKE256_RATE];
    shake256ctx s;

    shake256_absorb(&s, input, inlen);
    shake256_squeezeblocks(output, nblocks, &s);

    output += nblocks * SHAKE256_RATE;
    outlen -= nblocks * SHAKE256_RATE;

    if (outlen) {
        shake256_squeezeblocks(t, 1, &s);
        for (size_t i = 0; i < outlen; ++i) {
            output[i] = t[i];
        }
    }
}

void sha3_256_inc_init(sha3_256incctx *state) {
    keccak_inc_init(state->ctx);
}

void sha3_256_inc_absorb(sha3_256incctx *state, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(state->ctx, SHA3_256_RATE, input, inlen);
}

void sha3_256_inc_finalize(uint8_t *output, sha3_256incctx *state) {
    uint8_t t[SHA3_256_RATE];
    keccak_inc_finalize(state->ctx, SHA3_256_RATE, 0x06);

    keccak_squeezeblocks(t, 1, state->ctx, SHA3_256_RATE);

    for (size_t i = 0; i < 32; i++) {
        output[i] = t[i];
    }
}

/*************************************************
 * Name:        sha3_256
 *
 * Description: SHA3-256 with non-incremental API
 *
 * Arguments:   - uint8_t *output:      pointer to output
 *              - const uint8_t *input: pointer to input
 *              - size_t inlen:   length of input in bytes
 **************************************************/
void sha3_256(uint8_t *output, const uint8_t *input, size_t inlen) {
    uint64_t s[25];
    uint8_t t[SHA3_256_RATE];

    /* Absorb input */
    keccak_absorb(s, SHA3_256_RATE, input, inlen, 0x06);

    /* Squeeze output */
    keccak_squeezeblocks(t, 1, s, SHA3_256_RATE);

    for (size_t i = 0; i < 32; i++) {
        output[i] = t[i];
    }
}

void sha3_384_inc_init(sha3_384incctx *state) {
    keccak_inc_init(state->ctx);
}

void sha3_384_inc_absorb(sha3_384incctx *state, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(state->ctx, SHA3_384_RATE, input, inlen);
}

void sha3_384_inc_finalize(uint8_t *output, sha3_384incctx *state) {
    uint8_t t[SHA3_384_RATE];
    keccak_inc_finalize(state->ctx, SHA3_384_RATE, 0x06);

    keccak_squeezeblocks(t, 1, state->ctx, SHA3_384_RATE);

    for (size_t i = 0; i < 48; i++) {
        output[i] = t[i];
    }
}

/*************************************************
 * Name:        sha3_384
 *
 * Description: SHA3-256 with non-incremental API
 *
 * Arguments:   - uint8_t *output:      pointer to output
 *              - const uint8_t *input: pointer to input
 *              - size_t inlen:   length of input in bytes
 **************************************************/
void sha3_384(uint8_t *output, const uint8_t *input, size_t inlen) {
    uint64_t s[25];
    uint8_t t[SHA3_384_RATE];

    /* Absorb input */
    keccak_absorb(s, SHA3_384_RATE, input, inlen, 0x06);

    /* Squeeze output */
    keccak_squeezeblocks(t, 1, s, SHA3_384_RATE);

    for (size_t i = 0; i < 48; i++) {
        output[i] = t[i];
    }
}

void sha3_512_inc_init(sha3_512incctx *state) {
    keccak_inc_init(state->ctx);
}

void sha3_512_inc_absorb(sha3_512incctx *state, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(state->ctx, SHA3_512_RATE, input, inlen);
}

void sha3_512_inc_finalize(uint8_t *output, sha3_512incctx *state) {
    uint8_t t[SHA3_512_RATE];
    keccak_inc_finalize(state->ctx, SHA3_512_RATE, 0x06);

    keccak_squeezeblocks(t, 1, state->ctx, SHA3_512_RATE);

    for (size_t i = 0; i < 64; i++) {
        output[i] = t[i];
    }
}

/*************************************************
 * Name:        sha3_512
 *
 * Description: SHA3-512 with non-incremental API
 *
 * Arguments:   - uint8_t *output:      pointer to output
 *              - const uint8_t *input: pointer to input
 *              - size_t inlen:   length of input in bytes
 **************************************************/
void sha3_512(uint8_t *output, const uint8_t *input, size_t inlen) {
    uint64_t s[25];
    uint8_t t[SHA3_512_RATE];

    /* Absorb input */
    keccak_absorb(s, SHA3_512_RATE, input, inlen, 0x06);

    /* Squeeze output */
    keccak_squeezeblocks(t, 1, s, SHA3_512_RATE);

    for (size_t i = 0; i < 64; i++) {
        output[i] = t[i];
    }
}
