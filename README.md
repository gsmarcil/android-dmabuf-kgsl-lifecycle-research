# Android DMA-BUF / KGSL Lifecycle Security Research

Defensive research into Android shared GPU memory behavior, DMA-BUF ownership models, KGSL lifecycle semantics, and graphics trust boundaries on real devices.

---

## Overview

This repository documents practical research into how Android graphics components share and retain GPU memory buffers during normal operation.

The project focuses on:

* DMA-BUF shared memory behavior
* KGSL memory lifecycle semantics
* Cross-process buffer delegation
* SurfaceFlinger / app ownership relationships
* Cleanup and revocation behavior after process death
* Security hardening opportunities

This is a **defensive research project** intended for engineers, security teams, and researchers.

---

## Key Findings

* Trusted system components and apps may simultaneously reference shared GPU buffers.
* Buffer lifetime may differ from individual process lifetime.
* Client termination does not always imply immediate buffer destruction.
* Core graphics service restart can revoke dependent access.
* Ownership and cleanup paths may be asymmetric.
* Graphics memory lifecycle deserves security review.

---

## What This Project Does NOT Claim

This repository does **not** claim:

* privilege escalation
* sandbox escape
* SELinux bypass
* arbitrary code execution
* universal vulnerability across all devices

The findings are architectural observations and lifecycle research.

---

## Tested Devices

| Device        | Android    | Kernel |
| ------------- | ---------- | ------ |
| OnePlus 8T    | Android 14 | 4.19   |
| Honor MTN-NX1 | Android 16 | 6.1    |

Additional devices are encouraged for comparison.

---

## Repository Structure

```text id="p4v0wu"
tools/       C research utilities and scanners
docs/        Methodology, findings, matrices, limitations
results/     Sample outputs and notes
reports/     Whitepaper and disclosure drafts
methodology/ Cross-campaign research precedents and their verifiers
```

---

## Included Tools

* `delegation_scan.c` – enumerate KGSL / DMA-BUF users
* `delegation_scan2.c` – classify users by SELinux domain
* `shared_inode_scan.c` – detect shared DMA-BUF references
* `process_matrix_scan.c` – compare sharing across processes
* `lifecycle_test.c` – observe buffer behavior after client death
* `persistence_test.c` – lifecycle semantics experiments

---

## Example Questions This Research Answers

* Which apps currently hold DMA-BUF handles?
* Does SurfaceFlinger share buffers with apps?
* What survives when a process exits?
* Are ownership assumptions always accurate?
* How consistent is cleanup across vendors?

---

## Build

```bash id="1j6o8g"
make all
```

Or compile manually:

```bash id="1ofh7m"
clang -O2 tool.c -o tool
```

---

## Example Usage

```bash id="h0t5e1"
adb push delegation_scan /data/local/tmp/
adb shell chmod +x /data/local/tmp/delegation_scan
adb shell /data/local/tmp/delegation_scan
```

---

## Intended Audience

* Android platform engineers
* Kernel developers
* Mobile security researchers
* OEM hardening teams
* Academic systems researchers

---

## Future Work

* Binder graphics transaction tracing
* Vendor driver comparison
* Fence lifecycle analysis
* Multi-device reproducibility study
* Memory teardown hardening proposals

---

## License

MIT License

---

## Disclaimer

All research is for defensive analysis, interoperability understanding, and platform hardening. Users are responsible for complying with applicable laws, policies, and device ownership requirements.
