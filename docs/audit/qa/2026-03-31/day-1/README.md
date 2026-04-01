# Day 1 QA Evidence Package

Date: 2026-03-31
Scope: Wave 1 foundation tests (HW, SN, RS)

## Folder Contents
- `results.csv`: Test-level result tracker for Wave 1.
- `logs/`: Serial, logic analyzer, and timing logs.
- `screenshots/`: Firebase/dashboard captures.
- `videos/`: Safety test recordings where applicable.

## Evidence Naming Convention
Use this format for every artifact:
`<test-id>_<utc-timestamp>_<artifact-type>.<ext>`

Examples:
- `SF-HW-003_20260331T021500Z_multimeter.jpg`
- `SF-RS-001_20260331T023200Z_serial.log`
- `SF-SN-004_20260331T030110Z_firebase.png`

## Minimum Evidence per Test
- PASS: at least one direct artifact proving expected behavior
- FAIL: artifact proving failure + note in `results.csv` + defect ID
- SKIP: reason in `results.csv`

## Progress Status
- Layer 1 (SF-HW-001..006): PASS (user-confirmed), tracker updated.
- Layer 2 (SF-SN-001..006): next execution block.
- Software-side automated validation: PASS (firmware builds + dashboard test/lint/build + Arduino test-sketch compile). See automated_validation_run.md.

## Next File to Use
- `layer2_sensor_node_execution.md`: ordered run sheet and gate criteria for SF-SN tests.

## Ready Next Phase Assets
- `layer3_rs485_execution.md`: RS-485 run sheet for SF-RS-001..006.
- `rs485_test_capture_template.csv`: per-test capture log for serial/logic/Firebase evidence.
