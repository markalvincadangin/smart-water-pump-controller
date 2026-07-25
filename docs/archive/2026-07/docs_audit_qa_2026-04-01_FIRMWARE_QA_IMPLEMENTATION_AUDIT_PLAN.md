# SmartFlow Firmware QA Implementation and Audit Plan
Date: 2026-04-01
Scope baseline: .plan/smartflow_firmware_qa_test_spec.md (v1.0)
Execution mode: Standardized manual-hardware execution with software-audit evidence and remediation loop

## 1. Objective
Execute the full firmware software QA specification in a standards-aligned way, produce auditable evidence for each test case, and enforce a defect workflow that enables targeted fixes and controlled retesting.

This plan is intentionally implementation-first:
- 109 total tests in scope
- 38 P1 (critical), 64 P2 (high), 7 P3 (medium)
- No P4 tests defined in current specification

## 2. Standards Alignment
This execution plan operationalizes:
- IEEE 829-2008 (test documentation and result recording)
- IEEE 1028-2008 (audit workflow and review traceability)
- IEC 61508-3 and IEC 61511 (safety behavior and fault handling verification)
- ISO/IEC 25010 (reliability and functional correctness)
- ISTQB FL v4.0 techniques (EP, BVA, decision table, state transitions)

## 3. Source Inputs and Prior Audit Reuse
Primary specification:
- .plan/smartflow_firmware_qa_test_spec.md

Prior audit context reused for consistency and speed:
- docs/audit/qa/2026-03-31/FIRMWARE_SOFTWARE_QA_EXECUTION_REPORT_2026-03-31.md
- docs/audit/qa/2026-03-31/HARDWARE_FLASH_AND_TEST_GUIDE.md
- docs/audit/qa/2026-03-31/day-1/layer2_sensor_node_execution.md
- docs/audit/qa/2026-03-31/day-1/layer3_rs485_execution.md
- docs/audit/qa/2026-03-31/day-1/test_record_template.md

## 4. Execution Strategy
Run tests in four waves. Do not proceed to the next wave until gate criteria are satisfied.

### Wave A: Environment, Baseline, and Instrumentation
Goal:
- Confirm reproducible bench setup, build hashes, logging settings, and evidence directories.

Required outputs:
- Baseline hash capture for master and sensor firmware
- Confirmation of serial/Firebase visibility
- Preflight checklist completion in day-0 notes

### Wave B: P1 Safety and Protocol Gate
Goal:
- Execute all P1 tests first (safety stop conditions, lockouts, stale data handling, protocol integrity).

Gate criteria:
- 100% of P1 tests executed
- 0 open P1 defects in status Open or In Progress
- If any P1 fails, stop progression and enter remediation loop

### Wave C: P2 Functional Correctness and Robustness
Goal:
- Execute P2 tests (configuration persistence, timing, compatibility, recovery scenarios, BVA/P2, long-duration P2).

Gate criteria:
- 100% of P2 tests executed
- No unresolved defect that can invalidate P1 behavior

### Wave D: P3 Observability and Operational Quality
Goal:
- Execute P3 log-quality and endurance operational checks.

Gate criteria:
- 100% of P3 tests executed
- Release recommendation decision recorded with residual risk statement

## 5. Defect Workflow (Audit to Fix Loop)
For every FAIL:
1. Log defect in day-0/defects.csv with severity, repro, and evidence path.
2. Link defect_id in day-0/results.csv for the failing test.
3. Classify root cause (SAFETY, FUNCTIONAL, PROTOCOL, DATA, RELIABILITY, SECURITY).
4. Implement minimal scoped fix in firmware.
5. Rebuild impacted firmware targets.
6. Retest failed test + mandatory regression subset from Section 18 of the spec.
7. Close defect only after objective evidence is attached.

Mandatory retest policy:
- Fix to safety logic: retest all P1 in Module 7 + Module 3 stop/latch tests + FW-FB-002 one-shot reset tests.
- Fix to RS-485 parser/protocol: retest FW-RS-001..FW-RS-006 and FW-SAF-009/010.
- Fix to config/NVS: retest FW-NVS-001..FW-NVS-005 and FW-BOOT-003.

## 6. Evidence Standard (per test case)
Minimum evidence per PASS:
- One raw artifact (serial log, screenshot, or capture)
- One structured test record markdown file
- Quantitative note in results.csv (timing/tolerance/threshold observed)

Minimum evidence per FAIL:
- Raw artifact showing mismatch
- Defect entry with reproducible steps
- Expected vs actual discrepancy statement

## 7. Data Artifacts in This Execution Pack
This plan initializes:
- docs/audit/qa/2026-04-01/day-0/results.csv
- docs/audit/qa/2026-04-01/day-0/defects.csv
- docs/audit/qa/2026-04-01/day-0/evidence_index.csv
- docs/audit/qa/2026-04-01/day-0/README.md

## 8. Test Ownership and Session Discipline
Recommended roles:
- QA operator: runs procedures and captures raw evidence
- Auditor: verifies evidence completeness and standards alignment
- Fix owner: implements remediation with minimal-risk changes

Session rules:
- Do not mark PASS without direct artifact
- Do not mark FAIL without defect_id
- Do not continue past Wave B while any P1 defect is open

## 9. Immediate Next Actions
1. Run Wave A preflight and baseline capture.
2. Execute P1 wave using results.csv ordered by test_id.
3. For any failure, open defect immediately and stop at gate.
4. After fixes, perform mandatory retest bundle and update closure evidence.
5. Continue with P2 then P3 only when gate rules are satisfied.

## 10. Completion Definition
QA execution is complete when all 109 tests are in terminal state (PASS, FAIL with accepted waiver, or SKIP with approved rationale), all P1 are PASS, and all evidence links resolve to stored artifacts in day-0.
