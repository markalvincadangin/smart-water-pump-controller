# ADR 0005: BLE Headless Provisioning

**Date**: 2026-07-27
**Status**: Accepted (Implementation Deferred to Epic 2)

## Context
ESP32 devices need Wi-Fi credentials to connect to the internet. The previous MVP hardcoded these credentials or relied on AP Mode (Captive Portal), which provides a poor user experience on modern mobile OSes that drop connections with no internet.

## Decision
We will use Bluetooth Low Energy (BLE) to provision Wi-Fi credentials and securely claim the device.
The ESP32 will boot in a `PROVISIONING` state broadcasting a BLE beacon. The mobile app connects via BLE, securely transmits the user's Wi-Fi SSID/Password, and binds the device's MAC address to the user's account in the cloud.

## Consequences
**Positive**:
- Commercial-grade UX (similar to TP-Link, Xiaomi).
- Seamless flow without forcing the user to switch Wi-Fi networks manually.
- Enables cryptographic handshake for secure device claiming.

**Negative**:
- Increases firmware binary size (BLE stack is heavy).
- Requires BLE support in the mobile app.
