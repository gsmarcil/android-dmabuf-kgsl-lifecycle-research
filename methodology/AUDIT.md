# Re-audit — DWC2 PIO series, frozen model, and the precedent entry

Second and third pass over the three input artifacts and everything
derived from them. The question was narrow and correct: **was anything
missed, and is anything we concluded wrong?**

Both answers are yes, and the third pass moved the interpretation again.
The problem was never only in the implementation. It was in how the
equivalence relation itself was defined.

## What changed in round 3

```
F1  reframed   not "the model's scope was too narrow" but "S8 had the
               wrong domain". One function needed two relations
F2  CORRECTED  I wrote that nothing in the campaign had looked at this.
               Wrong. D7 modelled it already. The failure is
               attribution at patch integration, which is worse
F3  narrowed   dropped the word "defensive"; three invariants stated
F4  promoted   RX frozen as its own model, not a note derived from TX
F5  split      H-H3a and H-H3b are different defects with different laws
new            patch review matrix, commit split, artifact integrity
```

---

## Method

Everything is checked against mainline fetched in this session, not
against the diff's description of itself. The GitHub API is blocked
here, so files are pinned by content hash.

```
drivers/usb/dwc2/gadget.c  sha256 baf17cb89e78c8a63f0a9688af7697875018f162923118f72f1092df703f6d8d
drivers/usb/dwc2/hcd.c     sha256 56a91a8ff69fe8001100524d0df105ec491019fdbec6c13613da3bab7fdc45bb
drivers/usb/dwc2/core.h    sha256 a29c588ab8d4dba36947f5461f621dd49e916385e7b7e12b32e943be5d3eedfe
renesas_usb3.c             sha256 0636d64a95602fe30cd0f8d22621fb825169c6f461657e1a9d6133fe461075d9
```

The frozen TX model was **not modified** in any round. Gaps are covered
by additional instruments at their own declared domains.

---

## F1 — S8 had the wrong domain

`dwc2_hc_write_packet()`'s unaligned branch is not an unaligned variant
of `*data_buf`. It is a different algorithm:

```c
u32 *data_buf = (u32 *)chan->xfer_buf;

data_buf[0] | data_buf[1] << 8 | data_buf[2] << 16 | data_buf[3] << 24
```

`data_buf[1]` is at +4, then +8 and +12. The code treats a `u32 *` as if
it were a `u8 *`, producing value corruption and a far wider read window
than the operation needs — which is what H-H3 recorded.

So `PATCH must preserve the old u32-load representation` cannot be
applied to the function as a whole. The correct form is a partition:

```
dwc2_hc_write_packet()

ALIGNED path
    preservation relation
    PATCH ≡ OLD_ALIGNED                on valid source-byte representation

UNALIGNED path
    correction relation
    PATCH ≠ OLD_UNALIGNED              REQUIRED
    PATCH ≡ BYTE-CONTRACT ORACLE       REQUIRED
```

Measured:

```
patch vs aligned reference : 0 mismatches
unaligned vs reference     : 32 of 32 cases differ
```

The 32/32 is **not** a problem with the patch. Under a CORRECT relation
it is the pass condition, so long as an independent oracle establishes
the new form is byte-correct — which the aligned reference supplies.

The rule was therefore rewritten as **frozen-model domain binding**: a
frozen model binds only over its declared domain, reference path and
property, and every changed path must declare `PRESERVE`, `CORRECT`, or
`INTENTIONALLY_DIVERGE`. Both directions can fail — an implementation
that does **not** differ under an intended CORRECT relation fails too.

That is the real lesson: **equivalence is not always the target.**

Recorded as a legitimate discharge of the previous rule, not the
forbidden edit: no implementation failed the old rule and was rescued by
rewriting it. The old rule's verdicts all stand; it was too narrow in
domain, not too strict in judgment.

---

## F2 — D7 re-entered through the patch, unattributed

**Correction to the previous version of this report.** I wrote that this
was "the one nothing in the campaign looked at". That is false, and the
truth is worse.

D7 had already modelled exactly this: `to_write` converting from bytes to
DWORDs and then being compared against `can_write` measured in bytes,
producing a false negative for `-ENOSPC`. The caller consequence was
modelled too — `ret < 0 -> break`, and the difference in whether the next
endpoint is scanned.

So the finding is not the discovery of D7. It is:

```
D7 existed and was modelled.
PATCH REVIEW failed to recognize that removing DIV_ROUND_UP
also silently incorporated the D7 fix.
```

