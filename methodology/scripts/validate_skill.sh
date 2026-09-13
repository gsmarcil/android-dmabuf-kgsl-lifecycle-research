#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
SKILL="$ROOT/SKILL.md"
fail=0

require_text() {
  local text="$1"
  if ! grep -Fq "$text" "$SKILL"; then
    echo "[FAIL] missing contract: $text" >&2
    fail=1
  fi
}

for f in "$ROOT"/references/*.md; do
  [ -s "$f" ] || { echo "[FAIL] empty module: $f" >&2; fail=1; }
done

while IFS= read -r ref; do
  [ -f "$ROOT/references/$ref" ] || { echo "[FAIL] missing referenced module: $ref" >&2; fail=1; }
done < <(grep -oE '`[a-z0-9-]+\.md`' "$SKILL" | tr -d '`' | sort -u | while read -r r; do
  case "$r" in SCOPE.md|TARGET.md|LIVE.md|DEAD.md|PARKED.md|HYPOTHESES.md|ARTIFACTS.md|BLOCKERS.md|REJECTIONS.md|LESSONS.md|TARGET_PROFILE.md|CAMPAIGN_INDEX.md) ;; *) echo "$r";; esac
done)

require_text 'ATTACKER CAPABILITY EQUAL TO OR GREATER THAN RESULTING ACTION = NO_CONFIRMED_PRIVILEGE_GAIN'
require_text 'UI RESTRICTION WITHOUT SERVER-SIDE POLICY = UX_MISMATCH_NOT_YET_SECURITY'
require_text 'ONE CLOSED PATH WITH UNTESTED SIBLING PATHS = PARTIAL_FIX_COVERAGE'
require_text 'ORIGINAL EXPLOIT BLOCKED WITH REGRESSION ELSEWHERE = FIX_WITH_REGRESSION'
require_text 'LEGACY_PARK_ENTRY TREATED AS FULL PARK_ENTRY WITHOUT RECLASSIFICATION = NON_AUDITABLE_RESUME'
require_text 'STATUS UPGRADE WITHOUT AXIS-RELEVANT NEW EVIDENCE = FORBIDDEN'

if grep -Fq 'token-attack-surfaces.md' "$SKILL"; then
  echo '[FAIL] stale missing module reference: token-attack-surfaces.md' >&2
  fail=1
fi
if grep -Fq 'LIVE + artifact absent' "$SKILL"; then
  echo '[FAIL] impossible Auto PoC trigger remains' >&2
  fail=1
fi
if grep -Fq 'program explicitly rejected this class' "$SKILL"; then
  echo '[FAIL] program rejection still used as technical kill' >&2
  fail=1
fi

# --- precedent library integrity -------------------------------------------
# Without these, [PASS] is silent about precedents/ and an entry could be
# written, mis-linked, or lost without the validator noticing. That is the
# failure mode precedents/null-result-requires-liveness-control.md describes.

PRECEDENTS="$ROOT/precedents"
INDEX="$PRECEDENTS/_index.md"

if [ ! -f "$INDEX" ]; then
  echo "[FAIL] missing precedent index: $INDEX" >&2
  fail=1
else
  real=0
  for f in "$PRECEDENTS"/*.md; do
    base=$(basename "$f")
    if [ "$base" != "_index.md" ]; then
      [ -s "$f" ] || { echo "[FAIL] empty precedent entry: $base" >&2; fail=1; }
      if ! grep -Fq "(./$base)" "$INDEX"; then
        echo "[FAIL] precedent entry not indexed: $base" >&2
        fail=1
      fi
      grep -Fq '[seed]' "$f" || real=$((real + 1))
    fi
  done

  while IFS= read -r target; do
    [ -f "$PRECEDENTS/$target" ] || {
      echo "[FAIL] index links a precedent that does not exist: $target" >&2
      fail=1
    }
  done < <(grep -oE '\(\./[^)]+\.md\)' "$INDEX" | tr -d '()' | sed 's|^\./||' | sort -u)

  declared=$(grep -oE '^- real entries: [0-9]+' "$INDEX" | grep -oE '[0-9]+$' || true)
  if [ -z "$declared" ]; then
    echo "[FAIL] precedent index states no 'real entries' count" >&2
    fail=1
  elif [ "$declared" != "$real" ]; then
    echo "[FAIL] precedent index claims $declared real entries, $real on disk" >&2
    fail=1
  fi
fi

if [ "$fail" -ne 0 ]; then exit 1; fi
echo '[PASS] skill structure and canonical contracts validated'
