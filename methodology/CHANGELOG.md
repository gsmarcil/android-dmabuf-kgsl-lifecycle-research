# Loop Engineering — Generalization Release

## Core changes

- Renamed the skill from `codex-hunter` to `loop-engineering`.
- Made the execution kernel target-agnostic; target playbooks are optional modules.
- Replaced destructive workspace initialization with idempotent creation.
- Removed the fake default `H-1 ACTIVE` hypothesis.
- Separated `security_status` from derived `submission_readiness`.
- Corrected chain semantics: missing leverage/pivot no longer kills a proven primitive.
- Prohibited legacy gate promotion from narrative or memory alone.
- Corrected Auto PoC promotion to start before `REPORTABLE` status.
- Removed program rejection as a technical kill condition.
- Added a complete canonical operational contract.
- Added cold park archive semantics and strict legacy park migration.
- Removed product-specific scanner marketing from the kernel.
- Added `scripts/validate_skill.sh`.

## v3 — scope layer (2026-08-11)

- Added `references/scope-audit.md`. The kernel gates hypotheses; nothing
  gated the space they were drawn from. SC0 instrument liveness, SC1
  spelling enumeration, SC2 boundary justification, SC3 class-claim
  scope, plus META-GATE LIVENESS and EXECUTABLE-JUDGE LIVENESS, which
  admit a new gate only after it reproduces a known historical miss and
  leaves a known-good case admissible.
- Added `precedents/scope-precedes-rigor.md`: gate quality and scope
  quality are independent; RIGOR(H) is bounded by SCOPE.
- Added `precedents/null-result-requires-liveness-control.md`: a check
  that cannot fail has not been run. Four recurrences in one campaign.
- Module registry, gate-module exemption, and two closure contracts
  updated: VOID_SWEEP and UNSCOPED_CLAIM.

Gate status at release: SC0-SC3 all NORMATIVE under META-GATE.
SC0/SC1 record EXECUTABLE-JUDGE LIVENESS = N/A (raw output, no verdict
function); SC2/SC3 record PASS with a 4/4 mutation matrix.

## v4 — authority layer (2026-08-14)

- Added `precedents/frozen-model-binding.md`. Precedent index to four
  real entries.
- Added `S8` to the `source-audit.md` gate ladder.
- Extended `scripts/validate_skill.sh` with precedent-library integrity:
  entries non-empty, every entry indexed, every index link resolving,
  and the stated `real entries` count agreeing with the entries on disk.
  Liveness recorded on a 4/4 mutation matrix.

## v5 — domain binding (2026-08-14)

Re-audit against mainline discharged the v4 formulation of S8, under the
rule's own discharge condition: the modeled property was incomplete,
shown by evidence obtained independently of any implementation.

- `S8` restated as **frozen-model domain binding**. Each changed path
  declares `PRESERVE`, `CORRECT`, or `INTENTIONALLY_DIVERGE`, and both
  directions can fail. Equivalence is not always the target.
- Added the S8 **patch review matrix**: memory effect, value
  representation, control flow, accounting, bus transaction count.

## v6 — obligation edges (2026-09-13)

The v5 matrix could mark a cell `changed` and stop there. v6 makes the
cell owe something.

- Added the **attribution obligation** to S8: every axis marked
  `changed` must name a finding, requirement, or explicit-intent ID, or
  `PATCH_REVIEW = INCOMPLETE`. Written as an edge rather than a
  reminder, because the failure it prevents is not ignorance — a patch
  can silently incorporate the fix for an already-modelled finding.
- Added **span is not effect** to S8: an addressed source span is
  source-proven, but the allocation boundary and the forbidden read are
  separate bindings. A wide read window is a source-span violation until
  bound to an allocation.
- Three closure contracts: `PATCH_REVIEW_INCOMPLETE`,
  `UNATTRIBUTED_FIX`, `UNBOUND_SPAN_CLAIM`.
- `precedents/frozen-model-binding.md` gains part 3, the attribution
  obligation, and a naming-discipline section separating `req.actual`,
  `copied_bytes`, `consumed_bytes`, and `source_span`. Indexed under a
  third defect class; still four real entries.

Recovery note: this release was written after a container recycle wiped
the live skill back to v3. v4/v5 were restored from the repository
mirror, which did not carry `source-audit.md` or `CHANGELOG.md` and had
to be reconstructed by hand. The mirror now carries every changed file.

