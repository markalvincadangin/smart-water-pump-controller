# SmartFlow Firmware Issue Register

Date: 2026-04-02

Scope:

- Master Node: ESP32 controller firmware in `firmware/platformio_smart_water_pump_controller/`
- Sensor Node: NodeMCU firmware in `firmware/platformio_sensor_node/`

Purpose:

- Record confirmed bugs, logic failures, safety risks, and operational weaknesses in a way that is easy to scan.
- Keep mitigated issues visible so future regressions are easier to spot.

## 1. Severity Scale

- Critical: unsafe pump behavior or loss of safety control.
- High: repeated outages, failed recovery, or serious operator confusion.
- Medium: reliability defect that can disrupt operation or complicate support.
- Low: limited impact, but still worth tracking.

## 2. Master Node (ESP32 Controller)

### 2.1 Confirmed Issues

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| M-01 | High | RS-485 read stalls can delay the main loop and starve Firebase work. | Control/status sync becomes unreliable under transport pressure. | Confirmed |
| M-02 | High | Firebase control and status calls are sequential and blocking. | A transport failure can cascade into repeated cloud timeouts. | Mitigated in latest firmware |
| M-03 | High | Crash-loop safe mode disables WiFi, Firebase, and sensor initialization until the latch clears or the device is power-cycled. | The controller can remain offline and unreachable. | Confirmed |
| M-04 | High | Safe-mode recovery depends on NTP/latch state and an auto-clear timer. | Recovery can be delayed or inconsistent if time sync does not settle cleanly. | Confirmed |
| M-05 | High | Emergency stop must preserve the current mode and block mode changes while latched. | A bad restart path could reintroduce unsafe pump behavior. | Mitigated in latest firmware |
| M-06 | Medium | Safety-related state is persisted in NVS, but some runtime timers remain volatile. | A reboot during active operation can reset timing context and complicate recovery logic. | Confirmed |
| M-07 | Medium | Retry/backoff logic can mask the original upstream fault. | Diagnosis becomes slower and recovery may appear random. | Confirmed |
| M-08 | Medium | WiFi/NTP setup includes blocking waits during startup and reconnect paths. | Long blocking calls can reduce responsiveness and increase watchdog risk if the environment is unstable. | Confirmed |
| M-09 | High | Secrets files are present in the firmware tree. | Real credentials could leak if files are committed or shared incorrectly. | Confirmed risk |

### 2.2 Runtime Errors Observed

| ID | Severity | Observation | Impact | Status |
|---|---|---|---|---|
| M-10 | Medium | Firebase RTDB timeout on control/status reads when RS-485 is under load. | Dashboard actions can appear delayed or lost. | Confirmed |
| M-11 | Medium | RS-485 read-frame timeout bursts during normal polling. | Sensor-node status can oscillate between online and offline. | Confirmed |
| M-12 | Low | PlatformIO test runner can fail in the Unity stage even when the firmware compile succeeds. | Test execution is noisy and not fully reliable as a validation signal. | Confirmed toolchain issue |

### 2.3 Master Node Notes

- The latest firmware changes reduced the cloud timeout cascade and improved emergency-stop handling.
- The remaining concern is not raw compile stability; it is how the controller behaves under transport stalls, safe-mode recovery, and persisted state transitions.

## 3. Sensor Node (NodeMCU)

### 3.1 Confirmed Issues

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| S-01 | Medium | RS-485 slave framing uses a short partial-frame stall reset to avoid hanging on malformed packets. | Incomplete or noisy packets can interrupt polling until the receiver resets. | Confirmed |
| S-02 | Medium | Response turnaround is immediate after valid packet reception. | If the Master has not fully switched to RX, the first reply can be lost. | Confirmed risk |
| S-03 | Medium | Ultrasonic plausibility filtering can reject large level jumps. | Real level changes may be delayed if they look like outliers. | Confirmed |
| S-04 | Medium | Flow hardening uses aggressive deglitching and diagnostic tuning values. | Legitimate pulses may be dropped if the installed sensor behaves differently in the field. | Needs field validation |
| S-05 | Low | Sensor-node logic is primarily telemetry-oriented and depends on the Master for final safety action. | A Master-side fault can delay the actual safety response. | Design limitation |

### 3.2 Sensor Node Notes

- The sensor node is intentionally lightweight and mostly reports data.
- Its biggest risk is not a single catastrophic local bug; it is communication quality and sensor filtering interacting with the Master node’s safety logic.

## 4. Cross-System Risks

| ID | Severity | Risk | Impact | Status |
|---|---|---|---|---|
| X-01 | Critical | Any condition that leaves the pump ON during a communication fault. | This is the most important safety failure mode. | Must never regress |
| X-02 | High | Backward-compatibility logic for deprecated control fields can override a safer current state if not handled carefully. | A stale or malformed command could cause unexpected mode behavior. | Confirmed risk |
| X-03 | Medium | Persistent state in NVS/Firebase can outlive a reboot. | Stale flags can affect startup behavior if not explicitly cleared. | Confirmed |

## 5. Open Items to Watch

- Validate emergency-stop behavior after a full field soak with repeated dashboard toggles.
- Confirm watchdog and reconnect behavior under poor WiFi plus RS-485 traffic.
- Decide whether the sensor node should expose any additional local fault signaling beyond telemetry.
- Keep secrets files out of version control and ensure example files stay sanitized.

## 6. Traceability Notes

- This document is intentionally operational, not academic.
- Items marked as mitigated remain listed so future regressions are easier to detect.
- Add new findings only after they are reproduced on hardware, seen in logs, or verified in code.