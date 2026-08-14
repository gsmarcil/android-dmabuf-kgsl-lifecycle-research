# PRECEDENT INDEX

Read at STARTUP, after `scope.md`, before the first hypothesis.
Search by defect class first, technique second, surface third.

```bash
grep -i "<defect_class>|<surface>|<technique>" \
  /mnt/skills/user/loop-engineering/precedents/_index.md
```

If a prior entry matches, read its `Gate sequence actually executed`
before ordering this campaign's gates.

`[seed]` entries are back-written from closed work and excluded from
stats. All entries follow `references/redaction-doctrine.md`.

---

## Stats

- real entries: 4
- seed entries: 0
- last updated: 2026-08-14

---

## By defect class

### accounting-vs-effect asymmetry
- [2026-08-03_dwc2-pio-word-rounding](./2026-08-03_dwc2-pio-word-rounding.md)

### authorization / capability boundary
- _(empty)_

### cache and key collision
- _(empty)_

### cryptographic protocol
- _(empty)_

### parser / canonicalization differential
- _(empty)_

### methodology — scope admissibility
- [scope-precedes-rigor](./scope-precedes-rigor.md)

### methodology — null-result admissibility
- [null-result-requires-liveness-control](./null-result-requires-liveness-control.md)

### methodology — implementation-model equivalence
- [frozen-model-binding](./frozen-model-binding.md)

### methodology — evidence-role confusion
- [frozen-model-binding](./frozen-model-binding.md)

### tenancy and context isolation
- _(empty)_

---

## By technique

### source-pinned static audit
- [2026-08-03_dwc2-pio-word-rounding](./2026-08-03_dwc2-pio-word-rounding.md)

### userspace model harness under sanitizer
- [2026-08-03_dwc2-pio-word-rounding](./2026-08-03_dwc2-pio-word-rounding.md)

### exhaustive parameter sweep for predicate scoping
- [2026-08-03_dwc2-pio-word-rounding](./2026-08-03_dwc2-pio-word-rounding.md)

### differential input / controlled mutation
- _(empty)_

### credentialed cross-origin probing
- _(empty)_

### runtime observer harness
- _(empty)_

### retro-testing a gate against a known historical miss
- [scope-precedes-rigor](./scope-precedes-rigor.md)
- [null-result-requires-liveness-control](./null-result-requires-liveness-control.md)
- [frozen-model-binding](./frozen-model-binding.md)

### frozen discriminating model rerun against a later patch
- [frozen-model-binding](./frozen-model-binding.md)

### cross-implementation sink comparison
- [frozen-model-binding](./frozen-model-binding.md)

---

## By surface

### kernel driver source
- [2026-08-03_dwc2-pio-word-rounding](./2026-08-03_dwc2-pio-word-rounding.md)
- [frozen-model-binding](./frozen-model-binding.md)

### HTTP API / web
- _(empty)_

### SDK / library
- _(empty)_

### mobile runtime
- _(empty)_

### protocol / cryptographic spec
- _(empty)_

### the methodology itself
- [scope-precedes-rigor](./scope-precedes-rigor.md)
- [null-result-requires-liveness-control](./null-result-requires-liveness-control.md)
- [frozen-model-binding](./frozen-model-binding.md)

---

## Pending back-write

Closed or in-flight work that has no entry yet. Each is one seed
requiring only `Decisive gate`, `Reusable pattern`, and
`could_it_have_run_first`:

- [ ] LaunchDarkly PHP SDK APCu cache collision (C-016)
- [ ] GoCardless CORS retest matrix
- [ ] Fireblocks W-006 / W-007 (Test A killed, Test B parked)
- [ ] OTTO.DE orbidder RTB probes
- [ ] Swiss Post e-voting protocol audit
- [ ] Android kernel / GPU / NFC campaign

Back-write in that order: the first three have the clearest gate
sequences and therefore the highest reuse value.
