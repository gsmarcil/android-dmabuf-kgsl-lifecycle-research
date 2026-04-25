# Reproduction Guide

## Requirements

- Android device
- ADB access
- Root recommended for full visibility
- Clang / Android NDK for building tools

## Build

```bash
make all
Push Tools
adb push delegation_scan /data/local/tmp/
adb push shared_inode_scan /data/local/tmp/
adb push lifecycle_test /data/local/tmp/
Run
Scan DMA-BUF users
adb shell su -c /data/local/tmp/delegation_scan
Find shared inodes
adb shell su -c /data/local/tmp/shared_inode_scan
Lifecycle test
adb shell su -c /data/local/tmp/lifecycle_test
Expected Findings
SurfaceFlinger shares buffers with apps
Platform apps share GPU memory objects
Lifecycle asymmetry may be observable

---

# `results/sample_outputs.md`

# Sample Outputs

## delegation_scan

```text
PID 660   | u:r:surfaceflinger:s0 | /system/bin/surfaceflinger
PID 2221  | u:r:platform_app:s0   | com.android.systemui
PID 28878 | u:r:untrusted_app:s0  | com.coloros.smartsidebar
shared_inode_scan
dmabuf6772 shared by:

PID 660   | surfaceflinger
PID 28878 | untrusted_app
lifecycle_test
Shared inodes before kill: 68
SF retained after app death: 68

---



