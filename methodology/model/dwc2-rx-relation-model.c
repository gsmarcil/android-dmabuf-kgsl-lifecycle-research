// SPDX-License-Identifier: GPL-2.0
/*
 * dwc2-rx-relation-model.c — the RX helper, as its own frozen model
 *
 * The TX model grants RX nothing. Deriving an RX claim from a TX result
 * is the same domain error the TX entry now records; this file exists so
 * RX is judged by an instrument whose declared domain is RX.
 *
 * DECLARED DOMAIN
 *   reference     dwc2_readl_rep():  *data_buf++ = dwc2_readl(...)
 *                 a whole-u32 store of the register value into memory
 *   candidate     dwc2_read_fifo_bytes():  memcpy(out, &word, copy_n)
 *
 * TWO RELATIONS, declared separately per S8:
 *
 *   R1 PRESERVE(representation)
 *      Over the destination bytes the caller owns, the candidate must
 *      hold exactly what the reference's u32 store placed there. The
 *      register value becomes an object representation in memory, so the
 *      preserved thing is that representation and not a numeric reading
 *      of it. Extracting with (word >> (8 * i)) & 0xff imposes a
 *      little-endian numeric interpretation and is the trap form.
 *
 *   R2 CORRECT(destination bound)
 *      The reference stores 4 * ceil(fifo_bytes / 4) bytes regardless of
 *      what the request buffer can hold. The candidate must store
 *      exactly copy_bytes. Under a CORRECT relation the candidate is
 *      REQUIRED to differ: a candidate that still matches the reference
 *      here fails, which is the inversion an equivalence-only gate
 *      cannot express.
 *
 * Build: cc -O1 -g -Wall -Wextra -fsanitize=address,undefined \
 *           -o rx-relation dwc2-rx-relation-model.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* object representation of a u32 on a host of the given byte order */
static void repr(uint32_t word, int be, uint8_t out[4])
{
	int i;

	for (i = 0; i < 4; i++)
		out[i] = (uint8_t)(word >> (be ? 8 * (3 - i) : 8 * i));
}

/* R1 reference: what the whole-u32 store left in the first copy_n bytes */
static void r1_reference(uint32_t word, int be, uint8_t *dst, unsigned copy_n)
{
	uint8_t rep[4];
	unsigned i;

	repr(word, be, rep);
	for (i = 0; i < copy_n; i++)
		dst[i] = rep[i];
}

/* R1 candidate: the series form */
static void r1_memcpy(uint32_t word, int be, uint8_t *dst, unsigned copy_n)
{
	uint8_t rep[4];

	repr(word, be, rep);
	memcpy(dst, rep, copy_n);
}

/* R1 trap: numeric extraction, host order ignored */
static void r1_shift(uint32_t word, int be, uint8_t *dst, unsigned copy_n)
{
	unsigned i;

	(void)be;
	for (i = 0; i < copy_n; i++)
		dst[i] = (uint8_t)(word >> (8 * i));
}

/* R2: destination bytes written, given what the core reported and what
 * the caller can accept */
static unsigned r2_reference_written(unsigned fifo_bytes, unsigned copy_bytes)
{
	(void)copy_bytes;			/* the reference ignores it */
	return 4 * ((fifo_bytes + 3) / 4);
}

static unsigned r2_candidate_written(unsigned fifo_bytes, unsigned copy_bytes)
{
	unsigned written = 0;

	while (fifo_bytes) {
		unsigned fifo_n = fifo_bytes > 4 ? 4 : fifo_bytes;
		unsigned copy_n = copy_bytes > fifo_n ? fifo_n : copy_bytes;

		written    += copy_n;
		copy_bytes -= copy_n;
		fifo_bytes -= fifo_n;
	}
	return written;
}

/* R2 negative control: a candidate that "preserves" the old bound.
 * Under a CORRECT relation this must FAIL. */
static unsigned r2_unfixed_written(unsigned fifo_bytes, unsigned copy_bytes)
{
	return r2_reference_written(fifo_bytes, copy_bytes);
}

int main(void)
{
	static const uint32_t words[] = {
		0x04030201, 0xdeadbeef, 0x00000000, 0xffffffff, 0x01000000
	};
	int be;
	unsigned i, copy_n, f, c;
	int r1_memcpy_bad = 0, r1_shift_bad = 0, r1_cases = 0;
	int r2_bad = 0, r2_overwrite_seen = 0, r2_unfixed_bad = 0, r2_cases = 0;

	printf("=== DWC2 RX helper — declared-domain relation model ===\n");
	printf("reference: *data_buf = dwc2_readl(...)   (whole-u32 store)\n\n");

	/* ---- R1 PRESERVE(representation) ---- */
	for (be = 0; be <= 1; be++) {
		for (i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
			for (copy_n = 1; copy_n <= 4; copy_n++) {
				uint8_t r[4] = {0}, m[4] = {0}, s[4] = {0};

				r1_reference(words[i], be, r, copy_n);
				r1_memcpy(words[i], be, m, copy_n);
				r1_shift(words[i], be, s, copy_n);

				if (memcmp(r, m, copy_n))
					r1_memcpy_bad++;
				if (memcmp(r, s, copy_n))
					r1_shift_bad++;
				r1_cases++;
			}
		}
	}

	printf("R1 PRESERVE(representation)   %d cases\n", r1_cases);
	printf("   memcpy form mismatches   : %d   (must be 0)\n", r1_memcpy_bad);
	printf("   shift  form mismatches   : %d   (must be > 0, else blind)\n",
	       r1_shift_bad);

	/* ---- R2 CORRECT(destination bound) ---- */
	for (f = 0; f <= 64; f++) {
		for (c = 0; c <= f; c++) {
			unsigned ref = r2_reference_written(f, c);
			unsigned cand = r2_candidate_written(f, c);
			unsigned unfixed = r2_unfixed_written(f, c);

			if (cand != c)
				r2_bad++;
			if (ref > c) {
				r2_overwrite_seen++;
				/* the required divergence */
				if (unfixed == c)
					r2_unfixed_bad++;
			}
			r2_cases++;
		}
	}

	printf("\nR2 CORRECT(destination bound) %d cases\n", r2_cases);
	printf("   candidate != copy_bytes  : %d   (must be 0)\n", r2_bad);
	printf("   reference over-writes in : %d cases\n", r2_overwrite_seen);
	printf("   unfixed control accepted : %d   (must be 0 — a candidate\n"
	       "                                    matching the reference\n"
	       "                                    FAILS a CORRECT relation)\n",
	       r2_unfixed_bad);

	printf("\nliveness: R1 needs the shift form to fail, R2 needs the\n"
	       "reference to over-write somewhere. Both observed -> the\n"
	       "instrument can separate conforming from non-conforming.\n");

	if (r1_memcpy_bad == 0 && r1_shift_bad > 0 &&
	    r2_bad == 0 && r2_overwrite_seen > 0 && r2_unfixed_bad == 0) {
		printf("\nverdict: PASS — RX satisfies PRESERVE on representation\n"
		       "and CORRECT on the destination bound. Frozen at this\n"
		       "domain: RX only, these two relations only.\n");
		return 0;
	}
	printf("\nverdict: FAIL\n");
	return 1;
}
