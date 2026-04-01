# SmartFlow Firmware Software QA Execution Report

Date: 2026-03-31
Scope: Software-only QA sweep for firmware projects (hardware assumed good, no live device required)
Reviewer: Copilot (GPT-5.3-Codex)

## 1) Build and Diagnostics Evidence

### 1.1 PlatformIO builds executed
- `firmware/platformio_smart_water_pump_controller`: SUCCESS
- `firmware/platformio_sensor_node` environments:
  - `nodemcuv2`: SUCCESS
  - `nodemcuv2_debug_usb`: SUCCESS
  - `nodemcuv2_ota`: SUCCESS (compile/build artifact generated; OTA upload_port warning expected in non-device context)
  - `nodemcuv2_ota_usb`: SUCCESS

### 1.2 IDE diagnostics
- No language/diagnostic errors reported for:
  - `firmware/platformio_smart_water_pump_controller/src/connectivity/connectivity_cloud.cpp`
  - `firmware/platformio_sensor_node/src/rs485/rs485_slave.cpp`

### 1.3 Unit test execution status
- `pio test` in `firmware/platformio_smart_water_pump_controller` requires a connected target (`upload_port` / `test_port`).
- Result in current environment: cannot execute on-device Unity runtime due missing hardware port.
- Existing test coverage in tree is currently limited (`test/test_crc16.cpp`).

## 2) Safety/Quality Findings and Hardening Applied

The following quality hardening changes were made with backward-compatible behavior:

1. One-shot command reliability (`manual_start`)
- File: `firmware/platformio_smart_water_pump_controller/src/connectivity/connectivity_cloud.cpp`
- Change: always clear `/pump_system/control/manual_start` after edge processing, even when command is rejected by safety gates.
- Benefit: prevents sticky one-shot command state and restores deterministic retrigger behavior.

2. One-shot command reliability (`clear_error`)
- File: `firmware/platformio_smart_water_pump_controller/src/connectivity/connectivity_cloud.cpp`
- Change: always clear `/pump_system/control/clear_error` after processing, even if there was no active error to clear.
- Benefit: prevents stale true flag and keeps remote control UX deterministic.

3. Log severity correction for successful status push
- File: `firmware/platformio_smart_water_pump_controller/src/connectivity/connectivity_cloud.cpp`
- Change: success-path status log level changed from ERROR to INFO.
- Benefit: cleaner operational telemetry and reduced false-positive alarm noise.

4. RS-485 request command parsing strictness on sensor node
- File: `firmware/platformio_sensor_node/src/rs485/rs485_slave.cpp`
- Change: command match tightened from substring match to exact command (`REQ`).
- Benefit: avoids accidental command acceptance from malformed/extra-content inputs.

## 3) Residual Risks / QA Gaps (Software-only context)

1. On-device runtime tests are pending
- Unity hardware tests cannot be executed in this session due no connected serial test target.
- Required for full QA sign-off: execute Layer 2/3 hardware test runbooks under `docs/audit/qa/2026-03-31/day-1/`.

2. Coverage depth is still light for firmware unit tests
- Existing unit test footprint is not yet broad enough for strict QA standard sign-off (state machine/safety/protocol boundary suites should be expanded).

3. Non-blocking toolchain warnings
- ESP8266 tool `elf2bin.py` emits Python `SyntaxWarning` for escape sequences; build still succeeds.

## 4) Overall Software-Only QA Status

- Compile integrity: PASS
- Static diagnostics: PASS
- Critical path hardening in this run: APPLIED
- Full QA sign-off (including device runtime/integration): PENDING HARDWARE EXECUTION

## 5) Recommended Next QA Step (for full standard pass)

Run the prepared hardware execution sheets and record evidence:
- `docs/audit/qa/2026-03-31/day-1/layer2_sensor_node_execution.md`
- `docs/audit/qa/2026-03-31/day-1/layer3_rs485_execution.md`

Then update:
- `docs/audit/qa/2026-03-31/day-1/results.csv`
- `docs/audit/qa/2026-03-31/day-1/defects.csv`
