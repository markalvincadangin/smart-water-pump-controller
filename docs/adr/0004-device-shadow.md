# ADR 0004: Device Shadow Pattern

**Date**: 2026-07-27
**Status**: Accepted (Implementation Deferred to Epic 2)

## Context
Mobile apps need to control the SmartFlow pump remotely. Traditional direct RPC (Remote Procedure Call) commands assume the device is always online and will instantly apply the command. If the pump is currently locked out by a physical safety mechanism (e.g., dry-run), it cannot honor an RPC command to turn `ON`. This leads to the app displaying the pump as `ON` while the device is actually `OFF`.

## Decision
We will implement the Device Shadow pattern (`desired` vs `reported`).
The cloud maintains a JSON shadow of the device state. Clients write to `desired`. The ESP32 listens to `desired`, evaluates the physical safety conditions, actuates the hardware, and then writes the final physical state to `reported`.

## Consequences
**Positive**:
- Eliminates race conditions.
- UI always reflects the true physical state of the hardware.
- Gracefully handles offline devices (changes wait in `desired` until the device reconnects).

**Negative**:
- Adds slightly more latency between button press and UI update (app must wait for `reported` to change).
