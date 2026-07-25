# SmartFlow QA Day-0 Execution Workspace

This folder contains the full-scope execution and audit trackers for the firmware QA specification.

## Files
- results.csv: master execution ledger for all FW-* test cases
- defects.csv: defect tracking with lifecycle fields for remediation
- evidence_index.csv: artifact manifest for logs, screenshots, captures, and test records

## Execution Rules
- Record every test in results.csv.
- PASS requires at least one linked evidence artifact.
- FAIL requires a defect_id that exists in defects.csv.
- Retests append a new row in results.csv with attempt incremented.

## Folder Use
Create these subfolders during execution:
- logs/
- screenshots/
- videos/ (optional)
- records/

## Reference Inputs
- .plan/smartflow_firmware_qa_test_spec.md
- docs/audit/qa/2026-04-01/FIRMWARE_QA_IMPLEMENTATION_AUDIT_PLAN.md
- docs/audit/qa/2026-03-31/HARDWARE_FLASH_AND_TEST_GUIDE.md
