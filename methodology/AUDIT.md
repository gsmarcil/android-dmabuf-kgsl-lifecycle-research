# Re-audit — DWC2 PIO series, frozen model, and the precedent entry

Second pass over the three input artifacts and everything derived from
them in the previous commit. The question asked was narrow and correct:
**was anything missed, and is anything we concluded wrong?**

Both answers are yes. Five findings, two of them material. The central
one is that the frozen model's reference covers **one of the three call
sites the series changes**, and a claim was written as if it covered all
three — the same `UNSCOPED_CLAIM` failure the skill's own
`scope-precedes-rigor` precedent describes, committed while documenting
its sibling.

---

## Method

Everything below was checked against mainline source fetched in this
session, not against the diff's own description of itself. The GitHub
API is blocked here, so no commit SHA was obtainable; files are pinned
by content hash.

```
drivers/usb/dwc2/gadget.c  sha256 baf17cb89e78c8a63f0a9688af7697875018f162923118f72f1092df703f6d8d
drivers/usb/dwc2/hcd.c     sha256 56a91a8ff69fe8001100524d0df105ec491019fdbec6c13613da3bab7fdc45bb
drivers/usb/dwc2/core.h    sha256 a29c588ab8d4dba36947f5461f621dd49e916385e7b7e12b32e943be5d3eedfe
renesas_usb3.c             sha256 0636d64a95602fe30cd0f8d22621fb825169c6f461657e1a9d6133fe461075d9
```

The frozen TX model was **not modified**. Under the rule this campaign
is documenting, a model may not be extended after the fact to cover what
it missed — that is the disallowed edit. The gaps are covered by a
second, separate instrument, `model/dwc2-coverage-gap-model.c`.

---

## Findings

| # | Severity | Finding |
|---|---|---|
| F1 | high | Kerneldoc claim is false at one of three call sites; model scope is narrower than the patch |
| F2 | high | Removing `DIV_ROUND_UP` silently changes a return value that an IRQ loop branches on |
| F3 | medium | RX `req.actual` accounting changed from reported bytes to stored bytes, unmodelled |
| F4 | medium | No model covered the RX helper at all — same property, no instrument |
| F5 | low | Mainline's unaligned TX branch over-reads up to 15 bytes; the series fixes it silently |

---

### F1 — the claim is broader than the model (high)

The TX helper's kerneldoc, as written in the series:

> The object representation handed to `dwc2_writel()` is that of the
> previous u32 load, on either byte order.

There are three call sites. At two of them — `gadget.c`'s
`dwc2_writel_rep()` and `hcd.c`'s **aligned** branch — this is true. At
the third it is false, because there was no u32 load to preserve.
`dwc2_hc_write_packet()`'s unaligned branch reads:

```c
u32 *data_buf = (u32 *)chan->xfer_buf;
...
u32 data = data_buf[0] | data_buf[1] << 8 |
           data_buf[2] << 16 | data_buf[3] << 24;
```

`data_buf` is `u32 *`. `data_buf[1]` is **+4 bytes**, not +1. The branch
reads four whole words, shifts them as if they were bytes, ORs them
together, and advances by one word. It is neither a representation load
nor a byte assembly — it is simply broken, and has been in mainline for
years.

So the frozen model's reference, `*(u32 *)src`, is the **aligned branch
only**. The series replaces both branches with one helper, which means:

- against the aligned branch the change is equivalence-preserving, which
  the frozen model proves;
- against the unaligned branch the change is **deliberately not
  equivalent** — it is a fix — and no model says so.

Measured by the second instrument, across 32 cases:

```
patch vs aligned reference : 0 mismatches
unaligned vs reference     : 32 of 32 cases differ
```

**What is wrong in what we concluded.** The previous commit stated the
preservation target as "the original loaded an object representation and
passed it through" without qualification, and the precedent entry
repeated it. That sentence is right about the target and wrong about its
extent. It must read: the preservation target is the aligned load; the
unaligned branch is replaced rather than preserved, on purpose.

Fixed in `precedents/frozen-model-binding.md` under **Model scope**.

---

### F2 — a control-flow change rode along unnoticed (high)

This is the one nothing in the campaign looked at.

`dwc2_hsotg_write_fifo()` ends:

```c
to_write = DIV_ROUND_UP(to_write, 4);        /* removed by the series */
data = hs_req->req.buf + buf_pos;
dwc2_writel_rep(hsotg, EPFIFO(hs_ep->index), data, to_write);

return (to_write >= can_write) ? -ENOSPC : 0;
```

`can_write` is in **bytes** on all three paths that set it — the two
register paths multiply by 4, and `hs_ep->fifo_size` is itself stored as
`(val >> FIFOSIZE_DEPTH_SHIFT) * 4`. So mainline compares a **word**
count against a **byte** count. Since `to_write` has already been clamped
to `can_write`, `ceil(b/4) >= can_write` is unreachable for any realistic
FIFO size: the line effectively always returns 0.

The series removes the `DIV_ROUND_UP` because the helper now takes
bytes. Correct for the helper — but the same variable feeds the return,
which now compares bytes against bytes and fires whenever the write
consumed the available space.

