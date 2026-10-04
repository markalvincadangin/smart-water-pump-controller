# WP-01 — MVP Control & Safety Audit

**Project:** SmartFlow  
**Repository:** \`markalvincadangin/smart-water-pump-controller\`  
**Branch:** \`docs/smartflow-system-contract\`  
**Work Package:** WP-01  
**Status:** Audit complete; implementation changes not included  
**Scope:** Current MVP only — Manual + Countdown/Timer + safety protections

## 1. Purpose

This audit establishes the current state of SmartFlow's MVP control and safety implementation.

The audit is intentionally limited to the currently intended MVP:

- Manual mode
- Countdown/Timer mode
- Pump safety protections
- Fault handling and recovery foundations
- Sensor and communication protection relevant to MVP operation

AUTO mode is explicitly excluded from MVP acceptance. AUTO foundations already exist in the firmware and remain part of the codebase, but AUTO is still under development and has not been finalized or fully tested. Its current configuration values must therefore not be treated as the production AUTO contract.

This document records what is currently implemented, what is still uncertain, and what must be corrected or physically verified before the MVP can be considered stable.

## 2. Audit boundary

This was primarily a static repository/code audit.

Verified from the repository:

- firmware control logic
- safety logic
- state and persistence handling
- device-shadow command handling
- documented safety parameters
- current MVP control paths

Not verified by this audit:

- physical relay/contactor behavior
- actual pump shutdown under fault conditions
- emergency-stop electrical behavior
- real dry-run detection with the installed pump
- real tank-full detection
- RS-485 behavior under physical EMI/noise
- power-loss behavior of the installed hardware
- production Firebase behavior
- Android runtime behavior

A passing static audit does not mean the physical safety function has been proven.

## 3. MVP definition

### Included

- MANUAL
  - Manual ON
  - Manual OFF
- COUNTDOWN
  - Start countdown
  - Local countdown execution
  - Countdown expiry
  - Explicit stop/cancel
- SAFETY
  - Emergency stop
  - Dry-run protection
  - Maximum continuous runtime
  - Tank/full-level protection
  - Sensor validity/freshness
  - RS-485 communication protection
  - Minimum pump OFF time
  - Fault/lockout handling

### Excluded from MVP acceptance

AUTO:
- Automatic fill/start/stop behavior
- User-configurable AUTO thresholds
- AUTO hysteresis tuning
- AUTO sensor-failure strategy
- AUTO edge-case validation

AUTO may remain in the firmware and its foundations may continue to evolve, but it must not weaken or redefine the MVP safety behavior.

## 4. Overall assessment

### Verdict

**MVP control architecture: substantially implemented.**

**MVP safety architecture: substantially implemented.**

**MVP validation status: not yet complete.**

The current architecture should be preserved rather than replaced. The main work is to close contract gaps, enforce safety limits consistently, remove ambiguous terminology, and perform physical verification.

The central control principle should remain:

> User commands express intent. The ESP32 decides whether operation is safe. Reported device state describes what actually happened.

## 5. Manual mode audit

Manual control is implemented through the firmware's manual desired state.

The effective behavior is:

    MANUAL + manualDesired = true
        -> request pump operation

    MANUAL + manualDesired = false
        -> request pump OFF

Manual ON does not appear to bypass the main safety decision path.

The intended sequence is:

    Manual intent
        -> safety evaluation
        -> pump control

rather than directly forcing the relay ON.

**Assessment: PASS — architecture**

Manual mode still requires physical validation for:

- ON/OFF operation
- emergency stop interaction
- dry-run protection
- tank-full protection
- maximum runtime
- minimum OFF time
- fault recovery

## 6. Countdown/Timer audit

Countdown is implemented locally on the ESP32.

The controller receives:

- countdown start
- countdown duration

and maintains countdown state locally, including the countdown end time.

This is the correct architecture because the physical pump should not depend on the Android app remaining connected after a countdown has started.

The intended behavior is:

    Start Countdown
        -> ESP32 starts local timer
        -> pump operates if safety permits
        -> timer expires
        -> countdown becomes inactive
        -> pump is turned OFF

Safety faults can also terminate countdown operation.

**Assessment: PASS — architecture**

Required physical/runtime verification:

- countdown starts correctly
- countdown survives Android disconnect
- countdown expires correctly
- pump turns OFF at expiry
- explicit stop works
- safety faults override countdown operation

## 7. Countdown maximum-duration finding

The firmware defines:

    COUNTDOWN_MAX_DURATION_MIN = 120

However, the desired-state command path does not currently provide sufficiently clear enforcement of this maximum at the command boundary.

This creates a potential contract gap where a command greater than 120 minutes could reach the countdown command path even though 120 minutes is documented as the maximum.

### Required rule

The ESP32 must be the final authority for countdown duration.

Recommended behavior:

    1–120 minutes
        -> accept

    <= 0 or otherwise invalid
        -> reject

    > 120 minutes
        -> reject

Rejecting an invalid command is preferable to silently changing the requested value because the Android UI and physical behavior should remain consistent.

**Priority: P1**

## 8. Maximum-runtime protection

The firmware currently defines a maximum continuous runtime of:

    120 minutes

The runtime protection tracks continuous pump operation and stops the pump when the configured maximum is exceeded.

This protection is independent of the operating mode and therefore applies conceptually to Manual, Countdown, and future AUTO operation.

This is appropriate because maximum runtime is a protective limit, not a normal operating-mode feature.

**Assessment: PASS — implementation foundation**

Required verification:

    Pump ON
        -> continuous runtime reaches maximum
        -> pump OFF
        -> fault recorded/reported
        -> restart behavior follows fault policy

The old README value of 45 minutes is stale and must later be replaced by the final approved contract.

## 9. Dry-run protection

Current firmware defaults are:

    Dry-run threshold: 1.0 L/min
    Dry-run timeout:   30 seconds

The safety layer evaluates insufficient flow and can issue a dry-run stop.

The firmware also considers sensor freshness/stability before interpreting low flow as a dry-run condition. This is important because stale or invalid sensor data should not automatically be treated as physical low flow.

**Assessment: PASS — architecture**

**Physical validation required.**

The 1.0 L/min and 30-second values are engineering defaults, not values proven optimal for every operating condition of the installed system. Final values must be validated against the actual pump, well, pipework, sensor characteristics, and normal startup/priming behavior.

## 10. Level/tank-full protection

The level sensor is relevant to MVP safety even though AUTO is unfinished.

AUTO being incomplete does not mean tank level is irrelevant during Manual or Countdown operation.

A high/full tank condition should be able to stop the pump independently of AUTO.

The recommended conceptual separation is:

    Normal operating control
        !=
    Physical safety cutoff

Therefore:

- Manual can remain a user-controlled mode.
- Countdown can remain a user-controlled timed mode.
- Tank-full protection can still override both.

**Assessment: PASS — architectural direction**

**Requires physical and edge-case validation.**

## 11. Sensor bypass audit

The firmware contains level and flow sensor bypass controls.

These are useful for controlled maintenance/testing, but they should not be considered normal user operating settings.

### Required normal state

    Level sensor bypass = OFF
    Flow sensor bypass  = OFF

### Maintenance state

A bypass may be temporarily enabled only when intentionally performing maintenance, diagnosis, or controlled testing.

The flow-sensor bypass is especially sensitive because it disables protections associated with low/no flow and therefore can remove dry-run protection.

### MVP policy

Automatic sensor bypass must remain disabled.

Normal MVP operation must not depend on bypassed sensors.

### Finding

The initialization/persistence path currently has potentially ambiguous bypass defaults and should be normalized so the safe operating state is unambiguous.

**Priority: P1**

## 12. Automatic sensor bypass

The firmware contains foundations for automatic sensor-failure bypass, but this is not appropriate as an MVP operating strategy.

For the MVP:

    Sensor invalid/faulted
        -> fail safe
        -> pump OFF
        -> fault state
        -> operator investigates

rather than:

    Sensor invalid
        -> automatically bypass sensor
        -> continue pumping

The automatic bypass configuration should remain disabled by default and should not be required for Manual or Countdown operation.

**Priority: P1**

## 13. Emergency stop

Emergency stop is handled as a high-priority condition and ultimately forces the pump OFF.

The device shadow also exposes the emergency-stop latch so clients can distinguish a safety state from ordinary user intent.

The Android application must not interpret a desired ON command as proof that the pump actually started.

### Required verification

    Pump operating
        -> Emergency Stop
        -> physical pump OFF
        -> emergency-stop latch remains active
        -> normal command cannot restart pump
        -> explicit reset/recovery required

**Priority: P0 physical verification**

## 14. Minimum OFF-time

The firmware currently provides a minimum pump OFF interval of approximately:

    30 seconds

This is useful to reduce rapid restart/cycling behavior.

The protection should remain independent of the user mode.

### Required verification

    Pump OFF
        -> immediate restart request
        -> restart blocked during minimum OFF period
        -> restart permitted after period expires

**Assessment: PASS — implementation foundation; runtime validation required.**

## 15. Safety precedence

The MVP safety model should be documented around the following precedence:

    Highest priority
          |
          v
    Emergency Stop
          |
          v
    Dry-Run Protection
          |
          v
    Maximum Runtime
          |
          v
    Tank-Full / High-Level Protection
          |
          v
    Sensor and Communication Validity
          |
          v
    Minimum OFF-Time
          |
          v
    User Operating Intent
          |
          v
    Physical Pump Command

This is a conceptual safety-precedence model. WP-02 will convert it into the authoritative state/transition contract.

The key rule is:

> A user operating mode must never override a higher-priority safety condition.

## 16. Fault terminology finding

The current implementation uses overflow-related naming for the maximum-runtime protection path.

A maximum-runtime timeout does not by itself prove that an overflow occurred.

For clearer diagnostics, the system should distinguish at least:

    MAX_RUNTIME
    TANK_FULL
    OVERFLOW / HIGH_HIGH
    DRY_RUN
    SENSOR_FAULT
    COMMUNICATION_FAULT
    EMERGENCY_STOP

This is important for Android notifications, event logs, troubleshooting, and future analytics.

**Priority: P1**

WP-02 should define the authoritative fault/event vocabulary.

## 17. Current safety defaults

The following values are the current implementation values observed during this audit.

| Protection | Current value | Status |
|---|---:|---|
| Dry-run threshold | 1.0 L/min | Provisional |
| Dry-run timeout | 30 sec | Provisional |
| Maximum runtime | 120 min | Provisional |
| Minimum OFF time | 30 sec | Keep |
| Countdown maximum | 120 min | Must enforce at command boundary |
| Automatic sensor bypass | Disabled | Keep disabled |
| Level bypass normal state | Should be OFF | Normalize |
| Flow bypass normal state | Should be OFF | Normalize |

These are not yet claimed to be universally optimal engineering values. They must be validated against the actual SmartFlow installation.

## 18. Documentation drift found

The repository contains older documentation values that no longer match the current firmware.

Known examples include:

| Item | Older documentation | Current firmware |
|---|---:|---:|
| Dry-run threshold | 0.5 L/min | 1.0 L/min |
| Dry-run timeout | 15 sec | 30 sec |
| Maximum runtime | 45 min | 120 min |

These contradictions should not be resolved by arbitrarily changing the firmware to match documentation.

The proper sequence is:

    WP-01 audit
        -> identify drift

    WP-02 contract
        -> approve authoritative values/rules

    Implementation
        -> enforce contract

    Documentation update
        -> reflect final contract

## 19. AUTO boundary

AUTO is intentionally retained in the codebase because its foundations are already integrated.

However:

**AUTO is not an MVP acceptance criterion.**

Current AUTO-related values must not be interpreted as final production settings.

In particular, the current firmware's AUTO start/stop configuration should not be used to define the MVP's Manual or Countdown behavior.

Future AUTO work should separately define:

- user-configurable start level
- user-configurable stop level
- hysteresis/deadband
- safe bounds
- sensor-failure behavior
- tank-full protection
- dry-run protection
- maximum runtime
- minimum OFF time
- restart behavior
- edge cases
- physical validation

This keeps future AUTO development from destabilizing the already-defined MVP.

## 20. Findings register

### P0 — physical safety validation

- [ ] P0-01: Verify physical emergency-stop shutdown.
- [ ] P0-02: Verify physical dry-run shutdown.
- [ ] P0-03: Verify maximum-runtime shutdown.
- [ ] P0-04: Verify relay/contactor fail-safe behavior on controller power loss.

### P1 — implementation corrections

- [ ] P1-01: Enforce countdown maximum in firmware command handling.
- [ ] P1-02: Normalize sensor-bypass defaults so normal operation is unambiguously bypass OFF.
- [ ] P1-03: Keep automatic sensor bypass disabled for MVP.
- [ ] P1-04: Separate MAX_RUNTIME from OVERFLOW terminology.
- [ ] P1-05: Validate configurable safety parameters at every configuration boundary.

### P1 — documentation corrections

- [ ] P1-06: Replace stale dry-run and runtime values in README/docs.
- [ ] P1-07: Mark AUTO explicitly as under development/non-MVP.
- [ ] P1-08: Define authoritative fault/event vocabulary in WP-02.

## 21. MVP acceptance criteria

### Manual

- [ ] Manual OFF reliably stops the pump.
- [ ] Manual ON starts the pump when safety conditions permit.
- [ ] Manual ON cannot override emergency stop.
- [ ] Manual ON cannot override dry-run protection.
- [ ] Manual ON cannot override maximum runtime.
- [ ] Manual ON cannot operate when required sensor data is invalid/stale.
- [ ] Manual restart respects minimum OFF-time.
- [ ] Tank-full protection can stop Manual operation.

### Countdown

- [ ] Countdown starts locally on the ESP32.
- [ ] Countdown continues if Android disconnects.
- [ ] Countdown expires locally.
- [ ] Pump turns OFF at expiry.
- [ ] Countdown can be explicitly stopped.
- [ ] Countdown respects all safety protections.
- [ ] Countdown duration is bounded by firmware.
- [ ] Invalid duration commands are rejected.
- [ ] Countdown state is accurately reported.

### Safety

- [ ] Emergency stop works physically.
- [ ] Emergency stop remains latched until explicit recovery.
- [ ] Dry-run detection works with the actual installation.
- [ ] Dry-run causes pump OFF.
- [ ] Maximum runtime causes pump OFF.
- [ ] Tank-full protection causes pump OFF.
- [ ] Sensor/RS-485 failure follows the defined fail-safe behavior.
- [ ] Minimum OFF-time works.
- [ ] Controller power loss leaves the pump in a safe state.
- [ ] Normal operation does not require sensor bypasses.

### AUTO

- [ ] No AUTO acceptance requirement exists in WP-01.
- [ ] AUTO may remain present as development code.
- [ ] AUTO is clearly identified as unfinished.
- [ ] AUTO does not weaken MVP safety paths.

## 22. Verification boundary

This audit is not a declaration that the SmartFlow hardware is safe for unattended production operation.

The following require controlled testing:

1. Emergency stop
2. Dry-run behavior
3. Tank-full cutoff
4. Maximum runtime
5. Minimum OFF-time
6. Sensor disconnection/failure
7. RS-485 loss/noise
8. Controller reboot
9. Power interruption
10. Pump relay/contactor fail-safe behavior
11. Fault recovery/reset
12. Countdown operation without network connectivity

Tests involving the AC pump, contactor, well, or mains voltage should be performed using appropriate electrical and equipment safety procedures.

## 23. Recommended next work package

**WP-02 — SmartFlow Control & Safety Contract**

WP-02 should convert the findings in this audit into an authoritative contract covering:

- operating modes
- command schema
- reported state
- countdown rules
- safety parameters
- allowed parameter ranges
- safety precedence
- sensor validity
- bypass rules
- fault codes
- event codes
- reset/recovery semantics
- configuration ownership
- explicit AUTO boundary

WP-02 should become the reference used by the firmware, Firebase, Android application, diagrams, and documentation.

## 24. Engineering rationale

The safety structure used by SmartFlow follows a sound general pump-control principle: dry-running protection and maximum-runtime protection should be independent protective mechanisms rather than relying solely on normal operating logic. Grundfos documents dry-run protection as a pump-damage prevention mechanism and maximum-runtime protection as a way to stop abnormal continuous operation. These sources support the architecture and design direction; they do not by themselves establish the correct numerical thresholds for the specific SmartFlow installation.

Separate ON/OFF thresholds and hysteresis are also established control practices for preventing undesirable repeated switching around a threshold. This principle will become relevant to AUTO when that feature is finalized.

## 25. Final conclusion

SmartFlow's current MVP has a viable foundation.

The correct path is not a firmware rewrite. The immediate priorities are:

1. Formalize the MVP control/safety contract.
2. Enforce countdown limits at the firmware boundary.
3. Normalize sensor-bypass behavior.
4. Clarify fault terminology.
5. Synchronize stale documentation after the contract is approved.
6. Physically verify the safety functions.
7. Keep AUTO isolated as a future feature until it is separately finalized and tested.

**WP-01 is complete when the audit findings are accepted as the input to WP-02.**