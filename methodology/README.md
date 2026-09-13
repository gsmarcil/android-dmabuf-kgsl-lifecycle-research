# Methodology precedents — loop-engineering skill delta

This directory is not Android research. It is the durable copy of a
change made to the `loop-engineering` skill, which lives outside any
repository (under `~/.claude/skills/synced/`, at a path containing a
session UUID) and is therefore lost when a session container is
reclaimed. The skill is updated live and mirrored here so the work
survives. It has already been reclaimed once; see **Reinstalling**.

The subject is a lesson from the DWC2 PIO campaign, kept because it
generalises well past that driver: **a patch can be green on every
conventional signal and still be wrong, and the one instrument that can
see it is the instrument most likely to be edited away.**

---

## What changed in the skill

| File | Change |
|---|---|
| `precedents/frozen-model-binding.md` | New. One entry, three rules. |
| `precedents/_index.md` | Three inverted indexes updated; stats 3 → 4 real entries, three defect classes. |
| `references/source-audit.md` | New gate `S8`: domain binding, review matrix, attribution obligation, span-is-not-effect. |
| `scripts/validate_skill.sh` | New precedent-library integrity checks. |
| `SKILL.md` | Three closure contracts. |
| `CHANGELOG.md` | `v4` authority layer, `v5` domain binding, `v6` obligation edges. |

Everything except `SKILL.md` is mirrored whole. For `SKILL.md` only the
changed block is kept, in `references/SKILL-closure-contracts.txt`.

---

## The entry

`precedents/frozen-model-binding.md` carries three rules in one file:

```
frozen-model binding      once a discriminating model has admitted an
                          implementation property, a later patch must
                          stay inside that admitted equivalence class.
                          The model is superseded only by new evidence
                          that the modeled property was wrong — never by
                          editing it until the new patch passes.

prior-art non-authority   a sibling implementation is evidence for a
                          candidate design, not evidence of equivalence.
                          Prior art generates hypotheses; the reference
                          implementation defines the preservation target;
                          the discriminating model judges candidates.

attribution obligation    knowing a finding and being obliged to apply
                          it are different. Every axis a patch changes
                          must name the finding or intent that
                          authorizes it, or review is INCOMPLETE.
```

They are one entry, not three, because they failed together on one
fixture. `renesas_usb3`'s partial-word construction was promoted from
hypothesis source to oracle, and the frozen model that contradicted it
was then nearly rewritten to agree with it. The second failure is what
disarms the defence against the first, and splitting the entry loses
exactly that.

---

## Re-audit

`AUDIT.md` is the re-audit, checked against mainline source rather than
against the diff's description of itself, and revised across four
rounds. It found five things, two material: the frozen model's reference
covers one of the three call sites the series changes, and removing
`DIV_ROUND_UP` also changed the units of a return value that a
TX-FIFO-empty IRQ loop branches on.

Two of its findings are corrections to the report itself. F2 was first
written as something the campaign had never examined; in fact D7 had
modelled it, which makes it an attribution failure rather than a
coverage one. F5 was first written as a "12-15 byte OOB read", which
promotes a source-span measurement to a memory-safety verdict without
binding the allocation.

The precedent entry has been corrected in step. The frozen model has not
been touched in any round.

---

## Evidence, re-verified

Both artifacts here were regenerated in the session that wrote the
entry, not quoted from the campaign.

### `model/dwc2-tx-equivalence-model.c` → `results/tx-equivalence-run.txt`

```bash
cc -O1 -g -Wall -Wextra -fsanitize=address,undefined \
   -o tx-equiv model/dwc2-tx-equivalence-model.c && ./tx-equiv
```

```
cases swept            : 32 (host order x offset x n)
memcpy form mismatches : 0
shift  form mismatches : 16     all on BE
verdict                : PASS
```

The model carries its own liveness control: it prints `MODEL IS BLIND`
instead of `PASS` if the rejected shift form fails to produce
mismatches. That is what makes the `0` in the memcpy column informative.

The `renesas_usb3` construction quoted in the entry was also fetched from
source and pinned by content hash — the GitHub API is blocked in this
environment, so no commit SHA was obtainable. Re-fetch and compare the
hash before reusing the citation.

### `model/dwc2-coverage-gap-model.c` → `results/coverage-gap-run.txt`

