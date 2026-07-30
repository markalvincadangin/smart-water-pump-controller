---
status: draft
version: 0.1
last-reviewed: 
source: auto-generated
---

# Feature Specification: App Branding and Design System

**Feature Branch**: `009-app-design-system`

**Created**: 2026-07-30

**Status**: Draft

**Input**: User description: "now we will work next on the branding for the app and its designsystem"

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | No | UI/Theming change only. |
| II. Dry-Run Lockout | No | UI/Theming change only. |
| III. Overflow Protection | No | UI/Theming change only. |
| IV. TOR Independence | No | UI/Theming change only. |
| V. Sensor Freshness / E-Stop | No | UI/Theming change only. |
| VI. Backward Compatibility | No | UI/Theming change only. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Consistent Branding (Priority: P1)

As a user, I want the SmartFlow app to have a consistent and modern look and feel across all screens so that the application feels trustworthy and professional.

**Why this priority**: A cohesive design system is the foundation for all future UI work.

**Independent Test**: Can be fully tested by navigating through the app and observing that colors, typography, and shapes follow a standardized theme (e.g. Material 3).

**Acceptance Scenarios**:

1. **Given** the app is launched in light mode, **When** viewing the dashboard, **Then** the primary brand colors and typography are applied correctly.
2. **Given** the app is launched in dark mode, **When** viewing the dashboard, **Then** the dark theme colors are applied seamlessly without legibility issues.

---

### User Story 2 - Accessible UI Components (Priority: P2)

As a user, I want UI components (buttons, cards, text) to have proper contrast and sizing so that the app is easy to read and interact with in various lighting conditions.

**Why this priority**: Legibility is critical for an IoT app used in potentially bright (outdoors) or dim (pump room) environments.

**Independent Test**: Can be fully tested by verifying contrast ratios and touch target sizes.

**Acceptance Scenarios**:

1. **Given** a critical action button, **When** displayed on screen, **Then** it has a minimum touch target size of 48x48dp and meets WCAG AA contrast ratios.

---

## Clarifications

### Session 2026-07-30
- Q: Should we enable Material You dynamic colors (Android 12+) or strictly enforce brand colors? → A: Strictly enforce SmartFlow brand colors everywhere to ensure brand consistency.
- Q: Should we implement the Android 12+ Splash Screen API using the new standalone icon right now, or defer it? → A: Implement the Android 12+ Splash Screen API using the new standalone icon in this feature branch.
- Q: The spec requires Inter and Roboto typefaces. Should we bundle the raw font files directly in the APK, or use Google Fonts (Downloadable Fonts) via Compose? → A: Use Google Fonts (Downloadable Fonts) in Compose to reduce APK size, with standard Android system fonts as a fallback.
- Q: How should the app's UI adapt when the device is rotated to landscape orientation? → A: Use a Side-by-Side Grid (e.g. 2-column layout) to utilize horizontal space efficiently and prevent UI elements from stretching too wide.

- Q: How should we implement the Android app icons and splash logo given the raster PNG assets in `docs/design/assets/logos/android/`? → A: Use the provided `ic_launcher_foreground.png` and `ic_launcher_background.png` directly for adaptive icons and the splash screen.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The app MUST implement a standardized color palette including Primary, Secondary, Background, Surface, and Error colors for both Light and Dark themes.
- **FR-002**: The app MUST strictly enforce the SmartFlow Brand Identity colors (Primary Blue `#0EA5E9`, Secondary Green `#10B981`, Background `#0F172A`, Accent White `#FFFFFF`) everywhere. Material You dynamic colors must be explicitly disabled to ensure brand consistency.
- **FR-003**: The app MUST define a standardized typography scale (e.g., standardizing on a specific font family or using default Roboto with specific weights/sizes).
- **FR-004**: The app MUST use Inter as the primary typeface and Roboto as the secondary typeface, loading them dynamically via Compose Downloadable Fonts (Google Fonts provider) to minimize APK size, falling back to system default fonts if offline.
- **FR-005**: All existing Compose screens MUST be updated to use the new `SmartFlowTheme` design system components instead of hardcoded colors or styles.
- **FR-006**: The app MUST support dynamic screen orientation. In landscape mode, the UI MUST adapt into a 2-column Side-by-Side Grid layout using responsive Compose modifiers to prevent elements from stretching unnecessarily wide.
- **FR-007**: The app MUST implement the `androidx.core:core-splashscreen` API utilizing the new standalone brand icon and background colors for a cohesive launch experience.

### Key Entities

- **Design System Theme**: The centralized Kotlin object/file (e.g., `Theme.kt`, `Color.kt`, `Type.kt`) defining the Material 3 specifications.

---

## Dashboard UX *(if applicable)*

### User-Facing Changes
- `MainActivity.kt` and all Compose UI elements will be wrapped in a unified `SmartFlowTheme`.
- Buttons, Cards, and Text elements will visually change to match the new branding.

### Offline / PWA Behavior
- No changes to offline behavior.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of hardcoded colors in Compose files are replaced with theme references.
- **SC-002**: The app successfully compiles and launches with the new theme applied to both light and dark system settings.
- **SC-003**: All interactive elements meet a minimum touch target size of 48x48dp.

### Validation Commands

```bash
# Android App
.\gradlew.bat assembleDebug
```

---

## Assumptions

- The app uses Jetpack Compose for UI.
- The design system will be based on Material Design 3 guidelines.
