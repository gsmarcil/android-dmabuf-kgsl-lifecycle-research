# Whitepaper: Android DMA-BUF / KGSL Lifecycle Semantics Research

## Author

gsmarcil

## Repository

https://github.com/gsmarcil/android-dmabuf-kgsl-lifecycle-research

---

# Abstract

This research examines Android GPU shared-memory lifecycle behavior on real production devices, focusing on Qualcomm KGSL and Linux DMA-BUF mechanisms.

The primary objective was to understand whether memory ownership teardown, process death, object release, and userspace mappings always share identical revocation semantics.

Testing showed that in several scenarios, previously established mappings remained usable after selected release events such as free operations, file-descriptor closure, or owner termination, until explicit userspace unmapping occurred.

The results are best interpreted as lifecycle semantics observations in a shared-memory architecture. They do not independently establish privilege escalation, SELinux bypass, or arbitrary code execution.

---

# 1. Background

Modern Android graphics systems rely heavily on shared memory.

Common paths include:

- Application ↔ SurfaceFlinger
- Application ↔ GPU driver
- Vendor HAL ↔ system services
- Hardware composer ↔ display pipeline

Two important components in tested devices:

## KGSL

Qualcomm's GPU driver used on Snapdragon devices.

Provides:

- GPU memory allocation
- GPU virtual address management
- mmap support
- ioctl control surface

## DMA-BUF

Linux shared-buffer framework enabling zero-copy buffer sharing across processes and drivers.

Used throughout Android for:

- graphics buffers
- camera pipelines
- media codecs
- display composition
- GPU surfaces

---

# 2. Research Motivation

Traditional security review often focuses on:

- memory corruption
- kernel bugs
- privilege escalation
- logic flaws

Less attention is given to:

- ownership teardown
- capability persistence
- mapping revocation
- lifecycle asymmetry between cooperating processes

This research focused on those boundaries.

---

# 3. Threat Model

The study considered whether a lower-trust process that previously received legitimate access to shared GPU memory could retain access longer than expected after lifecycle changes.

Examples:

- owner exits
- allocation freed
- fd closed
- process killed
- trust boundary transitions

The study did not assume arbitrary kernel compromise.

---

# 4. Test Environment

| Device | Android | Chipset | Kernel | Access |
|--------|---------|---------|--------|--------|
| OnePlus 8T | Android 14 | Snapdragon 865 | 4.19.x | Root (Magisk) |
| Honor MTN-NX1 | Android 16 | Kirin | 6.1.x | ADB shell |

SELinux was enforcing during testing.

---

# 5. Methodology

## 5.1 Static Enumeration

Enumerated live users of:

- /dev/kgsl-3d0
- /dmabuf:*

Using /proc/<pid>/fd.

Collected:

- PID
- SELinux domain
- command line
- shared inode counts

## 5.2 Lifecycle Testing

Created allocations and tested behavior after:

- free()
- close()
- owner process exit
- child exit
- munmap()

## 5.3 Cross-Process Delegation

Used Unix sockets and SCM_RIGHTS to pass descriptors between processes.

## 5.4 Domain Survey

Observed participation of:

- system_server
- surfaceflinger
- platform_app
- priv_app
- untrusted_app
- vendor HAL processes

---

# 6. Key Observations

## 6.1 Shared Memory Is Normal Across Trust Layers

Real devices showed DMA-BUF presence across:

- trusted system services
- privileged apps
- regular apps
- vendor daemons

This confirms shared-memory delegation is normal Android behavior.

---

## 6.2 Established Access May Outlive Selected Release Events

In tested scenarios, previously mapped memory sometimes remained writable after:

- allocator free event
- descriptor close
- owner teardown

until explicit local unmapping.

This suggests object lifecycle and mapping lifecycle are not always identical.

---

## 6.3 Explicit Unmapping Was a Reliable Boundary

Across repeated tests:

- munmap() consistently removed local access.

This was the clearest revocation boundary observed from userspace.

---

## 6.4 New Access and Existing Access Behaved Differently

Some tests suggested:

- creating fresh access after teardown was blocked,
- while already-existing mappings could remain functional.

This distinction matters in security review.

---

# 7. Process Survey Results

Observed active users included:

## System Domains

- u:r:surfaceflinger:s0
- u:r:system_server:s0

## App Domains

- u:r:platform_app:s0
- u:r:priv_app:s0
- u:r:untrusted_app:s0

## Vendor Domains

Multiple vendor HAL and hardware service contexts.

This confirms real-world multi-domain memory sharing.

---

# 8. Security Interpretation

The findings are best understood as:

## Lifecycle / Capability Semantics

not automatically as:

- kernel vulnerability
- privilege escalation
- policy bypass

Security relevance arises when:

1. higher-trust components delegate writable memory,
2. teardown assumptions are incorrect,
3. lower-trust clients retain capability longer than intended.

---

# 9. What Was NOT Demonstrated

This research did not independently prove:

- root escalation
- arbitrary kernel write
- SELinux bypass
- arbitrary cross-process takeover
- universal exploitability across Android devices

These distinctions are important.

---

# 10. Limitations

## Device Variability

Behavior may differ across:

- OEM kernels
- driver revisions
- memory managers
- SELinux policy
- GPU vendor implementations

## Scope

The repository focuses on:

- lifecycle behavior
- ownership semantics
- revocation timing

not full exploit chains.

---

# 11. Why This Matters

Many modern systems rely on delegated shared memory.

Security boundaries are no longer only:

- UID
- SELinux label
- process separation

They also depend on:

- who still holds mappings
- when access is revoked
- whether teardown semantics match expectations

That makes lifecycle analysis increasingly important.

---

# 12. Recommendations

## For Kernel / Driver Developers

- document mapping lifetime explicitly
- define revocation expectations
- audit release paths
- differentiate ownership vs access semantics
- minimize writable delegation duration

## For Security Reviewers

Evaluate:

- persistent mappings
- delayed teardown
- stale capabilities
- cross-domain sharing assumptions

---

# 13. Reproducibility

The repository includes:

- scanners
- lifecycle tests
- process survey tools
- methodology notes
- sample outputs

Goal: enable independent validation.

---

# 14. Conclusion

Android graphics memory sharing is complex and highly optimized.

This research shows that access lifetime may not always match object lifetime or owner lifetime. In practice, previously established mappings can persist through selected teardown events until explicit userspace unmapping.

That behavior is not automatically a vulnerability, but it is security-relevant lifecycle semantics deserving careful review in shared-memory architectures.

---

# 15. References

- Linux DMA-BUF subsystem documentation
- Android graphics architecture documentation
- Qualcomm KGSL driver interfaces
- AOSP SurfaceFlinger documentation
- Android SELinux policy model

---

# Research Posture

This project intentionally uses conservative language:

observations are presented as observations, and conclusions are limited to what was directly tested.
