# Architecture Research & Decisions

This document summarizes the core technical decisions reached during the Phase 0 Research spike for the SmartFlow Commercial IoT Platform transition. These decisions are formally expanded in the `docs/adr/` Architecture Decision Records.

---

## 1. Firmware Architecture
- **Decision**: Adopt a strict Layered Embedded Architecture (`config/`, `hal/`, `drivers/`, `services/`, `core/`, `safety/`).
- **Rationale**: The previous MVP structure placed business logic, safety rules, and hardware manipulation into the same files (e.g., `main.cpp`, `safety_pump.cpp`), creating a monolith that was dangerous to modify and impossible to unit test off-hardware.
- **Alternatives Considered**: Continuing with the organic monolithic structure, or moving to a full RTOS-centric message-passing system like FreeRTOS queues for everything. The layered approach provides the right balance of discipline without over-engineering.

## 2. Hardware Abstraction
- **Decision**: Implement a dedicated Hardware Abstraction Layer (HAL).
- **Rationale**: `drivers/` and `core/` logic must be completely ignorant of ESP32-specific GPIO pins or registers. By exposing capabilities (e.g., `PumpHal::enable()`), we can later mock the HAL for local testing or easily swap microcontrollers if supply chain issues occur.
- **Alternatives Considered**: Direct `digitalWrite()` calls inside drivers (rejected due to tight coupling).

## 3. Cloud State Synchronization (Future Epic)
- **Decision**: Use the Device Shadow Pattern (`desired` vs `reported` state).
- **Rationale**: Direct command execution from the cloud creates race conditions. If the pump is in an active Dry-Run lockout, a cloud command to turn it `ON` must be rejected locally. The shadow pattern allows the app to request a `desired` state, which the device evaluates and safely applies to the `reported` state.
- **Alternatives Considered**: Direct RPC (Remote Procedure Call) commands. Rejected because it assumes the device is always online and ready to accept commands instantaneously, which violates offline safety autonomy.

## 4. Headless Provisioning (Future Epic)
- **Decision**: Use BLE (Bluetooth Low Energy) for Wi-Fi provisioning.
- **Rationale**: Commercial IoT products require headless setup. The ESP32 supports BLE, allowing the Android App to securely transmit Wi-Fi credentials without requiring the user to connect to a temporary ESP32 Wi-Fi hotspot (which is notoriously unreliable on modern Android devices).
- **Alternatives Considered**: SmartConfig / ESPTouch (often fails on 5GHz routers), or AP Mode captive portal (poor UX).

## 5. Mobile Application (Future Epic)
- **Decision**: Rebuild the client natively using Android Jetpack Compose and MVVM.
- **Rationale**: The Next.js dashboard was sufficient for an MVP, but native BLE provisioning and background push notifications require a native application. Compose provides modern declarative UI, and MVVM cleanly separates UI state from cloud synchronization repositories.
- **Alternatives Considered**: React Native / Flutter. Rejected to maximize native BLE API stability and Android ecosystem alignment.

## 6. Cloud Backend (Future Epic)
- **Decision**: Retain Firebase (Realtime Database & Functions) for the V2 MVP.
- **Rationale**: Firebase RTDB supports offline persistence and real-time syncing out-of-the-box, which maps perfectly to the Device Shadow pattern. Moving to AWS IoT Core or Google Cloud IoT would require significantly more infrastructure overhead (MQTT brokers, cert provisioning) that is unnecessary at this scale.
- **Alternatives Considered**: AWS IoT Core. Acknowledged as the standard for enterprise IoT, but deemed too heavyweight for the current platform scale. The schema is designed vendor-agnostic so this migration remains possible later.
