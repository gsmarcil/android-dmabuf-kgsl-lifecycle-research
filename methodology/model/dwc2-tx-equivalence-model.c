// SPDX-License-Identifier: GPL-2.0
/*
 * dwc2-tx-equivalence-model.c — S8 patch/model equivalence for the TX helper
 *
 * Verifies that the helper as it now stands in the series reproduces the
 * object representation the original code handed to dwc2_writel(), on both
 * host byte orders, and that the previously proposed shift form does not.
 *
 *   original      word = *(u32 *)src            (whole word, may over-read)
 *   memcpy form   word = 0; memcpy(&word, src, n)
 *   shift form    word = 0; word |= src[i] << (8 * i)
 *
 * dwc2_writel() applies swab32() to the VALUE when needs_byte_swap is set;
 * that conversion is identical for identical values, so it cannot rescue a
 * form whose value already differs. The reference is the original word
 * restricted to the n valid bytes: whatever lane src[i] occupied under
 * *(u32 *)src, it must still occupy.
 *
 * Both host orders are modelled explicitly, so no big-endian machine is
 * needed.
 *
 * Build: cc -O1 -g -Wall -Wextra -fsanitize=address,undefined \
 *           -o tx-equiv dwc2-tx-equivalence-model.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* what *(u32 *)p yields on a host of the given byte order */
static uint32_t host_load(const uint8_t *p, int be)
{
	uint32_t w = 0;
	int i;

	for (i = 0; i < 4; i++)
		w |= (uint32_t)p[i] << (be ? 8 * (3 - i) : 8 * i);
	return w;
}

/* reference: the original whole-word load, masked to the n valid bytes */
static uint32_t reference(const uint8_t *src, int n, int be)
{
	uint8_t m[4] = { 0, 0, 0, 0 };
	int i;

	for (i = 0; i < n && i < 4; i++)
		m[i] = src[i];
	return host_load(m, be);
}

/* the form now in the series: memcpy for every n, tail included */
static uint32_t memcpy_form(const uint8_t *src, int n, int be)
{
	uint8_t staged[4] = { 0, 0, 0, 0 };
	size_t k = (size_t)n > 4u ? 4u : (size_t)n;

	memcpy(staged, src, k);
	return host_load(staged, be);
}

/* the form that was briefly in the series and is rejected here */
static uint32_t shift_form(const uint8_t *src, int n, int be)
{
	uint32_t w = 0;
	int i;

	(void)be;
	for (i = 0; i < n; i++)
		w |= (uint32_t)src[i] << (8 * i);
	return w;
}

int main(void)
{
	static const uint8_t pat[8] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
	};
	int be, n, off, mc_bad = 0, sh_bad = 0, cases = 0;

	printf("=== DWC2 TX helper: patch/model equivalence ===\n");
	printf("reference = original *(u32 *)src, masked to n valid bytes\n\n");
	printf("%-4s %-3s %-4s %-12s %-12s %-12s %-8s %s\n",
	       "host", "n", "off", "reference", "memcpy", "shift",
	       "memcpy", "shift");

	for (be = 0; be <= 1; be++) {
		for (off = 0; off <= 3; off++) {
			for (n = 1; n <= 4; n++) {
				const uint8_t *s = pat + off;
				uint32_t r = reference(s, n, be);
				uint32_t m = memcpy_form(s, n, be);
				uint32_t h = shift_form(s, n, be);

				if (m != r) mc_bad++;
				if (h != r) sh_bad++;
				cases++;

				if (off == 0)
					printf("%-4s %-3d %-4d 0x%08x   0x%08x   0x%08x   %-8s %s\n",
					       be ? "BE" : "LE", n, off, r, m, h,
					       m == r ? "ok" : "MISMATCH",
					       h == r ? "ok" : "MISMATCH");
			}
		}
	}

	printf("\ncases swept                : %d (host order x offset x n)\n", cases);
	printf("memcpy form mismatches     : %d\n", mc_bad);
	printf("shift  form mismatches     : %d\n", sh_bad);

	printf("\nliveness: the shift form must fail, or this model cannot\n"
	       "discriminate. shift mismatches > 0 -> %s\n",
	       sh_bad > 0 ? "discriminating" : "MODEL IS BLIND");

	if (mc_bad == 0 && sh_bad > 0) {
		printf("\nverdict: PASS — the series form preserves representation on\n"
		       "both byte orders; the rejected form does not.\n");
		return 0;
	}
	printf("\nverdict: FAIL\n");
	return 1;
}