The return is not discarded:

```
dwc2_hsotg_write_fifo()  -> dwc2_hsotg_trytx()  -> dwc2_hsotg_irq_fifoempty()
                                                     if (ret < 0) break;
```

`dwc2_hsotg_irq_fifoempty()` loops over every IN endpoint on a TX-FIFO-
empty interrupt. Before: the loop effectively never broke. After: it
breaks as soon as one endpoint fills the FIFO, skipping the remaining
endpoints in that pass.

The new behaviour is almost certainly what the code always meant, and
the interrupt will fire again, so this is likely benign or an
improvement. But it is a real, observable change in multi-IN-endpoint
configurations, shipped inside a patch justified purely as a
memory-safety fix, with no model, no kerneldoc mention, and no commit
message mention.

**Recommendation:** either restore the original comparison explicitly
(`DIV_ROUND_UP(to_write, 4) >= can_write`) to keep the series
behaviour-neutral, or split the unit fix into its own patch with its own
justification. Silently correcting a second bug inside a memory-safety
patch is what makes a series hard to review and hard to backport.

---

### F3 — RX accounting semantics changed (medium)

```c
-	hs_req->req.actual += to_read;     /* bytes the core reported */
+	hs_req->req.actual += to_copy;     /* bytes actually stored */
```

`req.actual` is part of the gadget API contract and is read by every
function driver. Mainline advanced it by the full reported count even
when it wrote past the buffer end; the series advances it by what was
delivered.

The new value is the defensible one — reporting bytes that were never
stored is what made the original overflow invisible. But it is a
behaviour change at an API boundary, it only triggers on the
`WARN_ON_ONCE` path, and it is unmodelled. It deserves a sentence in the
commit message.

Note the guard is sound: `max_req = req.length - read_ptr` cannot be
negative in normal operation, and `max_req > 0 ? min(to_read, max_req) : 0`
handles the degenerate case.

---

### F4 — the RX helper had no model (medium)

The frozen model covers TX. The RX helper carries the **same**
representation property in the opposite direction, and the same trap was
available: `out[i] = word >> (8 * i)` would have looked equally natural
and broken equally on big-endian. Nothing would have caught it.

Now covered by the second instrument:

```
memcpy form mismatches : 0
shift  form mismatches : 4     (all BE)
```

The helper is also robust to its own kerneldoc being violated: if a
caller passed `copy_bytes > fifo_bytes`, `copy_n` is still bounded by
`fifo_n` each iteration, so it under-copies rather than overflowing.

---

### F5 — the unaligned branch over-reads far more than the aligned one (low)

Quantified by the second instrument, bytes touched past the payload:

```
payload   aligned   unaligned   patch
5 bytes   +3        +15         +0
8 bytes   +0        +12         +0
```

The unaligned branch over-reads even when the payload is an exact
multiple of four, because the last iteration still reads three words
beyond its own. The series removes it.

Scope note on impact, stated carefully: this is an out-of-bounds
**read**, which can fault at a page boundary and is KASAN-reportable.
The excess bytes land in the TxFIFO, but the core transmits only the
programmed packet length, so this is not a wire disclosure under normal
programming. Do not upgrade it to one without an artifact.

---

## What held up

- The frozen TX model rebuilds and reruns clean: 32 cases, 0 memcpy
  mismatches, 16 shift mismatches, all big-endian. Its liveness control
  works — it refuses to print PASS if the rejected form stops failing.
- The `renesas_usb3` citation is real and the sink analysis is right:
  `usb3_write()` is `iowrite32()` (value), while the same function's bulk
  path is `iowrite32_rep()` (representation). The two agree only on
  little-endian, which is why the shape is safe there and not here.
- Word counts on the bus are unchanged. TX of 5 bytes writes 2 words
  before and after; RX of 5 bytes pops 2 words before and after. The
  FIFO stays in sync.
- Zero-byte cases are equivalent: `while (bytes)` matches mainline's
  `if (count)` guard, and `byte_count == 0` writes no words either way.
- `req.actual` and `fifo_load` on the TX side are assigned before the
  removed `DIV_ROUND_UP` and are unaffected. F2 is the *only* collateral
  effect of removing it.
- The `int` → `unsigned int` conversions at the helper boundary are
  bounded by GRXSTSP `BCNT` (11 bits) and `max_packet`. Same exposure as
  the code being replaced; not a regression.

## Correctness vs equivalence

Worth stating once, because the model invites the confusion: the model
proves the patch does **what the original did**, not that the original
was right. On big-endian with `needs_byte_swap`, whatever lane order
mainline produced is faithfully reproduced. If that order was already
wrong, the series is faithfully wrong. Establishing correctness needs a
device, and that gate has not been run.

## Artifacts

```
model/dwc2-tx-equivalence-model.c    frozen, unmodified
model/dwc2-coverage-gap-model.c      new — unaligned branch + RX helper
results/tx-equivalence-run.txt
results/coverage-gap-run.txt
results/validator-liveness.txt
```
