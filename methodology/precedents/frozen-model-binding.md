# Frozen-model binding, and prior-art non-authority

Two rules, one entry. They are recorded together because they failed
together, on one fixture, in one round: an external driver was promoted
from hypothesis source to oracle, and the frozen model that contradicted
it was nearly rewritten until it agreed. Either failure alone is
recoverable. Together they produce a patch that is green on every
conventional signal and wrong on half the world's byte orders.

Read both halves. Splitting them across two entries loses the only thing
the fixture teaches, which is that the second failure is what disarms the
defence against the first.

## Evidence layers in this entry

```
RE-VERIFIED HERE   the discriminating model, rebuilt and rerun
                   the renesas_usb3 construction, fetched from source
RECORDED ONLY      build / checkpatch / apply results from the campaign,
                   not reproduced in this write (no kernel tree present)
```

---

# 1 — Frozen-model binding

## Rule

```
Once a discriminating model has admitted an implementation property, a
later patch MUST remain inside that model's admitted equivalence class.

BUILD success, checkpatch success, prior-art similarity, and
implementation convenience cannot override a model mismatch.

If implementation and frozen model disagree:
    the implementation loses by default.
```

## Discharge condition

```
The model may be superseded ONLY by new discriminating evidence showing
that the modeled property itself was wrong or incomplete.

It MUST NOT be rewritten so that a new implementation passes.

DISCHARGED     new evidence falsifies the modeled property, and the
               replacement model is itself discriminating
NOT DISCHARGED the model was edited after seeing the implementation fail
```

The second clause is the load-bearing one. Without it the gate is
falsifiable from inside the methodology: a well-shaped patch fails the
model, the model is "corrected" until it passes, and the check has
verified nothing. The edit is always locally reasonable — that is the
danger. Nobody rewrites a model believing they are cheating.

## Audit question

```
fires: whenever an implementation disagrees with a frozen model.

  Am I about to change the implementation, or the model?
  If the model — what NEW evidence, obtained independently of this
  implementation, shows the modeled property was wrong?
```

"Independently of this implementation" is the whole question. Evidence
that consists of the new patch failing is not evidence about the
property; it is evidence about the patch.

## Campaign application

The DWC2 PIO series reached a state where every conventional signal was
green (recorded, not re-verified here):

```
exact-tree build, W=1        0 diagnostics
apply / reverse apply        OK
checkpatch --strict          0 errors, 0 warnings, 0 checks
```

and the TX helper still contained a semantic regression. Its tail built
the final FIFO word by value:

```c
for (i = 0; i < n; i++)
        word |= (u32)in[i] << (8 * i);
```

A previously frozen discriminating model had already admitted the memcpy
form and rejected exactly this shift form on big-endian hosts. Rerun
against the shipped shape, it still rejected it. The model was not
adjusted; the helper was, and the series now stages the tail with
`memcpy(&word, in, n)`.

The compiler cannot see this. checkpatch cannot see this. Only the
frozen model could, and only because it was not permitted to move.

## Reusable pattern

```
Three authorities, never interchangeable:

  compiler / checkpatch       is the change structurally valid?
  frozen discriminating model does it preserve the decided property?
  external prior art          does it suggest a shape worth testing?

Green from the first says nothing about the second question.
```

Generalises past patches: any time a fixed oracle — a golden file, a
recorded protocol trace, a reference implementation, a property test with
a pinned seed — disagrees with new work, the oracle is the artifact under
protection, not the obstacle to route around.

---

# 2 — Prior-art non-authority

## Rule

```
A sibling or external implementation is evidence for a CANDIDATE design,
not evidence of equivalence.

PRIOR ART generates hypotheses.
REFERENCE IMPLEMENTATION defines the preservation target.
DISCRIMINATING MODEL judges candidate implementations.

Prior art occupies the first role only. Promoting it to the third is a
category error.
```

## Discharge condition

```
Before importing a construction from another implementation, identify
what ITS sink consumes:

    object representation
    numeric value
    byte stream
    device-native lane order
    encoded protocol value

DISCHARGED     the sinks consume the same thing, shown from both sources
NOT DISCHARGED the operations merely look alike
```

Same-looking operations across different sink semantics are not
substitutes. The construction is not the unit of reuse; the
construction-plus-sink is.

## Audit question

```
fires: when an external implementation is used to justify a construction.

  What does the other driver's sink consume, and what does mine consume?
  If I cannot answer both from source, this is a hypothesis, not a model.
```

## Campaign application

`renesas_usb3` builds its final partial FIFO word by value. Verified
from source for this entry:

```c
/* drivers/usb/gadget/udc/renesas_usb3.c, usb3_write_pipe() */
if (len >= 4) {
        iowrite32_rep(usb3->reg + fifo_reg, buf, len / 4);
        buf += (len / 4) * 4;
        len %= 4;
}

if (len) {
        for (i = 0; i < len; i++)
                tmp |= buf[i] << (8 * i);
        usb3_write(usb3, tmp, fifo_reg);   /* -> iowrite32() */
}
```

This was cited as in-tree precedent for the DWC2 TX tail, and the
construction was imported. The sinks differ:

```
renesas_usb3   usb3_write() is iowrite32(): the register consumes a
               VALUE, assembled by the driver and converted on the way out
dwc2 original  loaded an object REPRESENTATION from memory and passed it
               through dwc2_writel(); the preservation target is that
               representation
```

