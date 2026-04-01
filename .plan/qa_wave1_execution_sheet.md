# SmartFlow QA Wave 1 Execution Sheet

Date: 2026-03-31  
Wave: 1 (HW + SN + RS)  
Environment: ENV-A for [DRY-RUN-SAFE], ENV-B only where required  
Reference Plan: .plan/smartflow_test_plan.md

## 1. Scope for This Wave
- SF-HW-001 to SF-HW-006
- SF-SN-001 to SF-SN-006
- SF-RS-001 to SF-RS-006

## 2. Mandatory Pre-Run Checks
- [ ] Wave 0 checklist signed as READY
- [ ] Baseline hashes captured
- [ ] Results tracker opened: docs/audit/qa/2026-03-31/day-1/results.csv
- [ ] Safety observer present for any [SAFETY] test
- [ ] MCB access and shutdown path confirmed

## 3. Execution Order
1. SF-HW-001 to SF-HW-006
2. SF-SN-001 to SF-SN-006
3. SF-RS-001 to SF-RS-006

Do not proceed to SN or RS if CRITICAL HW tests fail.

## 4. Per-Test Operator Procedure
1. Confirm precondition from master plan.
2. Run exact steps from test case.
3. Capture at least one direct evidence artifact.
4. Update results.csv row for the test.
5. If FAIL, file defect immediately and link defect ID.

## 5. Safety Stop Criteria
Stop wave execution immediately if any of the following occurs:
- Unexpected pump start
- Pump fails to stop when commanded
- Lockout behavior appears bypassable
- Electrical hazard observed

Escalation path:
1. Cut power at MCB if hazard exists
2. Mark test as FAIL with notes
3. Open S0 defect
4. Hold remaining tests until triage sign-off

## 6. Wave 1 Exit Criteria
- 100% of planned Wave 1 tests executed or explicitly skipped with reason
- No unresolved CRITICAL defect in HW/SN/RS behavior
- Evidence captured for all PASS results
- Defect list updated and triaged

## 7. End-of-Day Handoff
- Update wave summary in docs/audit/qa/2026-03-31/day-1/README.md
- Store logs/screenshots/videos in designated folders
- Commit audit artifacts if repository workflow allows
