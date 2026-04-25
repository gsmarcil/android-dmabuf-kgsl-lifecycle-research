# Android DMA-BUF / KGSL Lifecycle Research

This repository contains defensive research artifacts for analyzing DMA-BUF / KGSL lifecycle semantics on Android devices.

The work focuses on:
- userspace mapping lifetime
- release and revocation boundaries
- cross-process shared buffer behavior
- trusted and less-trusted process interaction
- production device observations across Android versions

## Scope

This is a research repository, not an exploit repository.

It documents:
- runtime enumeration of KGSL and DMA-BUF users
- persistence behavior after owner teardown
- shared buffer presence across Android process domains
- lifecycle inconsistencies observed during testing

## What was tested

The following behaviors were examined:
- `GPUMEM_FREE_ID`
- `close(fd)` / `kgsl_release()`
- owner process exit
- `munmap()`
- long-lived stale mappings
- shared DMA-BUF inodes across processes
- process-domain context mapping

## Key observations

- Some mappings remained writable after release paths completed.
- `munmap()` was the only boundary that consistently revoked local access in testing.
- DMA-BUF objects were observed across multiple Android process classes, including system services, vendor services, privileged apps, and untrusted apps.
- Some lifecycle behaviors appeared reference-driven rather than strictly process-bound.

## Devices tested

Examples of tested environments:
- HONOR MTN-NX1 — Android 16
- OnePlus 8T — Android 14

## Repository contents

### tools/
Small C utilities used to enumerate processes, scan shared DMA-BUF inodes, and test lifecycle behavior.

### docs/
Notes on methodology, observed behavior, limitations, and environment details.

### results/
Sample command output and captured observations.

### reports/
Short summary draft suitable for internal tracking or later write-up.

## Important note

The results in this repository describe observed behavior and test boundaries.
They do not claim privilege escalation, arbitrary takeover, or a confirmed security vulnerability unless explicitly demonstrated.

## Suggested use

This repo is useful as:
- a defensive research reference
- a reproducibility bundle
- an Android graphics memory lifecycle study
- a basis for future policy or hardening analysis

## License

See `LICENSE`.
