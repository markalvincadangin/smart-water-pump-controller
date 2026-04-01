# SmartFlow QA Test Implementation Plan

Version: 1.0  
Date: 2026-03-31  
Owner: QA Lead  
Source Plan: .plan/smartflow_test_plan.md

## 1. Purpose

This document converts the SmartFlow master V&V test suite into an executable implementation plan with:
- Test execution waves and schedule
- Team roles and responsibilities
- Environment and tooling setup
- Automation strategy and evidence collection
- Defect triage workflow and deployment gates

Safety-critical objective: no release unless all CRITICAL tests pass and no unresolved safety-critical defects remain.

## 2. Scope and Priorities

Execution follows the source master plan test IDs and keeps the same priorities:
- CRITICAL: mandatory pass for deployment
- HIGH: mandatory pass or approved waiver
- MEDIUM/LOW: run as capacity allows, document outcomes

Safety-sensitive suites requiring strict control:
- SF-HW-* (hardware safety checks)
- SF-FW-007/008/009/010 (lockout and E-stop behavior)
- SF-SAF-* (fault injection and protective behavior)

## 3. Team Model and Responsibilities

## 3.1 Core Roles
- QA Lead: owns execution schedule, gate decisions, sign-off package
- Firmware QA Engineer: executes SF-SN, SF-RS, SF-FW, SF-SAF, SF-REL
- Dashboard QA Engineer: executes SF-DB, SF-PWA, dashboard portions of SF-E2E
- Cloud/Backend QA Engineer: executes SF-FB, SF-SEC rules/auth validations
- Safety Observer (qualified electrical person): mandatory for [SAFETY] tests on energized rig
- Release Manager: verifies waiver list and approves production cut

## 3.2 RACI Summary
- Test execution: Responsible = QA Engineers; Accountable = QA Lead
- Safety authorization: Responsible = Safety Observer; Accountable = QA Lead
- Defect fix implementation: Responsible = Firmware/Dashboard Dev Owner
- Go/No-Go decision: Accountable = QA Lead + Release Manager

## 4. Environment and Rig Strategy

## 4.1 Required Environments
- ENV-A (Bench Dry-Run): pump disconnected or simulated for [DRY-RUN-SAFE]
- ENV-B (Full Integration): complete 220V rig with water source and live pump
- ENV-C (Dashboard/Cloud): dashboard deployment + Firebase rules/auth test environment

## 4.2 Build Variants
- Production-like firmware build (default for acceptance)
- Debug firmware build (targeted diagnostics only; limited to tests that require logs)
- Dashboard production build + local debug build for troubleshooting

## 4.3 Configuration Baselines
Before every test cycle, capture baseline snapshot:
- ESP32 firmware commit hash
- NodeMCU firmware commit hash
- Dashboard commit hash
- Firebase rules version hash
- Active thresholds and bypass flags

## 5. Execution Waves and Calendar

## 5.1 Wave Plan
Wave 0 (Day 0): Readiness and dry run
- Verify hardware readiness checklist and personnel availability
- Validate tools (serial, logic analyzer, stopwatch, browser profiles)
- Confirm test data capture sheets and defect board are ready

Wave 1 (Day 1): Hardware and comm foundations
- SF-HW-001 to SF-HW-006
- SF-SN-001 to SF-SN-006
- SF-RS-001 to SF-RS-006
Exit criteria:
- No open CRITICAL defects
- RS-485 stable enough for firmware logic testing

Wave 2 (Day 2): Firmware boot and control safety core
- SF-FW-015, SF-FW-016, SF-FW-024 first
- SF-FW-001 to SF-FW-010
- SF-FB-001, SF-FB-005, SF-FB-006
Exit criteria:
- AUTO/MANUAL/E-stop behavior validated
- Lockout semantics confirmed

Wave 3 (Day 3): Extended firmware behavior + dashboard functional
- SF-FW-011 to SF-FW-014 and SF-FW-017 to SF-FW-023
- SF-DB-001 to SF-DB-010
- SF-FB-002 to SF-FB-004, SF-FB-007
Exit criteria:
- Dashboard operational controls and validation stable
- No unresolved HIGH defects in core operations

Wave 4 (Day 4): End-to-end, edge, and security
- SF-E2E-001 to SF-E2E-005
- SF-EDG-001 to SF-EDG-010
- SF-SEC-001 to SF-SEC-008
Exit criteria:
- End-to-end workflows pass
- Security gate tests pass (auth + rules)

Wave 5 (Day 5+): Reliability and soak
- SF-REL-001 (24h soak)
- SF-REL-002 to SF-REL-005
- SF-PWA-001 to SF-PWA-007
Exit criteria:
- Soak complete with acceptable telemetry trends
- Accessibility/PWA targets met or waived with release risk acknowledgement

## 5.2 Fast Re-Test Loop (after each code fix)
Run mandatory regression subset from source plan section 16:
- SF-FW-001, SF-FW-002, SF-FW-004, SF-FW-005
- SF-FW-007, SF-FW-008, SF-FW-009, SF-FW-010
- SF-FW-015, SF-FW-016
- SF-RS-001, SF-FB-001, SF-FB-005
- SF-DB-001, SF-DB-002, SF-DB-003, SF-DB-005
- SF-E2E-001, SF-SAF-001, SF-PWA-003

