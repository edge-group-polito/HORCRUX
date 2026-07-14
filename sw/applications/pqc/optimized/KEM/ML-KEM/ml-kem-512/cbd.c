#include <stdint.h>
#include "params.h"
#include "cbd.h"

/*************************************************
* Name:        load32_littleendian
*
* Description: load 4 bytes into a 32-bit integer
*              in little-endian order
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x
**************************************************/
static uint32_t load32_littleendian(const uint8_t x[4])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  r |= (uint32_t)x[3] << 24;
  return r;
}

/*************************************************
* Name:        load24_littleendian
*
* Description: load 3 bytes into a 32-bit integer
*              in little-endian order.
*              This function is only needed for Kyber-512
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x (most significant byte is zero)
**************************************************/
#if KYBER_ETA1 == 3
static uint32_t load24_littleendian(const uint8_t x[3])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  return r;
}
#endif


/*************************************************
* Name:        cbd2
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=2
*              Uses CBD2 custom instruction (opcode=0x3B, funct7=0x01)
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
static void cbd2(poly *r, const uint8_t buf[2*KYBER_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<KYBER_N/8;i++) {
    t  = load32_littleendian(buf+4*i);
    d  = t & 0x55555555;
    d += (t>>1) & 0x55555555;

    /* Use CBD2 custom instruction to extract 8 coefficients */
    asm volatile (
      "mv t0, %[src]\n\t"
      ".insn r 0x3b, 0x7, 0x1, %[dst0], t0, %[x0]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst1], t0, %[x1]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst2], t0, %[x2]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst3], t0, %[x3]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst4], t0, %[x4]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst5], t0, %[x5]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst6], t0, %[x6]\r\n"
      ".insn r 0x3b, 0x7, 0x1, %[dst7], t0, %[x7]\r\n"
      : [dst0] "=r" (r->coeffs[8 * i]    ),
        [dst1] "=r" (r->coeffs[8 * i + 1]),
        [dst2] "=r" (r->coeffs[8 * i + 2]),
        [dst3] "=r" (r->coeffs[8 * i + 3]),
        [dst4] "=r" (r->coeffs[8 * i + 4]),
        [dst5] "=r" (r->coeffs[8 * i + 5]),
        [dst6] "=r" (r->coeffs[8 * i + 6]),
        [dst7] "=r" (r->coeffs[8 * i + 7])
      : [src] "r" (d), 
        [x0] "r" (0),
        [x1] "r" (1),
        [x2] "r" (2),
        [x3] "r" (3),
        [x4] "r" (4),
        [x5] "r" (5),
        [x6] "r" (6),
        [x7] "r" (7)
      : "cc", "t0" );
  }
}

/*************************************************
* Name:        cbd3
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=3.
*              Uses CBD3 custom instruction (opcode=0x3B, funct7=0x02)
*              This function is only needed for Kyber-512
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
#if KYBER_ETA1 == 3
static void cbd3(poly *r, const uint8_t buf[3*KYBER_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<KYBER_N/4;i++) {
    t  = load24_littleendian(buf+3*i);
    d  = t & 0x00249249;
    d += (t>>1) & 0x00249249;
    d += (t>>2) & 0x00249249;

    /* Use CBD3 custom instruction to extract 4 coefficients */
    asm volatile (
    "mv t0, %[src]\n\t"
    ".insn r 0x3b, 0x7, 0x2, %[dst0], %[src], %[x0]\r\n" 
    ".insn r 0x3b, 0x7, 0x2, %[dst1], %[src], %[x1]\r\n"
    ".insn r 0x3b, 0x7, 0x2, %[dst2], %[src], %[x2]\r\n"
    ".insn r 0x3b, 0x7, 0x2, %[dst3], %[src], %[x3]\r\n"
    : [dst0] "=&r" (r->coeffs[4*i+0]),
      [dst1] "=&r" (r->coeffs[4*i+1]),
      [dst2] "=&r" (r->coeffs[4*i+2]),
      [dst3] "=&r" (r->coeffs[4*i+3])
    : [src] "r" (d), 
      [x0] "r" (0), 
      [x1] "r" (1), 
      [x2] "r" (2), 
      [x3] "r" (3)
    : "t0", "cc");
    }   
}
#endif

void poly_cbd_eta1(poly *r, const uint8_t buf[KYBER_ETA1*KYBER_N/4])
{
#if KYBER_ETA1 == 2
  cbd2(r, buf);
#elif KYBER_ETA1 == 3
  cbd3(r, buf);
#else
#error "This implementation requires eta1 in {2,3}"
#endif
}

void poly_cbd_eta2(poly *r, const uint8_t buf[KYBER_ETA2*KYBER_N/4])
{
#if KYBER_ETA2 == 2
  cbd2(r, buf);
#else
#error "This implementation requires eta2 = 2"
#endif
}
