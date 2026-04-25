# Android DMA-BUF / KGSL Lifecycle Security Research

## Research Objectives

This repository contains defensive research artifacts for analyzing DMA-BUF / KGSL lifecycle semantics on Android devices. The primary focus is understanding:

- **Memory Lifecycle Management**: How DMA-BUF buffers are created, shared, and released across process boundaries
- - **Ownership Delegation**: Permission transitions between trusted and less-trusted processes
  - - **GPU Memory Security**: KGSL subsystem interaction with shared buffer objects
    - - **Process Isolation**: Cross-process buffer behavior and access control mechanisms
      - - **Resource Revocation**: Proper cleanup and ownership release boundaries
       
        - ## Tested Devices
       
        - This research was conducted on real Android devices across multiple versions:
       
        - - Google Pixel series (multiple generations)
          - - Samsung Galaxy series
            - - OnePlus devices
              - - Android versions 10 through 14
               
                - ## Confirmed Results
               
                - The following behaviors were analyzed and documented:
               
                - - **Runtime enumeration** of KGSL and DMA-BUF users through `/proc` and debugfs interfaces
                  - - **Persistence behavior** after owner process teardown and lifecycle inconsistencies
                    - - **Shared buffer presence** across multiple Android process domains
                      - - **Lifecycle inconsistencies** observed during stress testing and concurrent access scenarios
                        - - **Memory state transitions** under various ownership delegation models
                          - - **SELinux integration** with DMA-BUF lifecycle management
                           
                            - ## What Is NOT a Vulnerability
                           
                            - This research specifically documents:
                           
                            - - Expected behavior per Android security model
                              - - Architectural limitations of current memory management
                                - - Design constraints in cross-process buffer sharing
                                  - - Proper functioning of existing access controls
                                   
                                    - This repository is **not** an exploit collection and does not demonstrate security bypasses.
                                   
                                    - ## Tools & Artifacts
                                   
                                    - The repository contains:
                                   
                                    - - **Kernel monitoring tools**: Scripts for tracking DMA-BUF lifecycle events
                                      - - **Process analysis utilities**: Tools to examine buffer ownership and delegation
                                        - - **Test harnesses**: Stress testing frameworks for reproducing behavioral patterns
                                          - - **Documentation**: Detailed analysis of findings from real device testing
                                           
                                            - ## Key Implementation Details
                                           
                                            - - SELinux policy analysis for buffer access
                                              - - Memory ownership tracking through kernel interfaces
                                                - - Cross-domain buffer behavior documentation
                                                  - - Hardware GPU (KGSL) interaction patterns
                                                   
                                                    - ## Notes for Developers
                                                   
                                                    - When working with DMA-BUF and KGSL:
                                                   
                                                    - 1. Always explicitly revoke buffer access before process termination
                                                      2. 2. Verify ownership delegation in multi-process scenarios
                                                         3. 3. Use proper SELinux contexts for buffer operations
                                                            4. 4. Monitor lifecycle events during shared buffer operations
                                                               5. 5. Test buffer cleanup under abnormal termination conditions
                                                                 
                                                                  6. ## Scope
                                                                 
                                                                  7. This is a research repository, not an exploit repository. It documents observations from real devices to improve understanding of Android memory management security.
                                                                 
                                                                  8. ## References
                                                                 
                                                                  9. - Linux DMA-BUF subsystem documentation
                                                                     - - Qualcomm KGSL driver architecture
                                                                       - - Android security framework (SELinux, hardware security modules)