## 6. Daily Operational Cadence

Daily timeline (recommended):
- 08:30-09:00: safety briefing + rig status check
- 09:00-12:00: execute planned test block
- 13:00-15:00: defect triage + fix validation
- 15:00-17:00: reruns and evidence consolidation
- 17:00-17:30: daily gate review and next-day reprioritization

Required syncs:
- Morning kick-off (15 min)
- Midday blocker review (15 min)
- End-of-day pass/fail review (30 min)

## 7. Evidence, Traceability, and Artifacts

For every test case, capture:
- Test record template fields from source plan section 17
- Timestamped serial logs (ESP32/NodeMCU)
- Firebase snapshot or export for relevant nodes
- Dashboard screenshots for UI outcomes and error states
- Video capture for [SAFETY] tests with relay/contactor behavior

Artifact structure:
- docs/audit/qa/2026-03-xx/day-<n>/
- docs/audit/qa/2026-03-xx/day-<n>/logs/
- docs/audit/qa/2026-03-xx/day-<n>/screenshots/
- docs/audit/qa/2026-03-xx/day-<n>/videos/
- docs/audit/qa/2026-03-xx/day-<n>/results.csv

Required traceability matrix columns:
- Test ID
- Requirement/Risk Mapping
- Build Hashes (FW/SN/DB)
- Result (PASS/FAIL/SKIP)
- Defect ID (if FAIL)
- Evidence Link
- Re-test Result

## 8. Defect Workflow and Severity Policy

Severity policy:
- S0 Safety-critical: unexpected pump start, failed stop, bypassable lockout
- S1 High: incorrect control latency, incorrect mode behavior, stale/incorrect safety state
- S2 Medium: non-safety functional/UI defects
- S3 Low: cosmetic/documentation issues

Workflow:
1. Log defect with precise reproduction steps and evidence path
2. Triage within same day with QA Lead + relevant Dev Owner
3. Assign fix ETA and target wave
4. Execute focused re-test + mandatory regression subset
5. Close only after evidence of pass on fixed build

Non-negotiable stop rules:
- Pause full-suite progression on any S0 until root cause is fixed and revalidated
- No production recommendation with unresolved S0/S1 impacting control/safety

## 9. Automation Plan

Immediate automation (this cycle):
- Dashboard: typecheck/build/test smoke in CI for every change
- Lint and static checks for dashboard and scripts
- Scripted Firebase path assertions for one-shot reset and auth failures

Near-term automation (next cycle):
- Hardware-in-loop smoke harness for SF-FB-001 and mode transitions
- Serial log parser for automatic timing assertions (3s status push, 6s control latency)
- RS-485 frame checker for CRC/SEQ/LDSC compatibility

Manual-only tests retained:
- Energized 220V safety procedures
- TOR and earth continuity physical checks
- Physical pump response under bypass scenarios

## 10. Entry and Exit Gates

## 10.1 Entry Gate (to start formal test execution)
- Hardware safety checklist complete and signed
- Baseline firmware/dashboard builds validated
- Firebase rules deployed and verified in test project
- Required personnel available, including safety observer

## 10.2 Exit Gate (production recommendation)
- 100% CRITICAL pass
- 100% HIGH pass or approved waivers with risk acceptance
- Soak test SF-REL-001 passed
- Security auth/rules tests SF-SEC-001 and SF-SEC-002 passed
- Accessibility score >= 95 and PWA score target met or waived
- Final Go/No-Go report issued and signed by QA Lead and Release Manager

## 11. Week-1 Deliverables

- D1: Hardware + RS-485 foundation result package
- D2: Firmware safety core result package
- D3: Dashboard + Firebase integration package
- D4: E2E + Security package
- D5: Reliability/PWA package + final consolidated matrix
- D6: Go/No-Go summary and waiver register

## 12. Implementation Risks and Mitigations

Top risks and mitigations:
- Limited access to full energized rig
  - Mitigation: maximize ENV-A dry-run-safe coverage; reserve fixed time window for ENV-B
- Intermittent WiFi/Firebase instability contaminating results
  - Mitigation: tag environment incidents separately; rerun affected tests in stable window
- Test data drift across firmware iterations
  - Mitigation: strict baseline capture and version pinning per test run
- Safety test bottleneck due to observer availability
  - Mitigation: batch all [SAFETY] tests into scheduled windows with pre-staged scripts

## 13. Immediate Next Actions (Execution Kickoff)

1. Appoint named owners for QA Lead, Firmware QA, Dashboard QA, Cloud QA, Safety Observer.
2. Create the evidence folder tree for Day 1 and prepare results.csv template.
3. Run Wave 0 readiness checklist and sign-off.
4. Execute Wave 1 test block and open defects same day.
5. Hold first daily gate review and update wave priorities based on defect severity.

