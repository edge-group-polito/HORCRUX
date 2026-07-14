/*
 * SHAKE implementation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

#include <string.h>

#include <stdio.h>

#include "inner.h"

/* Hardware Keccak coprocessor function (from keccak_coproc.S) */
extern void keccak_coproc_S(uint32_t *state);

/*
 * Process the provided state using HW Keccak coprocessor.
 * The state A is an array of 25 x 64-bit words = 50 x 32-bit words.
 */
static void process_block(uint64_t *A)
{
	/* Call HW Keccak-f[1600] permutation via coprocessor */
	keccak_coproc_S((uint32_t *)A);
}


/* see inner.h */
void
Zf(i_shake256_init)(inner_shake256_context *sc)
{
	sc->dptr = 0;

	/*
	 * Representation of an all-ones uint64_t is the same regardless
	 * of local endianness.
	 */
	memset(sc->st.A, 0, sizeof sc->st.A);
}

/* see inner.h */
void
Zf(i_shake256_inject)(inner_shake256_context *sc, const uint8_t *in, size_t len)
{
	size_t dptr;

	dptr = (size_t)sc->dptr;
	while (len > 0) {
		size_t clen, u;

		clen = 136 - dptr;
		if (clen > len) {
			clen = len;
		}
		for (u = 0; u < clen; u ++) {
			size_t v;

			v = u + dptr;
			sc->st.A[v >> 3] ^= (uint64_t)in[u] << ((v & 7) << 3);
		}
		dptr += clen;
		in += clen;
		len -= clen;
		if (dptr == 136) {
			process_block(sc->st.A);
			dptr = 0;
		}
	}
	sc->dptr = dptr;
}

/* see falcon.h */
void
Zf(i_shake256_flip)(inner_shake256_context *sc)
{
	/*
	 * We apply padding and pre-XOR the value into the state. We
	 * set dptr to the end of the buffer, so that first call to
	 * shake_extract() will process the block.
	 */
	unsigned v;

	v = sc->dptr;
	sc->st.A[v >> 3] ^= (uint64_t)0x1F << ((v & 7) << 3);
	sc->st.A[16] ^= (uint64_t)0x80 << 56;
	sc->dptr = 136;
}

/* see falcon.h */
void
Zf(i_shake256_extract)(inner_shake256_context *sc, uint8_t *out, size_t len)
{
	size_t dptr;

	dptr = (size_t)sc->dptr;
	while (len > 0) {
		size_t clen;

		if (dptr == 136) {
			process_block(sc->st.A);
			dptr = 0;
		}
		clen = 136 - dptr;
		if (clen > len) {
			clen = len;
		}
		len -= clen;
		while (clen -- > 0) {
			*out ++ = sc->st.A[dptr >> 3] >> ((dptr & 7) << 3);
			dptr ++;
		}
	}
	sc->dptr = dptr;
}
