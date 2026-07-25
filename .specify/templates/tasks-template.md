<!--
  ============================================================================
  SMARTFLOW TASKS TEMPLATE

  This file is the SAMPLE/GUIDE. The /speckit-tasks command replaces these
  with actual tasks based on:
    - User stories from spec.md (priorities P1, P2, P3...)
    - Module phases from plan.md
    - Entities from data-model.md / contracts/

  Tasks are organized by module then by user story so each story can be
  implemented, tested, and validated independently.

  IMPORTANT: Delete inapplicable module phases before using.
  ============================================================================
-->

# Tasks: [FEATURE NAME]

**Branch**: `[###-feature-name]` | **Spec**: [link] | **Plan**: [link]

**Modules**: [list which modules this feature touches]

---

## Phase 1: Shared Setup

**Purpose**: Any cross-module groundwork (schema design sign-off, new RTDB nodes, shared types)

- [ ] T001 Review constitution gate — confirm all applicable principles pass
- [ ] T002 [P] Read current behavior in affected files (no code changes)
- [ ] T003 [P] Confirm RS-485 frame / RTDB schema changes are backward compatible (if applicable)

**Checkpoint**: Constitution gate ✅ — implementation phases may begin

---

<!-- ============================================================
  FIRMWARE PHASE — delete this block if firmware is not touched
============================================================ -->
## Phase 2: Firmware *(if applicable)*

**Build**: `pio run` in `firmware/platformio_smart_water_pump_controller/` or `firmware/platformio_sensor_node/`
**Validation**: Hardware serial monitor (no CI — document expected serial output)

### User Story 1 — [Title] (P1) — Firmware

- [ ] T010 [FW] [US1] [describe: file path + function + what changes]
- [ ] T011 [FW] [US1] [describe: safety invariant check — grep for direct RELAY_PIN writes]
- [ ] T012 [FW] [US1] [describe: timing check — grep for raw millis() arithmetic]
- [ ] T013 [FW] [US1] Document expected serial output for manual hardware validation

**Checkpoint**: Firmware builds clean; relay safety invariants verified

---

<!-- ============================================================
  DASHBOARD PHASE — delete this block if dashboard is not touched
============================================================ -->
## Phase 3: Dashboard *(if applicable)*

**Dev**: `cd dashboard && npm run dev` (port 3000)
**Test**: `cd dashboard && npm test`
**Validate**: `cd dashboard && npm run validate` (lint + build + lighthouse)

### Tests — write first, must FAIL before implementation

- [ ] T020 [P] [DB-T] [US1] Write test in `dashboard/__tests__/[file].test.tsx` — verify it FAILS
- [ ] T021 [P] [DB-T] [US1] Write test in `dashboard/__tests__/[file].test.ts` — verify it FAILS

### User Story 1 — [Title] (P1) — Dashboard

- [ ] T022 [DB] [US1] [describe: component/hook + file path + what changes]
- [ ] T023 [DB] [US1] [describe]
- [ ] T024 [DB] [US1] Confirm pre-existing failure count unchanged: `__tests__/lib/usePumpData.test.tsx` (expect 4 failures)

**Checkpoint**: `npm test` passes (minus pre-existing 4); `npm run validate` clean

### User Story 2 — [Title] (P2) — Dashboard *(if applicable)*

- [ ] T030 [DB] [US2] [describe]

---

<!-- ============================================================
  CLOUD FUNCTIONS PHASE — delete if functions are not touched
============================================================ -->
## Phase 4: Cloud Functions *(if applicable)*

**Build**: `cd functions && npm run build`
**Test**: `cd functions && npm test`

### Tests — write first, must FAIL before implementation

- [ ] T040 [FN-T] [US1] Write test in `functions/src/__tests__/[file].test.ts` — verify it FAILS

### User Story 1 — [Title] (P1) — Functions

- [ ] T041 [FN] [US1] [describe: function name + trigger + what changes]
- [ ] T042 [FN] [US1] [describe]

**Checkpoint**: `tsc` clean; `npm test` passes

---

## Phase 5: Integration & Documentation

**Purpose**: Cross-module validation and docs updates

- [ ] T050 End-to-end validation: [describe observable system behavior to check]
- [ ] T051 [P] Update `docs/specs/[relevant-spec].md` if this changes a normative spec (in-place edit, do not create new file)
- [ ] T052 [P] Update `docs/operations/[relevant-runbook].md` if operational steps change
- [ ] T053 Commit: `feat([scope]): [description]` — Conventional Commits format

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Firmware)**: Depends on Phase 1 ✅
- **Phase 3 (Dashboard)**: Depends on Phase 1 ✅; can run in parallel with Phase 2
- **Phase 4 (Functions)**: Depends on Phase 1 ✅; can run in parallel with Phases 2–3
- **Phase 5 (Integration)**: Depends on ALL module phases complete

### Within Each Module Phase

- Tests MUST be written and confirmed FAILING before implementation begins
- Models / schema definitions before service logic
- Core implementation before integration points
- Each user story complete and validated before starting next priority

### Parallel Opportunities

- `[P]` tasks within a phase have no intra-phase dependencies and can run in parallel
- Dashboard and Cloud Functions phases can run in parallel once Phase 1 is done
- Firmware phase can run in parallel with dashboard/functions if teams are split

---

## Validation Summary

```bash
# Dashboard
cd dashboard && npm test
cd dashboard && npm run validate

# Cloud Functions
cd functions && npm run build && npm test

# Firmware — hardware only, document expected serial output in research.md
# pio run -e [env] --target upload && pio device monitor
```

**Pre-existing known failure**: `dashboard/__tests__/lib/usePumpData.test.tsx` — 4 tests (mock uses `set`, source uses `update`). Not a regression. Do not fix unless specifically scoped.

---

## Notes

- `[P]` = parallel-safe (no dependency on other `[P]` tasks in same phase)
- `[FW]` / `[DB]` / `[FN]` = module tag for traceability
- `[US1]` / `[US2]` = maps task to user story from spec
- Commit after each logical group, not just at phase end
- Stop at each ✅ checkpoint to validate independently before continuing
- `docs/` changes: always edit in-place; do NOT create new files for existing topics
