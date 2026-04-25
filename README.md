## Disclaimer

This repository documents lifecycle and memory ownership behavior.
It does not claim a universal security vulnerability, privilege escalation,
or SELinux bypass.

---

# Android DMA-BUF / KGSL Lifecycle Research

A defensive study of Android GPU shared-memory lifecycle behavior across
KGSL, DMA-BUF, SELinux domains, and real production services.

The repository documents observed behavior, reproducibility, limits, and
security implications — without claiming an exploit chain.

## What this project shows

- Shared GPU memory is used across system, vendor, privileged, and app domains.
- - Some mappings persisted after free/close/owner teardown in testing.
  - - munmap() was the only consistent local revocation boundary observed.
    - - New access was constrained by policy; persistence of established access was the key observation.
     
      - ## What this project does not claim
     
      - - No privilege escalation
        - - No SELinux bypass
          - - No arbitrary cross-process takeover
            - - No confirmed exploit chain
              - - No guaranteed vulnerability classification
               
                - ## Why this matters
               
                - Android graphics memory is shared across components with different trust levels.
                - When ownership and revocation semantics are not obvious, security review becomes
                - about lifecycle guarantees rather than only memory corruption.
               
                - ## Reproducibility
               
                - The repository includes:
                - - source code for scanners and lifecycle tests
                  - - sample outputs
                    - - device matrix
                      - - methodology notes
                       
                        - The goal is to make every observation reproducible on comparable hardware and
                        - firmware, within the limits of Android security policy and device access.
                       
                        - ## Research posture
                       
                        - This project is intentionally conservative:
                        - observations are stated as observations, not conclusions beyond the data.
                       
                        - ## Documentation
                       
                        - - **docs/disclaimer.md** - Legal and ethical scope statement
                          - - **docs/whitepaper.md** - Comprehensive technical whitepaper
                            - - **docs/methodology.md** - Testing methodology and environments
                              - - **docs/device_matrix.md** - Device and test matrix
                                - - **tools/** - Source code for research tools
                                  - - **results/** - Sample outputs and observations
                                    - - **reports/** - Summary documents
                                     
                                      - ## License
                                     
                                      - MIT License - See LICENSE file for details.