That second line is true of the aligned load, which is what the model
references. It is not true of every site the series touches — see
**Model scope** below, where writing it without that qualification is
recorded as a defect of this entry.

Note what the citation had to step over to be usable at all. Inside the
same function, renesas_usb3 pushes its whole words with `iowrite32_rep`,
which forwards the memory representation, and its tail with a
driver-assembled value. Two different disciplines, two sentences apart.
Reading the tail alone and carrying it away discards the half of the
function that shows the discipline is not uniform. On the little-endian
SoCs this driver ships on the two agree, so nothing there ever forces the
question.

The reference for a DWC2 patch is what DWC2 previously did, not what a
different driver does with a different sink. The imported shape diverged
on big-endian hosts and was caught only by the frozen model — which is
the join between the two halves of this entry: prior art supplied the
wrong shape, and the authority it borrowed was then aimed at the one
instrument that could detect the substitution.

## Positive control

Klipper's `fifo_read_packet` was cited earlier in the same campaign and
did NOT mislead: it was used to raise the prior that the tail must be
handled at all, never to define what the resulting bytes should be.

Prior art used as a hypothesis source is sound. The failure is specific
to using it as an oracle, and the control shows the rule is not "ignore
other implementations" — that would cost the campaign a real finding.

---

# Fixture

## Source pins

```
patch under test   DWC2 PIO series, dwc2_write_fifo_bytes() TX helper
                   drivers/usb/dwc2/core.h
prior art          drivers/usb/gadget/udc/renesas_usb3.c
                   usb3_write_pipe(), lines 1148-1184 of the fetched blob
                   ref:     master, fetched 2026-08-14
                   sha256:  0636d64a95602fe30cd0f8d22621fb825169c6f461657e1a9d6133fe461075d9
                   note:    commit-SHA pin unavailable — the GitHub API is
                            blocked in this environment. The content hash is
                            the pin; re-fetch and compare before reuse.
```

## Discriminating model — rerun for this entry

`dwc2-tx-equivalence-model.c`, built exactly as its header specifies:

```
cc -O1 -g -Wall -Wextra -fsanitize=address,undefined \
   -o tx-equiv dwc2-tx-equivalence-model.c
```

Reference is the original whole-word load `*(u32 *)src`, masked to the
`n` valid bytes: whatever lane `src[i]` occupied, it must still occupy.
Both host orders are modelled explicitly, so no big-endian machine is
required.

```
cases swept                : 32 (host order x offset x n)
memcpy form mismatches     : 0
shift  form mismatches     : 16     all on BE
verdict                    : PASS
```

The model carries its own liveness control: it refuses to report PASS
unless the rejected shift form actually fails, so a run that reported
zero mismatches everywhere would be declared `MODEL IS BLIND` rather than
clean. That is what makes the 0 in the memcpy column mean anything —
see `null-result-requires-liveness-control`.

## Model scope — added on re-audit

The model's reference is `*(u32 *)src`. That is one of the three sites
the series changes, and the first version of this entry stated the
preservation target as though it were all of them.

```
COVERED        gadget.c dwc2_writel_rep, and hcd.c's ALIGNED branch:
               the change is equivalence-preserving, and the model
               proves it

NOT COVERED    hcd.c's UNALIGNED branch. data_buf is u32 *, so
               data_buf[1] is +4 bytes: mainline reads four whole words,
               shifts them as if they were bytes, and advances by one.
               There is no u32 load there to preserve. The series
               replaces it, deliberately NOT equivalently — a fix, not a
               preservation

NOT COVERED    the RX helper, which carries the same property in the
               opposite direction and had no instrument at all

NOT COVERED    removing DIV_ROUND_UP also changed the units of the
               to_write >= can_write return, which an IRQ loop branches
               on. Entirely outside the modelled property
```

The gaps are covered by a second instrument, not by extending this one.
Extending a frozen model to reach what it missed is the edit this entry
forbids, and the prohibition does not weaken because the extension would
have been made in good faith. A model is frozen against its author too.

```
patch vs aligned reference : 0 mismatches
unaligned vs reference     : 32 of 32 cases differ
RX memcpy / RX shift form  : 0 / 4 mismatches
over-read past a 5-byte payload:  aligned +3, unaligned +15, patch +0
```

The lesson this adds to the two above: an equivalence result inherits
the scope of its reference. Green from a discriminating model is a
statement about the sites the reference describes, and about nothing
else the same patch happens to touch.

## Classification

```
class      implementation-model equivalence  (part 1)
           evidence-role confusion           (part 2)
gate       S8 — patch/model equivalence, see references/source-audit.md
fixture    renesas_usb3 -> dwc2 TX tail, big-endian divergence

related    null-result-requires-liveness-control — there the instrument
           could not fail; here the instrument could fail and was about
           to be silenced
related    scope-precedes-rigor — there every gate passed inside a
           boundary no gate examined; here every signal passed inside an
           authority no signal examined. Recurs INSIDE this entry: the
           model's reference covered one of three changed sites, and the
           first draft claimed all three. See Model scope
related    claim-promotion-requires-boundary-proof — there the claim
           exceeded the artifact; here the artifact contradicted a prior
           artifact and the contradiction was nearly resolved in favour
           of the newer one
```

## Reverse index keys

```
technique:     [frozen discriminating model rerun against a later patch,
                cross-implementation sink comparison,
                retro-testing a gate against a known historical miss]
surface:       [the methodology itself, kernel driver source]
defect_class:  [methodology — implementation-model equivalence,
                methodology — evidence-role confusion]
```
