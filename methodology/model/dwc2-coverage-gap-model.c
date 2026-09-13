// SPDX-License-Identifier: GPL-2.0
/*
 * dwc2-coverage-gap-model.c — what the frozen TX model does NOT cover
 *
 * The frozen model (dwc2-tx-equivalence-model.c) uses *(u32 *)src as its
 * reference. That is dwc2_hc_write_packet()'s ALIGNED branch only. The
 * series replaces more than that, so two changed properties fall outside
 * the frozen model's scope. Per frozen-model binding the frozen model may
 * NOT be extended to reach them — a model edited after the fact stops
 * being evidence. This is a separate instrument.
 *
 *   A  dwc2_hc_write_packet()'s UNALIGNED branch. data_buf is u32 *, so
 *      data_buf[1] is +4 BYTES, not +1. Mainline reads 16 bytes per
 *      iteration while advancing 4, and ORs whole u32 lanes together.
 *      It is neither a representation load nor a byte assembly.
 *      Claim under test: the patch is NOT equivalent to this branch, and
 *      that non-equivalence is intended.
 *
 *   B  the RX helper. Same representation property as TX, opposite
 *      direction, no frozen model anywhere.
 *
 * Reads are modelled by index, never performed out of bounds, so the
 * over-read is reported rather than executed.
 *
 * Build: cc -O1 -g -Wall -Wextra -fsanitize=address,undefined \
 *           -o coverage-gap dwc2-coverage-gap-model.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BUFCAP 64

struct probe {
	uint8_t  mem[BUFCAP];	/* payload, then poison */
	unsigned payload;	/* bytes the caller actually owns */
	unsigned max_touched;	/* highest byte index read, +1 */
};

static void probe_init(struct probe *p, unsigned payload)
{
	unsigned i;

	for (i = 0; i < BUFCAP; i++)
		p->mem[i] = (i < payload) ? (uint8_t)(i + 1) : 0xEE;
	p->payload = payload;
	p->max_touched = 0;
}

static uint8_t rd(struct probe *p, unsigned i)
{
	if (i + 1 > p->max_touched)
		p->max_touched = i + 1;
	return (i < BUFCAP) ? p->mem[i] : 0xEE;
}

static uint32_t load_word(struct probe *p, unsigned byte_off, int be)
{
	uint32_t w = 0;
	unsigned i;

	for (i = 0; i < 4; i++)
		w |= (uint32_t)rd(p, byte_off + i) << (be ? 8 * (3 - i) : 8 * i);
	return w;
}

/* ---- A: dwc2_hc_write_packet, the three forms ------------------------ */

/* aligned branch: *data_buf++ over ceil(n/4) words. Over-reads the tail. */
static unsigned tx_aligned(struct probe *p, unsigned n, int be, uint32_t *out)
{
	unsigned words = (n + 3) / 4, i;

	for (i = 0; i < words; i++)
		out[i] = load_word(p, 4 * i, be);
	return words;
}

/* unaligned branch, verbatim: data_buf is u32 *, so [1] is +4 bytes. */
static unsigned tx_unaligned(struct probe *p, unsigned n, int be, uint32_t *out)
{
	unsigned words = (n + 3) / 4, i;

	for (i = 0; i < words; i++)
		out[i] =  load_word(p, 4 * (i + 0), be)
		       | (load_word(p, 4 * (i + 1), be) << 8)
		       | (load_word(p, 4 * (i + 2), be) << 16)
		       | (load_word(p, 4 * (i + 3), be) << 24);
	return words;
}

/* the series: memcpy of only the valid bytes, zero-padded */
static unsigned tx_patch(struct probe *p, unsigned n, int be, uint32_t *out)
{
	unsigned words = 0, left = n, off = 0;

	while (left) {
		unsigned k = left > 4 ? 4 : left, i;
		uint8_t staged[4] = { 0, 0, 0, 0 };

		for (i = 0; i < k; i++)
			staged[i] = rd(p, off + i);

		out[words] = 0;
		for (i = 0; i < 4; i++)
			out[words] |= (uint32_t)staged[i]
				      << (be ? 8 * (3 - i) : 8 * i);
		words++;
		off  += k;
		left -= k;
	}
	return words;
}

/* reference: the aligned branch, with the invalid tail bytes zeroed */
static unsigned tx_reference(struct probe *p, unsigned n, int be, uint32_t *out)
{
	unsigned words = (n + 3) / 4, i, j;

	for (i = 0; i < words; i++) {
		uint8_t m[4] = { 0, 0, 0, 0 };

		for (j = 0; j < 4; j++)
			if (4 * i + j < n)
				m[j] = rd(p, 4 * i + j);
		out[i] = 0;
		for (j = 0; j < 4; j++)
			out[i] |= (uint32_t)m[j] << (be ? 8 * (3 - j) : 8 * j);
	}
	return words;
}

/* ---- B: the RX helper ------------------------------------------------ */