Methodologically that is worse than a missed finding. A **known**
semantic change passed through patch integration with no attribution.
The knowledge existed in the campaign and did not reach the review of the
patch that shipped it.

Confirmed unchanged in mainline: `can_write` is in bytes on all three
paths that set it — the two register paths multiply by 4, and
`hs_ep->fifo_size` is stored as `(val >> FIFOSIZE_DEPTH_SHIFT) * 4`. The
return is live:

```
dwc2_hsotg_write_fifo() -> dwc2_hsotg_trytx() -> dwc2_hsotg_irq_fifoempty()
                                                   if (ret < 0) break;
```

### Recommended split

Restoring the unit bug on purpose inside a memory-safety patch, only to
preserve old behaviour, is the wrong repair. The clean shape is three
commits:

```
commit A — fix TX FIFO return-unit comparison
           bytes vs bytes
           model: D7.1 / D7.2 / D7.3

commit B — replace word-oriented TX accesses with byte-safe helper
           memory footprint / alignment / representation

commit C — RX byte-safe conversion
           destination bounds / alignment / accounting
```

A reviewer can then see that the `irq_fifoempty()` change is intended and
modelled, rather than an unmentioned side effect of commit B.

---

## F3 — `req.actual` needs its own gate

```c
-	hs_req->req.actual += to_read;
+	hs_req->req.actual += to_copy;
```

In the normal case this is unobservable:

```
to_read <= remaining
to_copy == to_read
=> observationally identical
```

The change only bites in the over-capacity case:

```
controller-reported bytes = to_read
stored/accepted bytes     = to_copy
req.actual                = to_copy
FIFO drained              = ceil(to_read / 4) words
```

This redefines `req.actual` from *reported* to *accepted/stored* for that
class. Source alone does not license calling the new value "API-correct".
What can be claimed is narrower and stronger:

```
actual <= length invariant     restored
actual == bytes stored         restored
reported == actual             intentionally no longer true on overflow
```

That third line is a deliberate contract change and belongs in the commit
message and in a model — not under the word "defensive", which is what
the previous version of this report leaned on.

---

## F4 — RX is frozen as its own model

The TX model grants RX nothing; deriving an RX claim from a TX result
would repeat F1 in the opposite direction. RX's reference is its own:

```c
u32 word = dwc2_readl(...);
*data_buf = word;          /* register value becomes an object
                              representation in memory */
```

So `memcpy(dst, &word, n)` is the construction that preserves the valid
representation bytes, while `(word >> (8 * i)) & 0xff` imposes a numeric
little-endian interpretation.

`model/dwc2-rx-relation-model.c` declares both relations and freezes
them:

```
R1 PRESERVE(representation)    40 cases
   memcpy mismatches : 0
   shift  mismatches : 12          discriminator alive

R2 CORRECT(destination bound)  2145 cases
   candidate == copy_bytes everywhere
   reference over-writes in 2128 cases
   an "unfixed" control that still matches the reference is REJECTED
```

That last line is what makes the CORRECT direction real rather than
decorative: the instrument can fail a candidate for *complying* with the
old behaviour.

---

## F5 — H-H3a and H-H3b are different defects

Let `B = byte_count` and `K = ceil(B / 4)`. The unaligned branch runs `K`
iterations; the last starts at `4*(K-1)` and reaches `data_buf[3]`, whose
last byte is at `4*(K-1) + 15`. So:

```
SOURCE_SPAN(H-H3b) = 4*ceil(B/4) + 12
EXCESS             = 4*ceil(B/4) + 12 - B
```

Verified by simulation for B = 1..64 against the closed form, zero
deviations:

```
B % 4      H-H3a        H-H3b       patch
0          +0           +12         +0
1          +3           +15         +0
2          +2           +14         +0
3          +1           +13         +0
```

H-H3b is therefore **not a tail over-read at all**. It is a
wide-source-window defect, present at every `B`, including exact
multiples of four where H-H3a vanishes entirely. Each logical FIFO word
needs 4 source bytes; the code touches a 16-byte window per assembly
step.

```
H-H3a  aligned whole-word tail over-read
       excess = 1..3
       requires B % 4 != 0

H-H3b  unaligned wrong-width indexing / wide-window over-read
       excess = 12..15 beyond logical byte_count
       does NOT require a partial tail
```

The instrument checks that the two laws never coincide, so it cannot
conflate the defects it is meant to separate.

