# Checkpoint: Flow Signal Diagnostics (2026-04-01)

## Scope
Pause point for SmartFlow sensor-node flow debugging before further hardware changes.

## Firmware State at Checkpoint
- Flow input pin temporarily routed to D6 / GPIO12 for diagnostics.
- Flow ISR edge mode temporarily set to FALLING.
- Flow min pulse interval temporarily relaxed to 800us (diagnostic tuning).
- Raw edge diagnostics added to per-second logs.

### Files touched in this phase
- firmware/platformio_sensor_node/src/config/config.h
- firmware/platformio_sensor_node/src/sensors/sensors.cpp
- firmware/arduino_sensor_node/sensor_node_shared.h
- firmware/arduino_sensor_node/02_sensors.ino
- dashboard/lib/constants.ts
- dashboard/lib/validation.ts
- dashboard/components/DeviceConfigSettings.tsx
- dashboard/lib/types.ts

## Latest Runtime Evidence
- Upload + monitor succeeded on COM3.
- Repeated log pattern: flow=0.00LPM, pulse=0, raw=0, disc=0, pin=0.
- One brief transient observed once: flow=0.13LPM, pulse=1, raw=9, disc=8.
- Interpretation: signal at MCU input remains mostly LOW with near-zero valid transitions.

## Hardware Notes from Session
- Sensor yellow wire measured ~4.98V without proper divider loading.
- Improper single pull-down behavior produced ~0.98V (invalid HIGH for ESP8266).
- Intended divider guidance used in session:
  - R_top: 1k (signal -> node)
  - R_bottom: 2k (node -> GND)
  - node -> MCU pin (D6/GPIO12)
- Expected node at idle HIGH: ~3.33V.

## Calibration / Safety State (retained)
- Ultrasonic near-field safety clamp to full threshold is active.
- Dashboard calibration constraints enforce ultrasonic range bounds.

## Open Issue
Flow signal is still not consistently reaching interrupt path on D6.

## Next Step on Resume
1. Verify divider midpoint voltage directly at MCU node (pump OFF and ON).
2. If midpoint is valid but logs remain pin=0/raw=0, run A/B pin test on D2/GPIO4 with same diagnostics.
3. After stable raw edges, retune debounce and restore production flow filter values.
