# Tasks: App UI/UX Redesign & System Integration

**Branch**: `feature/app-uiux-integration` | **Spec**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

**Modules**: Android App (`app/`), Firebase RTDB

---

## Phase 1: Shared Setup

**Purpose**: Any cross-module groundwork (schema design sign-off, new RTDB nodes, shared types)

- [x] T001 Review constitution gate — confirm all applicable principles pass
- [x] T002 [P] Confirm RTDB schema changes (v2) are backward compatible
- [x] T003 [P] Ensure Google Stitch design assets (logos, placeholders) are ready

**Checkpoint**: Constitution gate ✅ — implementation phases may begin

---

## Phase 2: Foundational (Android App)

**Build**: `cd app && ./gradlew assembleDebug`
**Validation**: Run on physical Android device or emulator.

**Purpose**: Core UI theming and data layer setup blocking all UI components.

- [x] T010 [APP] Add `Theme.kt`, `Color.kt`, `Type.kt` importing the Google Stitch M3 color scheme.
- [x] T011 [APP] Setup Typography: Hanken Grotesk (headlines), Inter (body), JetBrains Mono (labels).
- [x] T012 [APP] Create `DeviceRepository` interface and Firebase implementation to observe `/devices/{id}/telemetry` and `/devices/{id}/shadow/reported`.
- [x] T013 [APP] Implement write functions for `/devices/{id}/shadow/desired` and `/devices/{id}/config`.
- [x] T014 [APP] Update `DashboardViewModel` to expose `StateFlow<DashboardUiState>` consuming the repository.
- [x] T015 [APP] Initialize Firebase Anonymous Auth (`signInAnonymously()`) on app startup to secure RTDB connections (SC-003).

**Checkpoint**: App builds clean with base M3 theme; ViewModel exposes state.

---

## Phase 3: User Story 1 — Real-time Dashboard Monitoring (P1)

- [x] T020 [APP] [US1] Implement `TankLevelCard.kt` with custom `Canvas` fluid wave animation (Cyan fill, desaturated for stale data).
- [x] T021 [APP] [US1] Implement `PumpStatusCard.kt` with Emerald glowing active halo (`glowPulse`) and monospaced flow rate display.
- [x] T022 [APP] [US1] Implement `DiagnosticsCard.kt` for signal strength and uptime.

**Checkpoint**: UI Components render real-time telemetry successfully in Jetpack Compose previews.

---

## Phase 4: User Story 2 — Remote Pump Control & Overrides (P1)

- [x] T030 [APP] [US2] Implement `ControlPanel.kt` with segmented toggle (AUTO/MANUAL) with 24px radius and high-visibility Red E-Stop pill button, ensuring 48x48dp minimum touch targets.
- [x] T031 [APP] [US2] Implement `ActivityPanel.kt` (Event Log) tracking system transitions.
- [x] T032 [APP] [US2] Update `DashboardViewModel` to process mode toggles and E-Stop commands to the repository.

**Checkpoint**: Button presses correctly write to Firebase RTDB (`shadow/desired`).

---

## Phase 5: User Story 3 — Device Configuration & Thresholds (P2)

- [x] T040 [APP] [US3] Implement `ConfigBottomSheet.kt` for threshold sliders (low water, dry-run, max overflow).
- [x] T041 [APP] [US3] Update `DashboardViewModel` to process config updates to the repository.

**Checkpoint**: Sliders update Firebase RTDB (`config` node) accurately.

---

## Phase 6: Integration & Polish

**Purpose**: Screen assembly and offline behavior.

- [x] T050 [APP] Wire all components into `DashboardScreen.kt`, passing the `DashboardUiState` down.
- [x] T051 [APP] Handle offline/disconnected UI states gracefully (e.g., prominent banner instructing user of stale state).
- [x] T052 [APP] Apply Google Stitch generated logo and placeholder assets for the App Icon and Splash Screen.
- [x] T053 Commit: `feat(app): implement vNext Dashboard UI and RTDB v2 integration`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 ✅
- **Phase 3 (US1)**: Depends on Phase 2 ✅
- **Phase 4 (US2)**: Depends on Phase 2 ✅; can run in parallel with Phase 3
- **Phase 5 (US3)**: Depends on Phase 2 ✅; can run in parallel with Phases 3 and 4
- **Phase 6 (Integration)**: Depends on ALL User Story phases complete

### Parallel Opportunities

- `[P]` tasks within a phase have no intra-phase dependencies and can run in parallel.
- US1, US2, and US3 UI components can be developed in parallel once Phase 2 (Foundational Theme & State) is complete.

---

## Validation Summary

```bash
# Android App Verification
cd app && ./gradlew assembleDebug

# Firebase RTDB Rule Validation (if needed)
firebase emulators:exec "npm test"
```

## Notes

- `[P]` = parallel-safe (no dependency on other `[P]` tasks in same phase)
- `[APP]` = module tag for traceability (Android App)
- `[US1]` / `[US2]` / `[US3]` = maps task to user story from spec
- Stop at each ✅ checkpoint to validate independently before continuing

---

## Phase 8: Convergence

- [x] T042 Implement COUNTDOWN mode segmented control and duration input in `ControlPanel.kt` per FR-007, FR-008 (missing)
- [x] T043 Implement `clearError` recovery button in `PumpStatusCard.kt` or `ControlPanel.kt` per US2/AC3 (missing)
- [x] T044 Implement `bypass_level_sensor` and `bypass_flow_sensor` toggles in `ConfigBottomSheet.kt` per FR-011 (missing)
- [x] T045 Wire COUNTDOWN, clear error, and bypass actions through `DashboardViewModel.kt` to the repository (missing)