Impact, stated carefully: an out-of-bounds **read**, faultable at a page
boundary and KASAN-reportable. The excess bytes land in the TxFIFO, but
the core transmits only the programmed packet length, so this is not a
wire disclosure under normal programming. Do not upgrade it without an
artifact.

---

## Patch review matrix

Every changed expression, on every axis it can move:

| changed expression | memory | value repr | control flow | accounting | bus txns |
|---|---|---|---|---|---|
| gadget.c `writel_rep` → byte helper | changed, modelled | preserved | **changed (D7)** | preserved | preserved |
| hcd.c aligned branch → helper | changed (H-H3a) | preserved | preserved | preserved | preserved |
| hcd.c unaligned branch → helper | changed (H-H3b) | **changed, required** | preserved | preserved | preserved |
| `dwc2_read_packet` → helper | changed | preserved | preserved | n/a | preserved |
| rx `actual += to_read` → `to_copy` | changed | preserved | preserved | **changed (F3)** | preserved |

Three changed cells lack a same-patch justification: D7's control flow,
the unaligned branch's required value change, and the RX accounting
change. The first two are handled by the commit split; the third needs a
commit-message sentence and a model.

Had this matrix been filled when the series was written, F2 would have
surfaced immediately: removing `DIV_ROUND_UP` reads as `MEMORY EFFECT:
intended`, but also `CONTROL FLOW: changed`, and a memory-safety
justification does not cover the second.

---

## What held up — now serving as controls

- Bus transaction count is `ceil(bytes / 4)` before and after, on both
  directions. The FIFO stays in sync.
- Zero-length invariant: `while (bytes)` matches mainline's `if (count)`;
  `byte_count == 0` writes no words either way.
- TX accounting (`req.actual`, `fifo_load`) is assigned **before** the
  removed conversion, so D7 is genuinely the only semantic side effect of
  that deletion in `dwc2_hsotg_write_fifo()`.
- The frozen TX model rebuilds and reruns clean: 32 cases, 0 memcpy
  mismatches, 16 shift mismatches, all big-endian, liveness control
  intact.
- The `renesas_usb3` sink analysis holds: `usb3_write()` is
  `iowrite32()` (value) while the same function's bulk path is
  `iowrite32_rep()` (representation). They agree only on little-endian.
- `int` → `unsigned int` conversions at the helper boundary are bounded
  by GRXSTSP `BCNT` (11 bits) and `max_packet`. Not a regression.

## Correctness vs equivalence

The models prove the patch does what the reference did **on the paths
where PRESERVE was declared**. They do not establish that the reference
was right. On big-endian with `needs_byte_swap`, whatever lane order
mainline produced is faithfully reproduced; if that order was already
wrong, the series is faithfully wrong. Establishing correctness needs a
device, and that gate has not been run.

---

## Artifact integrity

The stale-artifact concern is resolved in the good direction for this
session's copy, and only for this session's copy.

```
uploaded to this session   d02c8479-dwc2pioseries.diff
sha256                     648a33d603fae1930d654d542e51dedb0e6b460bf8add2fd73c60e8a9bbc3e9a
196 lines, real newlines
shift form  `<< (8 * i)`   0 occurrences
memcpy form                2 occurrences (RX line 46, TX line 83)
literal \n                 2, both inside dev_vdbg format strings on
                           unchanged context lines — source, not corruption
```

So the artifact analysed in rounds 2 and 3 **is** `648a33…`, and it is
locally verified here. The stale `47bbec…` copy carrying the shift form
still needs replacing on the other side; nothing in this report depends
on it.

The other two uploads:

```
dwc2-tx-equivalence-model.c  sha256 87eae979156e626a75567972c9b3cf9b5b50b7de1d0d4841339b0d2a9a0c5d86
frozen-model-binding.md      sha256 50e1102ee25626a6c31a79fda37a8b93d43681aa5c095a750e4c42b931df2711
```

## Artifacts

```
model/dwc2-tx-equivalence-model.c    frozen, unmodified across all rounds
model/dwc2-coverage-gap-model.c      unaligned branch + RX first pass
model/dwc2-rx-relation-model.c       RX frozen at its own domain, R1 + R2
model/dwc2-h3-window-model.c         H-H3a / H-H3b closed forms
results/tx-equivalence-run.txt
results/coverage-gap-run.txt
results/rx-relation-run.txt
results/h3-window-run.txt
results/validator-liveness.txt
```
