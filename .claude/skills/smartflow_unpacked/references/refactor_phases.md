# SmartFlow — Refactor Phases Detailed Reference

For the complete plan, see: `docs/refactor/smartflow_refactor_plan_v2.md`
This file is a quick operational reference for the agent.

---

## Phase 0 — Research & Audit (MANDATORY FIRST)

**Exit criterion:** `docs/audit/refactor_audit_2026.md` complete with all 7 sections.
**Nothing else starts until Phase 0 is done.**

### Deliverables Required

1. **File manifest** — every source file in both firmware projects and dashboard, with path,
   responsibility, dependencies, any TODO/FIXME/HACK, all compile-time flags.

2. **Pin assignment table** — extract all `#define PIN_*`, `#define RS485_*`, `RELAY_PIN`
   from both firmware projects. Compare against `hardware/wiring_notes.md`. Flag discrepancies.

3. **Firebase schema table** — read `pushFirebaseStatus()`, `readFirebaseControl()`,
   `readDeviceConfigFromFirebase()` in current form. Document every field.

4. **Dashboard stack confirmation** — framework, Firebase SDK version, component list,
   PWA manifest, current color palette.

5. **Bug triage table** — verify each of C-01 through M-06 (see SKILL.md bug registry).
   Add any new bugs with severity: Critical / High / Medium / Low.

6. **ISR safety audit** — confirm `volatile` on flow pulse counter, no unprotected reads.

7. **Revised scope** — what's still present, what's already fixed, what's new.

### Audit Report Structure

```
docs/audit/refactor_audit_2026.md

# SmartFlow Refactor Audit — 2026-03-30

## 1. Firmware — ESP32 Master
### 1.1 File manifest
### 1.2 Pin assignments vs hardware docs
### 1.3 Bug findings (severity: Critical / High / Medium / Low)
### 1.4 CRC & RS-485 protocol correctness
### 1.5 Mode state machine review
### 1.6 ISR safety findings

## 2. Firmware — NodeMCU Slave
### 2.1 File manifest
### 2.2 Bug findings

## 3. Dashboard
### 3.1 Stack + component tree
### 3.2 Firebase listener audit
### 3.3 Type safety issues
### 3.4 UI/UX bugs

## 4. Dependencies
### 4.1 npm audit summary (dashboard/ and functions/)

## 5. Rebranding checklist
(all occurrences of "Smart Water Pump Controller" to replace)

## 6. Revised scope
(what is still present, what is already fixed, what is new)
```

---

## Phase 1 — Debug Infrastructure

**Prerequisite:** Phase 0 complete.
**Must complete before any other firmware phase begins.**
**No functional behavior changes — observability only.**

Key implementation: `LOG()` macro, `LOG_SN()` macro, `DEBUG_USB_MODE` compile flag,
`gLogLevel` remote control via Firebase.

See `references/debug_system.md` for full implementation.

**Exit criteria:**
- LOG() macro in both projects
- All Serial.printf / Serial.println migrated
- gLogLevel writable via Firebase config
- debug_log_level in Firebase status push
- #warning fires for DEBUG_USB_MODE=1
- ≥80% serial volume reduction at LOG_INFO

---

## Phase 2 — Slave Node Bug Fixes

**Prerequisite:** Phase 1 complete.

Fixes: H-02, H-03, H-04, M-03.
Also adds `LDSC` field to RS-485 response frame.

See `references/bug_fixes.md` for all fix patterns.

**Exit criteria:**
- H-02, H-03, H-04, M-03 confirmed fixed via bench test
- snLevelDiscardCount visible in debug output
- Flow error flag stable under borderline conditions
- LDSC field present in RS-485 response
- Clean compilation, no new warnings

---

## Phase 3 — Master Node Bug Fixes

**Prerequisite:** Phase 1 complete.

Fixes: C-01, C-02, H-05, H-06, H-07, ISR safety, M-01, M-02, M-05, M-06.
Also: DRY_RUN_THRESHOLD_LPM default → 1.0f, LDSC parsing, Firebase write backoff,
String heap fix.

See `references/bug_fixes.md` for all fix patterns.

**Exit criteria:**
- All listed bugs confirmed fixed
- AUTO_COOLDOWN visible in Firebase and serial when off-timer active
- water_level_percent omitted from Firebase on first push (before valid frame)
- remote_level_discard_count in Firebase status
- debug_log_level in Firebase status
- Clean compilation, no new warnings

