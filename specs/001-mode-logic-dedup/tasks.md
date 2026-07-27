# Tasks: Mode Logic Dedup

**Branch**: `001-mode-logic-dedup` | **Spec**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md)

**Modules**: Firmware Master (`firmware/master_node/`)

---

## Phase 1: Shared Setup

**Purpose**: Cross-module groundwork and validation setup.

- [x] T001 Review constitution gate — confirm all applicable principles pass
- [x] T002 [P] Read current behavior in `firmware/master_node/src/safety/safety_pump.cpp` (no code changes)

**Checkpoint**: Constitution gate ✅ — implementation phases may begin

---

## Phase 2: Firmware

**Build**: `pio run -e esp32dev` in `firmware/master_node/`
**Validation**: Trace variables in code (hardware simulation) and check serial monitor requirements.

### User Story 1 — Maintain Timer End State (P1) — Firmware

- [x] T010 [FW] [US1] In `firmware/master_node/src/safety/safety_pump.cpp`: Centralize `allowStartFromSensors` and `MIN_PUMP_OFF_TIME_MS` checks inside `executePumpLogic()` before the `MANUAL` and `COUNTDOWN` mode branches.
- [x] T011 [FW] [US1] Verify by code inspection that `runMode` assignment logic remains mode-specific and is not pulled into the shared pre-evaluation block.

### User Story 2 — Maintain Hardware Lockouts (P1) — Firmware

- [x] T020 [FW] [US2] In `firmware/master_node/src/safety/safety_pump.h`: Add `SafetyDecision` enum and change `checkOverflowProtection` and `checkDryRunProtection` signatures to return `SafetyDecision`.
- [x] T021 [FW] [US2] In `firmware/master_node/src/safety/safety_pump.cpp`: Update implementation of `checkOverflowProtection` and `checkDryRunProtection` to return decisions instead of calling `setPump(false)`.
- [x] T022 [FW] [US2] Verify `setPump()` is the only relay control entry point. (Grep check, but distinguish "expected caller" from "leftover direct call", not just counting matches).
- [x] T023 [FW] [US2] Trace code path for dry-run trigger in each of AUTO, MANUAL, COUNTDOWN and confirm `SafetyDecision::STOP_DRYRUN` reaches `setPump(false)` in all three — document in `quickstart.md`.

**Checkpoint**: Firmware builds clean; relay safety invariants verified

---

## Phase 5: Integration & Documentation

**Purpose**: Cross-module validation and docs updates

- [x] T050 End-to-end validation: Trace the new enum returns in `executePumpLogic` to ensure they actually turn the pump off and do not cause logic loops.
- [x] T051 [P] Update `docs/specs/firmware_operational_rules.md` to reflect the deduplicated rules for MANUAL and COUNTDOWN.
- [x] T052 Commit: `refactor(firmware): deduplicate mode gate logic and decouple safety execution` — Conventional Commits format

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Firmware)**: Depends on Phase 1 ✅
- **Phase 5 (Integration)**: Depends on ALL module phases complete

### Within Each Module Phase

- Each user story complete and validated before starting next priority

### Parallel Opportunities

- `[P]` tasks within a phase have no intra-phase dependencies and can run in parallel

---

## Validation Summary

```bash
# Firmware — static analysis and build check
cd firmware/master_node && pio check -e esp32dev --fail-on-defect high
```

---

## Notes

- `[P]` = parallel-safe (no dependency on other `[P]` tasks in same phase)
- `[FW]` = module tag for traceability
- `[US1]` / `[US2]` = maps task to user story from spec
- Commit after each logical group, not just at phase end
- Stop at each ✅ checkpoint to validate independently before continuing
- `docs/` changes: always edit in-place; do NOT create new files for existing topics