A second instrument, covering what the frozen model does not: `hcd.c`'s
unaligned TX branch and the RX helper. It exists as a separate file
rather than as an extension of the frozen model because extending a
frozen model to reach what it missed is exactly the edit the precedent
forbids.

```
patch vs aligned reference : 0 mismatches
unaligned vs reference     : 32 of 32 cases differ
RX memcpy / RX shift form  : 0 / 4 mismatches
over-read past a 5-byte payload:  aligned +3, unaligned +15, patch +0
```

### `results/validator-liveness.txt`

The validator previously said nothing about `precedents/`, so its
`[PASS]` carried no information about the library it was supposed to
protect — the exact failure mode the skill's own
`null-result-requires-liveness-control` precedent describes. It now
checks that entries are non-empty, that every entry is indexed, that
every index link resolves, and that the stated `real entries` count
matches what is on disk.

Four controls were planted against a scratch copy, one per check, plus an
unmutated control:

| planted control | caught |
|---|---|
| unmutated tree (must stay admissible) | `[PASS]`, exit 0 |
| entry written but never indexed | yes, plus stat mismatch |
| index links an entry that does not exist | yes |
| stats disagree with entries on disk | yes |
| indexed entry is empty | yes |

---

## The new gate

Appended to the `S0…S7` static-first ladder in
`references/source-audit.md`:

```
S8  for EACH changed path, does the fix satisfy the relation
    declared for that path?                      (minutes, reruns S5)
```

S8 is not one equivalence check. A frozen model binds only over the
domain, reference path and property it declared, so each changed path
declares one relation:

```
PRESERVE(reference, property)        the fix must NOT differ
CORRECT(reference, oracle)           the fix MUST differ, and must
                                     satisfy an independent oracle
INTENTIONALLY_DIVERGE(...)           the fix MUST differ, reason recorded

differs under PRESERVE                     -> FAIL
does NOT differ under CORRECT / DIVERGE    -> FAIL
```

Both directions fail. A patch that faithfully reproduces a defective
reference passes an equivalence check and fails its real obligation —
which an equivalence-only gate cannot express. Equivalence is not always
the target.

S8 also carries a review matrix, because domain is not only *which
site* but *which axis*:

```
MEMORY EFFECT / VALUE REPRESENTATION / CONTROL FLOW /
ACCOUNTING / BUS TRANSACTION COUNT      changed or preserved
```

Every changed cell needs a preservation model or a correction oracle. A
green build says nothing about any of the five.

And a cell marked `changed` owes a name:

```
OBLIGATION EDGE

for each axis marked `changed`:
    there MUST exist a finding ID, requirement ID, or recorded
    explicit-intent ID that justifies the change
    absent that -> PATCH_REVIEW = INCOMPLETE
```

Finally, S8 separates a measured address span from a memory-safety
verdict. An addressed source span is source-proven; the allocation
boundary and the forbidden read are further bindings. A wide read window
is a source-span violation until it is bound to an allocation.

---

## Reinstalling into a fresh session

This has already been needed once. A container recycle reset the live
skill to v3 and everything here had to be restored — at which point the
mirror turned out to be missing `source-audit.md` and `CHANGELOG.md`,
which had to be rebuilt by hand from prose. The mirror now carries every
changed file, so the next restore is a copy.

```bash
SKILL=$(dirname "$(find ~/.claude/skills -name SKILL.md -path '*loop-engineering*' | head -1)")

cp methodology/precedents/*.md        "$SKILL/precedents/"
cp methodology/scripts/*.sh           "$SKILL/scripts/"
cp methodology/references/source-audit.md "$SKILL/references/"
cp methodology/CHANGELOG.md           "$SKILL/CHANGELOG.md"
chmod +x "$SKILL/scripts/validate_skill.sh"

# SKILL.md is not mirrored whole. Re-apply the last three lines of
# methodology/references/SKILL-closure-contracts.txt to the matching
# block in "$SKILL/SKILL.md".

bash "$SKILL/scripts/validate_skill.sh"      # expect [PASS]
```

The skill path contains a session UUID and changes between containers,
hence the `find`. Copying `_index.md` overwrites the index: if the live
skill has gained entries since this commit, merge rather than copy — the
validator catches the resulting stat drift if you do not.
