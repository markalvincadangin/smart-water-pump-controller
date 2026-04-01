# SmartFlow QA Wave A Execution Report
Date: 2026-04-01
Executor: Copilot (GPT-5.3-Codex)
Spec baseline: .plan/smartflow_firmware_qa_test_spec.md

## 1. Wave A Objective
Execute preflight, compile readiness, and immediately executable non-hardware checks while preserving safety-first QA gates.

## 2. Preflight Execution Results

### 2.1 Baseline capture
- Command: scripts/qa_capture_baseline.ps1 -OutputFile docs/audit/qa/2026-04-01/day-0/baseline_snapshot.md
- Result: PASS
- Artifact: docs/audit/qa/2026-04-01/day-0/baseline_snapshot.md

Baseline values captured:
- ESP32 master firmware commit: 881195f
- NodeMCU firmware commit: 881195f
- Dashboard commit: 881195f
- Firebase rules hash: ba011277b79f46a4a20bdf3ed2835f3133469a0a

### 2.2 Hardware visibility
- Command: arduino-cli board list
- Result: no boards found
- Command: pio device list
- Result: no devices returned

Conclusion: hardware-dependent runtime tests are blocked in this environment until boards are connected.

## 3. Software-side Validation Executed

### 3.1 Production firmware compile checks
1. ESP32 master build
- Command: pio run -d firmware/platformio_smart_water_pump_controller -e esp32dev
- Result: PASS
- Memory summary: RAM 15.0%, Flash 36.0%

2. NodeMCU sensor build
- Command: pio run -d firmware/platformio_sensor_node -e nodemcuv2
- Result: PASS
- Memory summary: RAM 35.8%, Flash 26.4%

### 3.2 Test firmware compile checks
1. Sensor test sketch
- Command: arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/test_sensor_node
- Result: PASS

2. Master test sketch
- Command: arduino-cli compile --fqbn esp32:esp32:esp32 firmware/test_master_node
- Result: PASS

### 3.3 On-device test execution feasibility
- Command: pio test -d firmware/platformio_smart_water_pump_controller
- Result: BLOCKED
- Error: upload_port not specified / no attached test target

Conclusion: runtime unit/integration execution requires attached hardware ports.

### 3.4 Debug warning conformance check (FW-LOG-005)
1. Forced clean debug build
- Commands:
  - pio run -d firmware/platformio_sensor_node -e nodemcuv2_debug_usb -t clean
  - pio run -d firmware/platformio_sensor_node -e nodemcuv2_debug_usb
- Result: PASS
- Evidence line observed in build output:
  - warning: #warning "DEBUG_USB_MODE=1: bench debug mode enabled; RS-485 slave traffic is disabled for production safety."

## 4. Spec Test Cases Executed in This Wave

The following spec tests were executable via code-review method and are now marked PASS in results.csv:

1. FW-RS-007 [P2]
- Requirement: DE/RE released only after flush()
- Evidence:
  - firmware/platformio_smart_water_pump_controller/src/rs485/rs485_comm.cpp contains Serial2.flush() before TX/RX turnaround call path

2. FW-RS-008 [P2]
- Requirement: turnaround guard >=80us on ESP32 and >=60us on NodeMCU
- Evidence:
  - firmware/platformio_smart_water_pump_controller/src/config/config.h defines RS485_TX_TURNAROUND_US 80
  - firmware/platformio_sensor_node/src/config/config.h defines RS485_TX_TURNAROUND_US 60
  - both rs485 modules call delayMicroseconds(RS485_TX_TURNAROUND_US)

3. FW-LOG-005 [P3]
- Requirement: compile emits DEBUG_USB_MODE=1 warning to prevent accidental production deployment of debug mode
- Evidence:
  - clean nodemcuv2_debug_usb rebuild emitted #warning text from firmware/platformio_sensor_node/src/config/config.h

## 5. Blockers for Wave B (P1 Hardware Execution)

Current hard blocker:
- No connected ESP32/NodeMCU boards detected by Arduino CLI or PlatformIO.

Cannot execute P1 runtime tests without this:
- Boot behavior timing tests
- state-machine emergency-stop runtime tests
- dry-run/overflow fault injection timing tests
- RS-485 runtime retry/timeout behavior tests
- Firebase live one-shot reset behavior tests

## 6. Safety and Audit Position
- Safety rule preserved: no firmware behavior changed in this wave.
- Audit traceability preserved via baseline snapshot and this report.
- Tracker updated only for objectively executed code-review tests.

## 7. Immediate Next Execution Step
Once hardware is connected, start Wave B P1 gate in this order:
1. FW-BOOT-001
2. FW-BOOT-002
3. FW-BOOT-004
4. FW-SM-003
5. FW-SM-004
6. FW-SM-005
7. FW-AUTO-001
8. FW-AUTO-002
9. FW-AUTO-003
10. FW-AUTO-004
11. FW-AUTO-006
12. FW-MAN-001
13. FW-MAN-002
14. FW-MAN-003
15. FW-MAN-004
16. FW-MAN-005
17. FW-SAF-001
18. FW-SAF-002
19. FW-SAF-003
20. FW-SAF-004
21. FW-SAF-005
22. FW-SAF-006
23. FW-SAF-007
24. FW-SAF-008
25. FW-SAF-009
26. FW-SAF-010
27. FW-RS-001
28. FW-RS-002
29. FW-RS-003
30. FW-RS-004
31. FW-FB-002
32. FW-FI-001
33. FW-FI-002
34. FW-FI-009
35. FW-COUNT-006
36. FW-COUNT-007
37. FW-SM-001
38. FW-SM-002

Gate rule: do not proceed to Wave C while any P1 defect remains open.
