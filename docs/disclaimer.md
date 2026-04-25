# Disclaimer

## Research Scope

This repository documents **defensive interoperability and lifecycle research** only.

It does NOT contain:
- Exploit chains or proof-of-concept exploits
- - Privilege escalation payloads
  - - SELinux bypass tools
    - - Kernel exploit code
      - - Attack frameworks or weaponized tools
       
        - ## Legal and Ethical Statement
       
        - All testing was performed on:
        - - **Owned devices** (personal OnePlus 8T, Honor device with authorization)
          - - **Root-privileged environments** (Magisk, authorized ADB access)
            - - **Isolated test setups** with no connection to production systems
             
              - No unauthorized access was performed on third-party systems.
             
              - No proprietary security mechanisms were bypassed without explicit authorization.
             
              - ## Purpose
             
              - This research aims to:
              - 1. Document defensive understanding of GPU memory lifecycle boundaries
                2. 2. Inform future hardening and policy improvements
                   3. 3. Provide a reference for researchers and security practitioners
                      4. 4. Support vendor acknowledgement and coordinated disclosure
                        
                         5. ## Not Responsible For
                        
                         6. This repository is not responsible for:
                         7. - Misuse of information contained herein
                            - - Unauthorized access or privilege escalation
                              - - Damage caused by derivative work or malicious adaptation
                                - - Violation of third-party device terms of service
                                 
                                  - ## Recommended Use
                                 
                                  - - ✅ Academic research and education
                                    - - ✅ Vendor security evaluation
                                      - - ✅ Policy and hardening decision-making
                                        - - ✅ Independent verification of findings
                                          - - ❌ Attack development or weaponization
                                            - - ❌ Unauthorized testing on third-party systems
                                             
                                              - ## Academic Reference
                                             
                                              - If you use this research, cite it as:
                                             
                                              - ```
                                                @misc{gsmarcil2026kgsl,
                                                  title={Android DMA-BUF / KGSL Lifecycle Security Research},
                                                  author={Marcil, G.S.},
                                                  year={2026},
                                                  howpublished={https://github.com/gsmarcil/android-dmabuf-kgsl-lifecycle-research}
                                                }
                                                ```

                                                ## Vendor Coordination

                                                This research is shared with relevant vendors for coordinated disclosure.

                                                Security patches addressing these findings should be contributed
                                                upstream to the Linux kernel, AOSP, and relevant OEM projects.

                                                ## Questions

                                                For inquiries regarding this research, please contact the author directly.
                                                Do not request or distribute attack tools derived from this work.
