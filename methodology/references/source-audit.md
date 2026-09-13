# SOURCE AUDIT — OPEN-SOURCE AND DRIVER-LEVEL TARGETS

Load when: the target is source you can read (kernel, driver, SDK,
library, firmware source) rather than a service you can probe.

`general-playbooks.md` routes native/kernel targets here. The difference
from runtime targets is inverted economics: static gates are nearly free
and hardware gates are expensive, so gate ordering — not tooling — is
what decides whether a campaign is worth running.

---

## 1. PIN BEFORE READING

No claim about source is admissible without a resolvable pin.

```
SOURCE_PIN
  repository:
  commit:            <full 40-char SHA, never a branch name>
  path:
  line_range:        <verified, not remembered>
```

Verify the pin resolves before citing it. A commit hash is the easiest
thing in this discipline to confabulate:

```bash
curl -s -o /dev/null -w '%{http_code}\n' \
  "https://raw.githubusercontent.com/<org>/<repo>/<sha>/<path>"
```

`200` proves the pin. Then extract the function and read the line
numbers off the extraction — never off memory, never off an earlier
session. A wrong line number in a maintainer-facing report costs more
credibility than a wrong hypothesis.

---

## 2. EVIDENCE LAYERS — NEVER MIX

Keep three registers separate in every artifact:

| Layer | Source | Admissible as |
|---|---|---|
| L1 | code you pinned and read | proof about this codebase |
| L2 | vendor databook, spec, binding docs | proof about the IP/protocol |
| L3 | third-party reporting, other implementations, prior disclosures | prior probability only |

An L3 constant used inside an L1 calculation is the most common failure.
Mark it or measure it — never assume it.

Symmetric rule: L2 silence is not L2 denial. "Not documented" is a claim
about your search, not about the hardware. Phrase it that way.

---

## 3. GATE ORDER — STATIC FIRST, ALWAYS

```
S0  pin resolves; function extracted; line numbers verified
S1  does the sink exist in source?              (free)
S2  is the accounting predicate == the effect predicate?  (free)
S3  is the vulnerable mode PRODUCTION-reachable, or did you
    enable it yourself?                          (free, decides severity)
S4  what is the introducing commit and affected range?    (free)
S5  userspace model of the defective logic under ASan/UBSan  (minutes)
S6  runtime reachability on real hardware        (expensive)
S7  impact — neighbour objects, corruption extent (expensive)
S8  for EACH changed path, does the fix satisfy the relation
    declared for that path?                      (minutes, reruns S5)
```

Do not buy hardware before S3 returns. S3 alone can move a finding from
High to Low.

### S8 — frozen-model domain binding

S8 runs on every revision of the patch, not once. The S5 model is frozen
when it first discriminates, and every later revision is judged against
it — including revisions that build clean, pass checkpatch, and copy a
construction from another in-tree driver.

A frozen model binds an implementation **only over the domain, reference
path, and property it declared**:

```
Model(reference = A, property = P) says nothing about reference = B
merely because A and B occur in the same function.
```

So S8 is not one equivalence check. For each changed path, declare
exactly one relation:

```
PRESERVE(reference, property)        the fix must NOT differ
CORRECT(reference, oracle)           the fix MUST differ, and must
                                     satisfy an independent oracle
INTENTIONALLY_DIVERGE(reference,     the fix MUST differ, with the
                      reason, oracle) reason recorded
```

Both directions can fail:

```
differs under PRESERVE                       -> FAIL
does NOT differ under CORRECT / DIVERGE      -> FAIL
```

The second line is the one an equivalence-only gate cannot express.
A patch that faithfully reproduces a defective reference passes an
equivalence check and fails its actual obligation.

The model is superseded only by new evidence that the modeled property
was itself wrong — never by editing it until the revision passes. When a
model's *domain* turns out to be narrower than the change, the answer is
a second model at the missing domain, not an extension of the first.
See `precedents/frozen-model-binding.md`.

### S8 — patch review matrix

Declaring relations only works if every changed expression is examined
on every axis it can move. For each changed expression record:

```
MEMORY EFFECT          changed / preserved
VALUE REPRESENTATION   changed / preserved
CONTROL FLOW           changed / preserved
ACCOUNTING             changed / preserved
BUS TRANSACTION COUNT  changed / preserved
```

Every changed cell needs either a preservation model or an
intentional-correction oracle. A green build says nothing about any of
the five.

The failure this catches: an expression edited for one axis silently
moves another. Removing a rounding step because a helper now takes bytes
is a MEMORY EFFECT change — and if the same variable still feeds a
comparison, it is a CONTROL FLOW change too, which no memory-safety
justification covers.

### S8 — the attribution obligation

A matrix cell marked `changed` is not discharged by noticing it. Knowing
a thing and having an obligation to apply it are different, and a long
campaign accumulates knowledge faster than it accumulates obligations.

```
OBLIGATION EDGE

for each axis marked `changed`:
    there MUST exist a finding ID, requirement ID, or recorded
    explicit-intent ID that justifies the change

    absent that -> PATCH_REVIEW = INCOMPLETE
```

