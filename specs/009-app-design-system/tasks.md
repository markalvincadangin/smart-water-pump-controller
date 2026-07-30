# Tasks: App Branding and Design System

**Branch**: `009-app-design-system` | **Spec**: [spec.md](file:///c:/Users/markc/_Projects/micro-controller/smartflow/specs/009-app-design-system/spec.md) | **Plan**: [plan.md](file:///c:/Users/markc/_Projects/micro-controller/smartflow/specs/009-app-design-system/plan.md)

**Modules**: Android App (`app/`)

---

## Phase 1: Shared Setup & Foundational

**Purpose**: Project initialization and asset setup.

- [x] T001 [P] Review constitution gate — confirm all applicable principles pass
- [x] T002 Create `app/src/main/java/com/smartflow/ui/theme/` package directory
- [x] T003 [P] Add `docs/design/assets/logos/smartflow-icon-monochrome.svg` as vector drawable `app/src/main/res/drawable/ic_splash_logo.xml`

**Checkpoint**: Setup is complete, packages and raw assets exist.

---

## Phase 2: User Story 1 — Consistent Branding (P1) — App

**Build**: `.\gradlew.bat assembleDebug`

**Purpose**: Establish the core Material 3 design system tokens and splash screen.

- [x] T010 [US1] Create `app/src/main/java/com/smartflow/ui/theme/Color.kt` with Brand Identity semantic colors.
- [x] T011 [US1] Create `app/src/main/java/com/smartflow/ui/theme/Shape.kt` with radius design tokens.
- [x] T012 [US1] Create `app/src/main/java/com/smartflow/ui/theme/Type.kt` using Google Fonts (Downloadable Fonts) for Inter and Roboto.
- [x] T013 [US1] Create `app/src/main/java/com/smartflow/ui/theme/Theme.kt` with Light and Dark Material 3 color schemes (dynamic color explicitly disabled).
- [x] T014 [US1] Update `app/src/main/res/values/themes.xml` (and `values-night`) to configure the Android 12+ Splash Screen API using `ic_splash_logo`.
- [x] T015 [US1] Update `app/src/main/java/com/smartflow/MainActivity.kt` to call `installSplashScreen()` and wrap content in `SmartFlowTheme`.

**Checkpoint**: App compiles, splash screen appears correctly, and empty screens inherit the new theme.

---

## Phase 3: User Story 2 — Accessible UI Components (P2) — App

**Purpose**: Refactor existing screens to use the new semantic design tokens.

- [x] T020 [US2] Update `app/src/main/java/com/smartflow/ui/screens/DashboardScreen.kt` (and children components) to use `MaterialTheme.colorScheme` and `MaterialTheme.typography` instead of hardcoded colors.
- [x] T021 [US2] Ensure all interactive components have a minimum touch target of 48dp via Compose modifiers.
- [x] T022 [US2] Implement responsive Compose modifiers in `DashboardScreen.kt` to adapt the UI into a 2-column side-by-side grid when the device is rotated to landscape orientation.

**Checkpoint**: Dashboard reflects brand colors in both Light and Dark mode.

---

## Phase 4: Integration & Documentation

**Purpose**: Cross-module validation and docs updates

- [x] T030 [US3] Compile the Android App and validate that the splash screen icon is centered properly on cold boot.
- [x] T031 [US3] Commit: `feat(app): implement design system` — Conventional Commits format

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: Start immediately
- **Phase 2 (US1 - Core Branding)**: Depends on Phase 1 ✅
- **Phase 3 (US2 - Refactoring UI)**: Depends on Phase 2 ✅
- **Phase 4 (Integration)**: Depends on ALL module phases complete

### Within Each Module Phase

- Asset setup must happen before implementation.
- Core tokens (`Color`, `Shape`, `Type`) must be created before the aggregate `Theme.kt`.
- `Theme.kt` must be created before `MainActivity` and `DashboardScreen` can use it.

---

## Validation Summary

```bash
# Android App
cd app
.\gradlew.bat assembleDebug
```
