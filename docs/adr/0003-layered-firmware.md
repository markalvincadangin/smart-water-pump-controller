# ADR 0003: Layered Embedded Firmware

**Date**: 2026-07-27
**Status**: Accepted

## Context
As features were added to the SmartFlow ESP32 firmware, the architecture grew into a monolithic "Big Ball of Mud", where files like `main.cpp` and `safety_pump.cpp` handled everything from cloud sync to physical hardware states. This breaks the Single Responsibility Principle and causes high risk of regressions during refactors.

## Decision
We will adopt a strict layered embedded architecture:
- `config/`: Constants, pins, feature flags.
- `hal/`: Hardware capability abstractions.
- `drivers/`: "Dumb" peripheral controllers that only read sensors and write to actuators via the HAL.
- `services/`: Domain logic that transforms raw driver data into meaningful signals (e.g. `WaterLevelService`).
- `core/`: Application orchestration (`core/app/`, `core/lifecycle/`).
- `safety/`: Independent protection logic that intercepts driver calls.

**Rule**: Dependencies point downwards. Higher layers can call lower layers, but lower layers cannot know about higher layers (e.g. drivers know nothing of cloud or safety).

## Consequences
**Positive**:
- Clear separation of concerns.
- Easy to onboard new engineers.
- Core and safety logic can be isolated and verified easily.

**Negative**:
- Requires creating many small files instead of a few large ones.
