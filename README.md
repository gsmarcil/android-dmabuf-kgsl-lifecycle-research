# Android DMA-BUF / KGSL Lifecycle Security Research

**This project analyzes Android KGSL / DMA-BUF lifecycle behavior on production devices.**

## Key Takeaway

Shared graphics buffers are common across Android trust domains, while revocation semantics are reference-driven and asymmetric. Mapping persistence after buffer release is observable across multiple Android versions and device families.

## Why This Matters

Android graphics memory is shared between apps, system services, and vendor components. Understanding ownership and revocation semantics helps:
- **Evaluate isolation guarantees** in multi-process graphics pipelines
- - **Identify hardening gaps** in memory delegation models
  - - **Assess threat surface** for privilege escalation vectors
    - - **Inform SELinux policy** for graphics driver constraints
     
      - ## Findings Summary
     
      - | Finding | Status | Evidence |
      - |---------|--------|----------|
      - | Shared DMA-BUF across trust domains | Confirmed | SurfaceFlinger manages buffers across app/system/vendor |
      - | Mapping persistence after release | Observed | munmap() does not guarantee revocation |
      - | Reference-driven revocation | Confirmed | Inode references persist in proc/debugfs |
      - | Asymmetric ownership model | Confirmed | Producer ≠ Consumer in memory semantics |
      - | Privilege escalation via DMA-BUF | Not demonstrated | SELinux constraints limit practical exploitation |
      - | SELinux bypass via graphics | Not demonstrated | Policy enforces allocator/driver boundaries |
     
      - ## Tested Environments
     
      - | Device | Android | Chipset | Kernel | Status |
      - |--------|---------|---------|--------|--------|
      - | Google Pixel 6 | 14 | Tensor | 5.15.x | ✅ Tested |
      - | OnePlus 9 | 13 | Snapdragon 888 | 5.10.x | ✅ Tested |
      - | Samsung Galaxy S21 | 13 | Exynos 2100 | 5.10.x | ✅ Tested |
      - | HONOR device | 12 | Kirin | 5.4.x | ✅ Tested |
     
      - ## Architecture Diagram
     
      - ```
        ┌─────────────────────────────────────────────────────────┐
        │                   Application Layer                      │
        │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
        │  │   Chrome    │  │   SystemUI   │  │  Gallery App │   │
        │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘   │
        └─────────┼──────────────────┼──────────────────┼───────────┘
                  │                  │                  │
                  └──────────────────┼──────────────────┘
                                    ▼
                           ┌─────────────────┐
                           │  SurfaceFlinger │  (Trust Domain 0)
                           │  (system_server)│
                           └────────┬────────┘
                                    │
                ┌───────────────────┼────────────────────┐
                │                   │                    │
                ▼                   ▼                    ▼
           ┌─────────┐      ┌──────────────┐    ┌──────────────┐
           │DMA-BUF  │      │  KGSL GPU    │    │  HW Composer │
           │ Objects │      │  Driver      │    │  HAL         │
           └────┬────┘      └──────┬───────┘    └──────┬───────┘
                │                  │                   │
                └──────────────────┼───────────────────┘
                                  ▼
                  ┌─────────────────────────────┐
                  │    ION Memory Manager       │
                  │  (Kernel Space)             │
                  └─────────────────────────────┘
                                  ▼
                  ┌─────────────────────────────┐
                  │    Display Hardware/GPU      │
                  │  (Hardware Memory)          │
                  └─────────────────────────────┘
        ```

        **Memory Lifecycle Flow:**

        ```
        Create Buffer
            ▼
        Grant Read/Write Delegation
            ▼
        Share via SurfaceFlinger
            ▼
        Multiple Consumers Hold Mappings
            ▼
        Producer Calls munmap() / Release
            ▼
        [OBSERVATION] Consumer Mappings Often Persist
            ▼
        Explicit Close Required for Revocation
            ▼
        Buffer Deallocated by ION
        ```

        ## Research Objectives

        This repository documents:

        1. **Memory Lifecycle Management** - How DMA-BUF buffers are created, shared, and released across process boundaries
        2. 2. **Ownership Delegation** - Permission transitions between trusted and less-trusted processes
           3. 3. **GPU Memory Security** - KGSL subsystem interaction with shared buffer objects
              4. 4. **Process Isolation** - Cross-process buffer behavior and access control mechanisms
                 5. 5. **Resource Revocation** - Proper cleanup and ownership release boundaries
                    6. 6. **SELinux Integration** - Policy enforcement in graphics driver context
                      
                       7. ## Reproducibility
                      
                       8. ### Prerequisites
                       9. - Android device with ADB access
                          - - Root access (for full coverage)
                            - - Target Android version 10+
                             
                              - ### Running Tests
                             
                              - ```bash
                                # Push tools to device
                                adb push results/tools/dmabuf_scan /data/local/tmp/
                                adb shell chmod +x /data/local/tmp/dmabuf_scan

                                # Execute buffer lifecycle scan
                                adb shell su -c "/data/local/tmp/dmabuf_scan --lifecycle"

                                # Monitor SurfaceFlinger buffer operations
                                adb shell cat /proc/$(adb shell pidof surfaceflinger)/maps | grep -i dmabuf

                                # Check debugfs for persistent references
                                adb shell cat /sys/kernel/debug/dma_buf/bufinfo

                                # Analyze inode persistence
                                adb shell lsof -p $(adb shell pidof surfaceflinger) | grep -i inode
                                ```

                                ### Output Interpretation

                                - **Mapped regions persisting after munmap()** → Revocation boundary observed
                                - - **Cross-process inode references** → Delegation semantics confirmed
                                  - - **SELinux denials in logcat** → Policy enforcement verification
                                   
                                    - ## Limitations
                                   
                                    - Results and observations may vary based on:
                                    - - **OEM Kernel Patches** - Android-based devices ship custom kernel modifications
                                      - - **Android Version** - Different AOSP versions implement varying buffer management
                                        - - **SELinux Policy** - OEMs customize security policies per device
                                          - - **Driver Implementation** - GPU driver behavior differs by chipset (Qualcomm, MediaTek, Exynos, Kirin)
                                            - - **Hardware Capabilities** - Memory protection units and IOMMU configurations
                                              - - **Timing Conditions** - Race conditions in buffer lifecycle may not reproduce consistently
                                               
                                                - **This is NOT a vulnerability assessment** — results document expected behavior under current Android architecture.
                                               
                                                - ## What This Is NOT
                                               
                                                - - ❌ Not an exploit framework
                                                  - - ❌ Not a privilege escalation toolkit
                                                    - - ❌ Not claiming SELinux bypasses
                                                      - - ❌ Not claiming unconstrained cross-app data access
                                                        - - ❌ Not production security advice
                                                         
                                                          - ## Tools & Artifacts
                                                         
                                                          - The repository contains:
                                                         
                                                          - - **Kernel monitoring tools** - Scripts for tracking DMA-BUF lifecycle events
                                                            - - **Process analysis utilities** - Tools to examine buffer ownership and delegation
                                                              - - **Test harnesses** - Stress testing frameworks for reproducing behavioral patterns
                                                                - - **Documentation** - Detailed analysis of findings from real device testing
                                                                  - - **Results** - Screenshots and logs from actual device testing
                                                                   
                                                                    - ## Future Work
                                                                   
                                                                    - Potential research directions:
                                                                   
                                                                    - 1. **Binder Graphics Delegation** - Tracing graphics buffer delegation through Binder IPC
                                                                      2. 2. **Vendor HAL Teardown** - Understanding vendor-specific graphics HAL behavior on buffer release
                                                                         3. 3. **Fence Synchronization** - Analyzing GPU fence semantics in shared buffer scenarios
                                                                            4. 4. **IOMMU Enforcement** - Testing IOMMU page table management during buffer lifecycle
                                                                               5. 5. **Performance Impact** - Quantifying revocation delays in high-frequency graphics scenarios
                                                                                  6. 6. **Multi-Display Scenarios** - Buffer behavior with wireless displays and composition chains
                                                                                    
                                                                                     7. ## Key Implementation Details
                                                                                    
                                                                                     8. - SELinux policy analysis for buffer access constraints
                                                                                        - - Memory ownership tracking through kernel debugfs interfaces
                                                                                          - - Cross-domain buffer behavior documentation from production devices
                                                                                            - - Hardware GPU (KGSL) interaction patterns with shared buffers
                                                                                              - - Buffer reference counting and lifecycle semantics
                                                                                               
                                                                                                - ## Notes for Developers
                                                                                               
                                                                                                - When working with DMA-BUF and KGSL:
                                                                                               
                                                                                                - 1. **Explicitly revoke** buffer access before process termination
                                                                                                  2. 2. **Verify ownership delegation** in multi-process scenarios
                                                                                                     3. 3. **Use proper SELinux contexts** for buffer operations
                                                                                                        4. 4. **Monitor lifecycle events** during shared buffer operations
                                                                                                           5. 5. **Test cleanup under abnormal termination** conditions
                                                                                                              6. 6. **Consider munmap() semantics** - release does not guarantee revocation
                                                                                                                 7. 7. **Validate cross-process access** in graphics composition pipelines
                                                                                                                   
                                                                                                                    8. ## Scope
                                                                                                                   
                                                                                                                    9. This is a **defensive research repository** documenting observations from real devices to improve understanding of Android memory management security.
                                                                                                                   
                                                                                                                    10. It is **NOT** a security vulnerability disclosure, exploit toolkit, or attack framework.
                                                                                                                   
                                                                                                                    11. ## References
                                                                                                                   
                                                                                                                    12. - Linux DMA-BUF subsystem documentation
                                                                                                                        - - Qualcomm KGSL driver architecture
                                                                                                                          - - Android security framework (SELinux, hardware security modules)
                                                                                                                            - - AOSP graphics memory management
                                                                                                                              - - SurfaceFlinger buffer management
                                                                                                                                - - ION memory allocator documentation
