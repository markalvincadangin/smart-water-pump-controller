# Layer 2 Execution Sheet - Sensor Node (SF-SN)

Date: 2026-03-31
Scope: SF-SN-001 to SF-SN-006
Prerequisite status: Layer 1 hardware tests passed (user-confirmed)

## Goal
Execute Sensor Node validation in order and produce complete evidence for each test before moving to Layer 3 RS-485 tests.

## Required Setup Before Start
- NodeMCU V2 powered and stable
- USB-TTL on GPIO2 active at 115200 for debug logs
- ESP32 + Firebase connected for status propagation checks
- Day 1 tracker open: docs/audit/qa/2026-03-31/day-1/results.csv
- Defect log open: docs/audit/qa/2026-03-31/day-1/defects.csv

## Execution Order
1. SF-SN-001 Ultrasonic sensor accuracy
2. SF-SN-002 Level percentage calculation
3. SF-SN-003 Flow sensor calibration verification
4. SF-SN-004 Sensor error flag behavior
5. SF-SN-005 Flow error hysteresis
6. SF-SN-006 Level plausibility filter

## Per-Test Evidence Minimum
- One serial log or screenshot showing actual measured output
- One Firebase screenshot when test requires status/state verification
- Notes in results.csv containing measured values and tolerance check

## Target Artifacts by Test
- SF-SN-001: 3-point level measurement table + tolerance check
- SF-SN-002: formula check snapshot for empty, full, midpoint
- SF-SN-003: bucket timing math + Firebase LPM average
- SF-SN-004: failure detect timestamp and auto-clear timestamp
- SF-SN-005: assert dwell and clear dwell timing evidence
- SF-SN-006: discard warning evidence + remote_level_discard_count increment

## Result Logging Rules
- PASS only when expected and timing/tolerance criteria are met
- FAIL requires defect ID in results.csv and entry in defects.csv
- SKIP requires explicit reason in results.csv

## Move-to-Next Gate (Layer 3)
Proceed to RS-485 tests only when:
- All SF-SN tests executed (PASS/FAIL/SKIP fully populated)
- No unresolved CRITICAL defects in SF-SN-* scope
- Evidence artifacts exist for every PASS case
