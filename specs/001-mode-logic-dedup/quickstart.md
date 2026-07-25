# Quickstart: Validation Guide for Mode Logic Dedup

This guide outlines the validation scenarios for the refactored mode and safety logic. 
The implementation decoupled safety evaluation from hardware execution (i.e. `checkOverflowProtection` and `checkDryRunProtection` returning `SafetyDecision` instead of directly calling `setPump()`) and consolidated the sensor freshness logic between MANUAL and COUNTDOWN modes.

## Validation Scenarios

1. **Verify Compilation and Static Checks**
   ```bash
   cd firmware/master_node
   pio check -e esp32dev --fail-on-defect high
   ```
   **Outcome:** Should pass without any high defects or warnings regarding layering/logic.

2. **Code Inspection - Single Point of Execution**
   ```bash
   grep -rn "setPump" firmware/master_node/src/safety/safety_pump.cpp
   ```
   **Outcome:** Ensure that `setPump(false)` and `setPump(true)` are *only* called within `executePumpLogic()`, never from `checkDryRunProtection()` or `checkOverflowProtection()`.

3. **Validate Safety Lockout Paths (Hardware/Serial Monitor)**
   - Run the firmware on the ESP32.
   - Disconnect the flow sensor (simulating a dry run) while the pump is ON in AUTO mode.
   - **Outcome:** System must stop the pump, log `[ERROR] DRY-RUN LOCKOUT`, and write `is_error=true` to RTDB. Wait for `clear_error` to recover.
   - Repeat the dry run simulation in MANUAL mode and COUNTDOWN mode.
   - **Outcome:** Same behavior in all modes.

4. **Validate Timer Expiration (Hardware/Serial Monitor)**
   - Trigger a 1-minute countdown.
   - Wait for 1 minute.
   - **Outcome:** Pump must stop (`setPump(false)`), `runMode` remains `COUNTDOWN`, and `countdown_active` becomes `false`.

5. **Validate Freshness Gate (Hardware/Serial Monitor)**
   - Unplug the RS-485 connection to the sensor node.
   - Request pump ON via MANUAL.
   - **Outcome:** System blocks the start, logs `No fresh/stable level data`, and forces `setPump(false)`.