This is deliberately an edge, not a reminder. The failure it prevents is
not ignorance: a patch may silently incorporate the fix for a finding
the campaign had **already modelled**, and ship it with no attribution,
because nothing forced the reviewer to look the finding up.

```
DISCOVERY   finding known and modelled
PATCH       change silently fixes it
REVIEW      reviewer does not attribute the delta to the finding

failure is not      missing finding
failure is          KNOWN FINDING  ⇏  PATCH DELTA ATTRIBUTION
```

The repair is never to reintroduce the defect to keep behaviour stable.
It is to split the change out under its own finding ID, so the delta is
attributable.

### S8 — span is not effect

A measured address span is a source-layer fact. Promoting it to a memory
-safety verdict needs two further bindings that source reading alone does
not supply:

```
addressed source span     SOURCE-PROVEN by the model
allocation boundary       needs binding to the real source object
actual forbidden read     needs effect/runtime evidence, or a
                          sufficient architectural guarantee
```

Report a wide read window as a **source-span violation** until the span
is bound to an allocation. `SOURCE SPAN REPORTED AS AN OUT-OF-BOUNDS READ
WITHOUT ALLOCATION BINDING = UNBOUND_SPAN_CLAIM`.

---

## 4. THE ASYMMETRY PATTERN

The recurring defect class in DMA, FIFO, ring, and parser code:

```
a quantity is advanced by a VARIABLE amount
and corrected/bounded by a FIXED or DIFFERENTLY-COMPUTED amount
```

Concrete instances: accounting in bytes while writing in words; a bounds
check on the raw length while the store uses the rounded length; a
pointer incremented by actual size and decremented by a nominal size.

Audit questions, in order:

1. Which quantity does the code account, and which does it actually act on?
2. Is the guard predicate identical to the safety predicate? Write both out.
3. Who owns the cursor between iterations — software or hardware?
4. Is the residual (unconsumed input) drained, or silently left behind?
5. Is the fix `min(x, cap)` sufficient, or does rounding defeat it?

Question 2 is the one that finds bugs. `if (size > max)` guarding a
`round_up(size,4)` store is a different predicate, and the gap is the bug.

---

## 5. SCOPE YOUR PREDICATE BEFORE YOU STATE IT

Any clean equivalence you derive (`bug ⟺ length % 4 != 0`) is derived
inside a sampled parameter space. Before writing `⟺` in a report,
re-run the sweep with every constant treated as a variable — packet
size, alignment, buffer size, architecture minimum alignment.

An equivalence that breaks outside the sample is worse than an
inequality, because a maintainer will find the counter-example.

---

## 6. DETECTOR VALIDITY

A sanitizer that stays silent is not a negative result until you prove
it can see the region.

- KASAN/ASan redzone the *allocation*, not sub-regions inside it. A
  buffer carved out of a larger object (`devm_kzalloc` inside `devres`,
  arena slices, sub-buffers) is invisible to overflow detection until it
  crosses the parent allocation.
- Alignment padding absorbs small backward overruns and produces false
  clean results.
- `WARN_ON_ONCE` fires once per boot: any sweep after the first case is
  silent. Replace with unconditional logging before sweeping.

State the detector's blind spot in the report before a reviewer finds it.

---

## 7. PRODUCTION-REACHABILITY DEFENCE

Prepare the answer to "you configured that yourself" before submitting:

```bash
# is the vulnerable mode selected by any in-tree platform?
grep -rn '<param>' <driver>/params.c

# or derived from hardware capability rather than chosen?
grep -rn 'CAPABILITY_MASK\|_ONLY_ARCH\|hw->' <driver>/*.c
```

An in-tree platform that selects the mode, or a capability bit that
forces it, converts a "test-only contrivance" objection into a
production finding.

---

## 8. SCOPE STATEMENT — WRITE YOUR OWN LIMITS FIRST

Every source finding declares what it does NOT claim, at the top:

```
SCOPE:      <exact path/mode affected>
NOT CLAIMED: <sibling paths that do not execute this sink>
NOT PROVEN:  <runtime reachability / impact, if still open>
```

A report that bounds itself is read twice as seriously as one that
a reviewer has to bound for you.

---

## 9. DISCLOSURE — NO PLATFORM, NO BOUNTY

Upstream open-source findings have no triage queue and no reward. Fix
the channel in `scope.md` before an artifact exists, because the report
shape differs from a platform submission:

- subsystem maintainer first (`MAINTAINERS`), security list in copy
- plain-text patch, `Fixes:` tag pointing at the introducing commit
- affected range verified branch by branch, not asserted
- reproducer harness offered privately, not posted
- disclose AI assistance where project policy requires it

---

## 10. WHAT DOES NOT BELONG IN A SOURCE CAMPAIGN

- Framing borrowed from an unrelated public exploit. If the target is
  this codebase, name it after this codebase.
- A claim about a third-party implementation's silicon derived from an
  open-source analogue.
- Any content under legal restriction, however obtained. Reconstruct
  from primary open sources or drop the line of inquiry.
