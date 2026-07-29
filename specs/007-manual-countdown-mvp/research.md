# Research Phase: Manual and Countdown MVP

## Resolved Clarifications

- **RS-485 Service**: Evaluated how to cleanly disable this. Decision: Add an early return to `rs485_init()` and bypass `rs485_poll()` in the main loop if `SENSOR_SERVICE_ENABLED` is false.
- **Relay Boot State**: The ESP32 GPIOs might float on boot. Decision: Explicitly ensure `pinMode(RELAY_PIN, OUTPUT)` and `digitalWrite(RELAY_PIN, LOW)` are the first instructions in `setup()`.
- **Command Separation**: `readFirebaseControl()` currently processes and applies logic. Decision: It will only return the active requested command, allowing the main loop state machine to decide how to act upon it (stopping old timers, starting new modes).
- **Wrap-Safe Timing**: Confirmed `elapsedMillis32` is available and sufficient for accurate tracking independent of network time.