---

## Phase 4 — Protocol & Firebase Contract

**Prerequisite:** Phases 2 and 3 complete.

Verify actual firmware matches schema. Produce `docs/specs/rs485_protocol.md`.

See `references/firebase_schema.md` and `references/rs485_protocol.md`.

**Exit criteria:**
- docs/specs/rs485_protocol.md created
- pushFirebaseStatus() matches schema table exactly
- readFirebaseControl() matches control table exactly
- All new Phase 2–3 fields present

---

## Phase 5 — Test Firmware Suite

**Prerequisite:** Phase 0 complete (hardware understanding needed).

Create standalone test sketches. See `references/test_firmware.md`.

NodeMCU: TC-S-01 through TC-S-05
ESP32: TC-M-01 through TC-M-05

**Exit criteria:**
- All sketches compile independently
- TC-S-01 through TC-S-05 PASS on NodeMCU hardware
- TC-M-01 through TC-M-05 PASS on ESP32 hardware
- README.md in each test directory

---

## Phase 6 — Dashboard Redesign

**Prerequisite:** Phase 0 dashboard audit complete.

See `references/brand_design.md` for design system.

Key deliverables:
- SmartFlow color tokens in tailwind.config.ts
- Geist + Geist Mono typography (self-hosted)
- All new Firebase fields displayed:
  - Cooldown chip with countdown
  - manual_runtime_warning amber alert
  - bypass_flow_sensor toggle
  - is_idle_mode badge
  - debug_log_level control (writes to Firebase config)
  - remote_level_discard_count display
  - Level estimate visual (~82% prefix, dashed chart line)
- Rebranding string substitution
- PWA manifest updated
- Dashboard bug fixes from Phase 0 audit:
  - Firebase listener cleanup
  - Typed Firebase data (no `as any`)
  - Null checks on nested data
  - Settings validation (start < stop levels)
  - React error boundaries
  - Skeleton loaders

**Exit criteria:**
- All new Firebase fields displayed
- Cooldown, idle mode, bypass, log level all working end-to-end
- 1280px and 375px rendering correct
- Dark + light themes functional
- E-stop always visible on mobile
- No console errors on load
- SmartFlow branding throughout

---

## Phase 7 — Integration & Validation

**Prerequisite:** All previous phases complete.

Run all 21 integration tests. 2-hour soak test. Deployment sign-off checklist.

### 21 Integration Tests (Summary)

| # | Test |
|---|------|
| I-01 | Boot sequence — cold power-on both nodes |
| I-02 | Normal AUTO — pump starts |
| I-03 | AUTO stop — pump stops at target level |
| I-04 | Cooldown — pump stays off during off-timer |
| I-05 | Dry run simulation |
| I-06 | Error clear via dashboard |
| I-07 | Emergency stop |
| I-08 | E-stop reset |
| I-09 | MANUAL mode |
| I-10 | MANUAL overflow warning (pump continues) |
| I-11 | COUNTDOWN mode — 15 min then reverts |
| I-12 | Comm loss — CAT6 disconnected |
| I-13 | Level sensor bypass |
| I-14 | Flow sensor bypass |
| I-15 | Idle mode activation |
| I-16 | NVS first boot with blank NVS |
| I-17 | Production serial — only boot + errors |
| I-18 | Debug serial — DEBUG level via Firebase |
| I-19 | Dashboard mobile 375px — E-stop visible |
| I-20 | Theme switch — dark/light |
| I-21 | 2-hour soak — no watchdog/crash |

### Deployment Sign-Off Checklist

- [ ] Phase 0 audit complete and reviewed
- [ ] All Critical and High bugs resolved
- [ ] Pin assignment table verified against physical wiring
- [ ] NVS config validated on production hardware
- [ ] DRY_RUN_THRESHOLD_LPM confirmed via bucket calibration (~1.0 L/min)
- [ ] TOR dial at motor FLA (8–9A for 1.5HP 220V single-phase)
- [ ] Earth continuity < 1Ω from DIN rail to pump motor casing
- [ ] CAT6 GND tied to both enclosure GNDs (RS-485 shared reference)
- [ ] Firebase security rules reviewed
- [ ] Test suite results archived in docs/audit/test_results_2026.md
- [ ] All 21 integration tests passed
- [ ] Production firmware flashed (not debug build)
- [ ] LOG_COMPILE_FLOOR confirmed for production
