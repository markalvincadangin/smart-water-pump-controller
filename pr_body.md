## Overview

This PR deduplicates the core safety gate logic shared between `MANUAL` and `COUNTDOWN` modes in `safety_pump.cpp`. It addresses structural duplication while preserving hardware-safety semantics. Additionally, it implements a structural decoupling by introducing `SafetyDecision`, returning safety state to the caller instead of directly manipulating the pump relay state across multiple layers.

### Changes Included
- **Extracted Shared Gates**: Merged `MANUAL` and `COUNTDOWN` logic blocks to use unified freshness, level-sensor, and cooldown evaluations.
- **Safety Decoupling**: Introduced `SafetyDecision` enum to `safety_pump.h`. Functions like `checkSafetyCutoff` now return a decision rather than directly invoking `setPump(false)`, isolating relay actuation to the main evaluation orchestrator (`executePumpLogic`).
- **Telemetry Regression Fix**: Caught and fixed an accidental suppression of the COUNTDOWN mode fault-reporting telemetry. A strict human-reviewed diff check revealed that `lastFaultCode` and `lastFaultMessage` assignments were being suppressed for `COUNTDOWN` aborts that occurred while the pump was cooling down. Unconditional assignment for `COUNTDOWN` aborts was restored, perfectly aligning with pre-refactor semantics.
- **Docs & Protocols**: Updated `firmware_operational_rules.md` to document the unified "Shared Gates" and clarified that `isCountdownActive = false` during a safety trip is a legacy, preserved behavior. Removed stale SKILL references for Arduino IDE, acknowledging the new project-wide `platformio` consolidation.

### Verification
- **Code Trace**: The deduplicated logic was meticulously traced against pre-refactor conditionals, successfully catching the fault-code telemetry bug.
- **Compilation Gate**: Because enum signatures and return types were updated, compilation verification was enforced. The touched files successfully compiled for `master_node` via `platformio` (validating C++ `.o` compilation), and a full firmware build succeeded for `sensor_node`.
- **Convergence Lessons**: Automated "converged, zero findings" reports initially failed to identify the telemetry regression and skipped the compiler gate. A rigorous manual review was essential in closing out the feature safely.
