# Methodology precedents — loop-engineering skill delta

This directory is not Android research. It is the durable copy of a
change made to the `loop-engineering` skill, which lives outside any
repository (`~/.claude/skills/synced/loop-engineering/`) and is therefore
lost when a session container is reclaimed. The skill was updated live
and mirrored here so the work survives.

The subject is a lesson from the DWC2 PIO campaign, kept because it
generalises well past that driver: **a patch can be green on every
conventional signal and still be wrong, and the one instrument that can
see it is the instrument most likely to be edited away.**

---

## What changed in the skill

| File | Change |
|---|---|
| `precedents/frozen-model-binding.md` | New. One entry, two rules. |
| `precedents/_index.md` | Three inverted indexes updated; stats 3 → 4 real entries. |
| `references/source-audit.md` | New gate `S8` appended to the static-first ladder. |
| `scripts/validate_skill.sh` | New precedent-library integrity checks. |
| `CHANGELOG.md` | `v4 — authority layer`. |

`source-audit.md` and `CHANGELOG.md` are not mirrored whole here; the
`S8` addition is reproduced below, and the changelog entry describes the
rest.

---

## The entry

`precedents/frozen-model-binding.md` carries two rules in one file:

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
```

They are one entry, not two, because they failed together on one
fixture. `renesas_usb3`'s partial-word construction was promoted from
hypothesis source to oracle, and the frozen model that contradicted it
was then nearly rewritten to agree with it. The second failure is what
disarms the defence against the first, and splitting the entry loses
exactly that.

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
S8  does the PROPOSED FIX stay inside the equivalence class the
    model admitted?                              (minutes, reruns S5)
```

> S8 runs on every revision of the patch, not once. The S5 model is
> frozen when it first discriminates, and every later revision is judged
> against that frozen model — including revisions that build clean, pass
> checkpatch, and copy a construction from another in-tree driver. A
> revision that fails S8 loses; the model is superseded only by new
> evidence that the modeled property was itself wrong, never by editing
> it until the revision passes.

---

## Reinstalling into a fresh session

```bash
SKILL=~/.claude/skills/synced/loop-engineering
cp methodology/precedents/*.md   "$SKILL/precedents/"
cp methodology/scripts/*.sh      "$SKILL/scripts/"
# re-apply the S8 block to references/source-audit.md by hand
bash "$SKILL/scripts/validate_skill.sh"      # expect [PASS]
```

Copying `_index.md` overwrites the index. If the live skill has gained
entries since this commit, merge rather than copy — and note that the
validator will now catch the stat drift if you do not.