/*
 * original: *buf++ = dwc2_readl(...) — a whole u32 store per word, which
 * writes up to 3 bytes past an exactly sized request buffer.
 * patch:    memcpy(out, &word, copy_n) — the same lanes, bounded.
 * shift:    out[j] = word >> (8*j) — the trap form, must fail on BE.
 */
static void rx_store_ref(uint32_t word, int be, uint8_t *dst, unsigned copy_n)
{
	uint8_t rep[4];
	unsigned i;

	for (i = 0; i < 4; i++)
		rep[i] = (uint8_t)(word >> (be ? 8 * (3 - i) : 8 * i));
	for (i = 0; i < copy_n; i++)
		dst[i] = rep[i];		/* what the u32 store put there */
}

static void rx_patch(uint32_t word, int be, uint8_t *dst, unsigned copy_n)
{
	uint8_t rep[4];
	unsigned i;

	for (i = 0; i < 4; i++)
		rep[i] = (uint8_t)(word >> (be ? 8 * (3 - i) : 8 * i));
	memcpy(dst, rep, copy_n);		/* memcpy of the representation */
}

static void rx_shift(uint32_t word, int be, uint8_t *dst, unsigned copy_n)
{
	unsigned i;

	(void)be;
	for (i = 0; i < copy_n; i++)
		dst[i] = (uint8_t)(word >> (8 * i));
}

int main(void)
{
	uint32_t ref[16], alg[16], una[16], pat[16];
	int be, cases = 0;
	int pat_vs_ref_bad = 0, una_vs_ref_bad = 0;
	int rx_bad = 0, rx_shift_bad = 0;
	unsigned n;

	printf("=== DWC2: coverage the frozen TX model does not have ===\n\n");

	printf("A. dwc2_hc_write_packet — bytes touched past the payload\n\n");
	printf("%-4s %-3s %-8s %-10s %-10s %s\n",
	       "host", "n", "aligned", "unaligned", "patch", "note");

	for (be = 0; be <= 1; be++) {
		for (n = 1; n <= 16; n++) {
			struct probe pa, pu, pp, pr;
			unsigned w, i;
			int una_differs = 0;

			probe_init(&pa, n); probe_init(&pu, n);
			probe_init(&pp, n); probe_init(&pr, n);

			w = tx_aligned(&pa, n, be, alg);
			    tx_unaligned(&pu, n, be, una);
			    tx_patch(&pp, n, be, pat);
			    tx_reference(&pr, n, be, ref);

			for (i = 0; i < w; i++) {
				if (pat[i] != ref[i])
					pat_vs_ref_bad++;
				if (una[i] != ref[i])
					una_differs = 1;
			}
			if (una_differs)
				una_vs_ref_bad++;
			cases++;

			if (n == 5 || n == 8)
				printf("%-4s %-3u +%-7u +%-9u +%-9u %s\n",
				       be ? "BE" : "LE", n,
				       pa.max_touched > n ? pa.max_touched - n : 0,
				       pu.max_touched > n ? pu.max_touched - n : 0,
				       pp.max_touched > n ? pp.max_touched - n : 0,
				       una_differs ? "unaligned != reference"
						   : "unaligned == reference");
		}
	}

	printf("\npatch vs aligned reference : %d mismatches  (must be 0)\n",
	       pat_vs_ref_bad);
	printf("unaligned vs reference     : %d of %d cases differ"
	       "  (must be > 0)\n", una_vs_ref_bad, cases);

	printf("\nB. RX helper — representation preservation, unmodelled until now\n\n");
	for (be = 0; be <= 1; be++) {
		uint32_t word = 0x04030201;
		unsigned copy_n;

		for (copy_n = 1; copy_n <= 4; copy_n++) {
			uint8_t r[4] = {0}, p[4] = {0}, s[4] = {0};

			rx_store_ref(word, be, r, copy_n);
			rx_patch(word, be, p, copy_n);
			rx_shift(word, be, s, copy_n);

			if (memcmp(r, p, copy_n))
				rx_bad++;
			if (memcmp(r, s, copy_n))
				rx_shift_bad++;
		}
	}
	printf("memcpy form mismatches     : %d  (must be 0)\n", rx_bad);
	printf("shift  form mismatches     : %d  (must be > 0)\n", rx_shift_bad);

	printf("\nliveness: an instrument that cannot separate the forms is\n"
	       "blind. unaligned must differ, and the RX shift form must fail.\n");

	if (pat_vs_ref_bad == 0 && una_vs_ref_bad > 0 &&
	    rx_bad == 0 && rx_shift_bad > 0) {
		printf("\nverdict: PASS — the patch reproduces the ALIGNED branch and\n"
		       "the RX store, and is deliberately NOT equivalent to the\n"
		       "unaligned branch, which was defective. The frozen TX model\n"
		       "covers none of this.\n");
		return 0;
	}
	printf("\nverdict: FAIL\n");
	return 1;
}
