# ADR 0001: Deprecate Arduino IDE Parity Build

**Date:** 2026-07-26
**Status:** Accepted

## Context
Historically, the SmartFlow firmware maintained two separate build targets that were functionally identical:
1. `platformio_*` - Modular C++ source code intended for PlatformIO/VS Code
2. `arduino_*` - Flat `.ino` sketches intended for the Arduino IDE

Maintaining both meant that every bug fix, refactor, and safety gate change (such as the mode-logic deduplication or extracting the `app/` layer) had to be written and tested twice. This dual-maintenance burden is a common source of bugs where one version drifts from the other.

## Decision
We are deprecating the Arduino IDE parity build (`arduino_*` folders) in favor of a single, canonical PlatformIO codebase.

1. The `arduino_smart_water_pump_controller` and `arduino_sensor_node` folders have been deleted.
2. The `platformio_smart_water_pump_controller` and `platformio_sensor_node` folders have been renamed to `master_node` and `sensor_node` respectively.
3. All build documentation and SKILL.md rules have been updated to reflect PlatformIO as the sole build target.

## Consequences
- **Positive:** Eliminates dual-build maintenance. Simplifies continuous integration, refactoring, and code reviews. Eliminates the risk of feature drift between the two builds.
- **Negative:** Contributors can no longer use the vanilla Arduino IDE to compile the firmware without restructuring the files. They must use PlatformIO (which wraps the Arduino framework). Given the complexity of the project, this is an acceptable tradeoff.
