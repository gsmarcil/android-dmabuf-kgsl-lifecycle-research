// SPDX-License-Identifier: GPL-2.0
/*
 * dwc2-h3-window-model.c — H-H3a and H-H3b are different defects
 *
 * Both live in dwc2_hc_write_packet(). Calling both "tail over-read"
 * conflates them, and the conflation hides the fact that one of them
 * does not need a tail at all.
 *
 *   H-H3a  aligned branch, whole-word tail over-read
 *          SOURCE_SPAN = 4 * ceil(B / 4)
 *          EXCESS      = SPAN - B  in {0,1,2,3}
 *          requires B % 4 != 0 — vanishes on exact multiples
 *
 *   H-H3b  unaligned branch, wrong-width indexing
 *          data_buf is u32 *, so data_buf[1..3] are +4, +8, +12 BYTES.
 *          Each assembly step touches a 16-byte window while advancing
 *          4, so the last step reaches 4*(K-1) + 15 for K = ceil(B/4).
 *          SOURCE_SPAN = 4 * ceil(B / 4) + 12
 *          EXCESS      = SPAN - B  in {12,13,14,15}
 *          NEVER zero — independent of any partial tail
 *
 * The spans are measured by index tracking, never read out of bounds,
 * then checked against the closed forms above.
 *
 * Build: cc -O1 -g -Wall -Wextra -fsanitize=address,undefined \
 *           -o h3-window dwc2-h3-window-model.c
 */

#include <stdio.h>

#define MAXB 64

struct span {
	unsigned high;		/* highest byte index touched, +1 */
};

static void touch(struct span *s, unsigned byte_off, unsigned width)
{
	if (byte_off + width > s->high)
		s->high = byte_off + width;
}

/* aligned branch: ceil(B/4) whole-word loads at 4*i */
static unsigned span_aligned(unsigned b)
{
	struct span s = { 0 };
	unsigned words = (b + 3) / 4, i;

	for (i = 0; i < words; i++)
		touch(&s, 4 * i, 4);
	return s.high;
}

/* unaligned branch verbatim: data_buf[0..3] with data_buf a u32 * */
static unsigned span_unaligned(unsigned b)
{
	struct span s = { 0 };
	unsigned words = (b + 3) / 4, i, k;

	for (i = 0; i < words; i++)
		for (k = 0; k < 4; k++)
			touch(&s, 4 * (i + k), 4);
	return s.high;
}

/* the series helper: exactly B bytes, nothing beyond */
static unsigned span_patch(unsigned b)
{
	struct span s = { 0 };
	unsigned left = b, off = 0;

	while (left) {
		unsigned n = left > 4 ? 4 : left;

		touch(&s, off, n);
		off  += n;
		left -= n;
	}
	return s.high;
}

int main(void)
{
	unsigned b, r;
	int form_bad = 0, h3b_zero_excess = 0, laws_identical = 0;
	int patch_excess_bad = 0;
	unsigned a_by_mod[4] = { 0 }, u_by_mod[4] = { 0 };
	int seen[4] = { 0 };

	printf("=== dwc2_hc_write_packet — H-H3a vs H-H3b source windows ===\n\n");

	for (b = 1; b <= MAXB; b++) {
		unsigned k = (b + 3) / 4;
		unsigned sa = span_aligned(b);
		unsigned su = span_unaligned(b);
		unsigned sp = span_patch(b);
		unsigned ea = sa - b, eu = su - b;

		/* closed forms */
		if (sa != 4 * k || su != 4 * k + 12 || sp != b)
			form_bad++;
		if (eu == 0)
			h3b_zero_excess++;
		if (ea == eu)
			laws_identical++;
		if (sp != b)
			patch_excess_bad++;

		r = b % 4;
		if (!seen[r]) {
			a_by_mod[r] = ea;
			u_by_mod[r] = eu;
			seen[r] = 1;
		} else {
			if (a_by_mod[r] != ea || u_by_mod[r] != eu)
				form_bad++;
		}
	}

	printf("EXCESS bytes beyond byte_count, by B %% 4:\n\n");
	printf("%-8s %-12s %-12s %s\n", "B % 4", "H-H3a", "H-H3b", "patch");
	for (r = 0; r < 4; r++)
		printf("%-8u +%-11u +%-11u +%u\n", r, a_by_mod[r], u_by_mod[r], 0u);

	printf("\nclosed-form deviations       : %d   (must be 0)\n", form_bad);
	printf("H-H3b cases with zero excess : %d   (must be 0 — the defect\n"
	       "                                     does not need a tail)\n",
	       h3b_zero_excess);
	printf("B where the two laws agree   : %d   (must be 0, else the\n"
	       "                                     instrument conflates them)\n",
	       laws_identical);
	printf("patch touching past B        : %d   (must be 0)\n",
	       patch_excess_bad);

	printf("\nSOURCE_SPAN(H-H3a) = 4*ceil(B/4)\n");
	printf("SOURCE_SPAN(H-H3b) = 4*ceil(B/4) + 12\n");

	if (form_bad == 0 && h3b_zero_excess == 0 && laws_identical == 0 &&
	    patch_excess_bad == 0) {
		printf("\nverdict: PASS — two distinct defects. H-H3a is a tail\n"
		       "over-read conditional on B %% 4 != 0; H-H3b is a\n"
		       "wide-source-window defect present at every B.\n");
		return 0;
	}
	printf("\nverdict: FAIL\n");
	return 1;
}
