# Architecture Validation Quickstart

This guide outlines how to validate the Epic 1 Embedded Platform Foundation.

## Prerequisites
- Physical ESP32 hardware
- Visual Studio Code with PlatformIO extension installed.

## 1. Compilation Check

Validate that the new firmware hierarchy (`config/`, `hal/`, `drivers/`, `services/`, `core/`, `safety/`) compiles without cyclic dependencies.

```bash
cd firmware/master_node
pio run -e esp32dev
```

## 2. Hardware Simulation (Offline)

Since Epic 1 completely removes cloud and network logic from the core orchestrator, the firmware MUST behave identically to the original MVP offline mode.

1. Connect the ESP32 to the computer via USB.
2. Upload the firmware and open the serial monitor:
   ```bash
   pio run -e esp32dev --target upload
   pio device monitor -b 115200
   ```
3. **Verify Dry Run:** Trigger the manual start button (or serial command), then simulate 0 L/min flow. Wait for `cfgDryRunTimeoutSec`. The device must print `[SAFETY] Dry run detected` and the relay must disengage.
4. **Verify Overflow:** Start the pump. Let it run past the `cfgMaxPumpRuntimeMin` threshold. The device must print `[SAFETY] Overflow detected` and the relay must disengage.
5. **Verify E-Stop:** Assert the emergency stop pin/command. The pump must shut down synchronously, overriding all other logic.
