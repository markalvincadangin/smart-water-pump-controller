# WP-08 — Android UI/UX Audit

**Status:** Audit complete; implementation not yet authorized by this document
**Baseline:** `docs/smartflow-system-contract` after WP-07A–E
**Scope:** Dashboard, control panel, telemetry/status cards, configuration sheet, supporting Compose components, responsive behavior, accessibility, interaction feedback, and Material 3 alignment.

## Audit Basis

Source basis:

- SmartFlow System Contract v1.1
- Current Jetpack Compose source in the repository
- Current Android Material 3 / adaptive guidance

Android guidance currently recommends at least 48dp touch targets for interactive elements, use of window size classes for layout decisions, and appropriate Compose semantics for accessibility. Material 3 adaptive APIs are intended for UI adaptation across window configurations and device postures.

## Executive Summary

The dashboard is structurally usable and already uses Material 3 primitives, but several interaction and state-presentation decisions should be corrected before visual polish.

### Priority findings

| ID | Severity | Finding |
|---|---|---|
| UI-01 | P1 | AUTO is presented as a normal selectable operating mode even though AUTO is explicitly outside the validated MVP contract. |
| UI-02 | P1 | E-STOP inherits the same global `CommandState` as ordinary controls and can become disabled while another command is pending. |
| UI-03 | P1 | Large-screen/landscape layout uses `LocalConfiguration.screenWidthDp >= 600` rather than window size classes/window metrics. |
| UI-04 | P1 | Connection changes trigger transient snackbars and can create noisy/repetitive feedback during reconnects. |
| UI-05 | P1 | Configuration bottom sheet is not vertically scrollable and does not explicitly account for IME/keyboard space. |
| UI-06 | P1 | Connecting overlay blocks input with an empty `clickable` surface and provides weak accessibility semantics. |
| UI-07 | P2 | Telemetry/status semantics sometimes overstate the cause of unavailable or stale data. |
| UI-08 | P2 | Dashboard uses fixed card sizing and duplicated compact/expanded layout branches, creating avoidable responsive maintenance cost. |
| UI-09 | P2 | Tank level wave animation runs continuously even when telemetry is stale/unavailable. |
| UI-10 | P2 | Several labels mix contract terms such as Countdown/Timer and legacy/general wording, reducing consistency. |

## UI-01 — AUTO Must Not Look Production-Ready

**Severity: P1**

`ModeSelector` renders every `OperatingMode` entry, including `AUTO`, as an ordinary `SegmentedButton`.

The system contract explicitly states that AUTO is not MVP and must not be presented as a production-ready capability until independently validated.

### Recommended direction

For the MVP dashboard, use only MANUAL and COUNTDOWN as actionable modes. AUTO can remain represented in the domain/firmware but should either:

- be hidden from the production control selector, or
- be visibly disabled with clear future-feature wording.

Do not let a normal tap path make AUTO appear equivalent to validated MVP modes.

## UI-02 — E-STOP Must Not Share the Ordinary Command Lock

**Severity: P1**

`ControlPanel` passes one `commandState` to both ordinary command controls and `EmergencyStopButton`. `EmergencyStopButton` disables itself for `Pending` and `Accepted` states.

Therefore, while a normal command is awaiting confirmation, the remote E-STOP control can also become unavailable.

The firmware remains the ultimate safety authority and a physical E-STOP still exists, but the remote emergency action should not be unnecessarily blocked by an unrelated normal command.

### Recommended direction

Separate emergency-stop availability from the ordinary command lifecycle.

Minimum UI rule:

- ordinary command pending → ordinary controls disabled
- remote E-STOP → independently available whenever a connected/authorized control path exists
- E-STOP request pending → disable repeated E-STOP activation and show explicit stopping/interlocked feedback

This keeps emergency interaction visually and behaviorally higher priority without weakening firmware safety enforcement.

## UI-03 — Replace Raw Width Breakpoint Logic

**Severity: P1**

`DashboardScreen` decides its two-column layout with:

`LocalConfiguration.current.screenWidthDp >= 600`

This is a coarse device-width check rather than a window-aware adaptive layout decision.

Current Android guidance recommends making app-level layout decisions from the actual available window size and using window size classes rather than assuming a physical device size. This matters for landscape orientation, split-screen, foldables, desktop-style windows, and resizable large screens.

### Recommended direction

Adopt Material 3/window size class handling for the dashboard.

Suggested layout policy:

| Window class | Layout |
|---|---|
| Compact | Single-column dashboard |
| Medium | Single-column or selectively paired cards based on available width |
| Expanded+ | Supporting control/activity pane beside primary device status |

Avoid duplicating the entire dashboard content tree where only arrangement changes.

## UI-04 — Connection Feedback Is Too Transient and Potentially Noisy

**Severity: P1**

`LaunchedEffect(uiState.connectionStatus)` emits a snackbar for every transition to CONNECTED or DISCONNECTED.

This is not a reliable primary status mechanism for an IoT control screen because reconnect churn can create repeated transient messages. The dashboard already has persistent diagnostic/status information, so the snackbar should be reserved for consequential user-facing events.

### Recommended direction

Use persistent connection state in the dashboard hierarchy:

- Offline → persistent prominent status banner
- Connecting → persistent non-blocking connection state
- Connected → normal state with a subtle status indicator

Reserve snackbars for user-triggered outcomes such as rejected commands, saved configuration, or completed maintenance actions.

## UI-05 — Configuration Sheet Needs Adaptive Scrolling

**Severity: P1**

`ConfigBottomSheet` contains multiple threshold controls, maintenance overrides, reboot controls, and action buttons inside a plain `Column`.

The content is long enough to become difficult to reach on compact-height windows and landscape orientation. A text field can also trigger the software keyboard and further reduce available vertical space.

### Recommended direction

Use a vertically scrollable content container and keyboard-aware padding. Preserve the primary save/cancel actions in a predictable location where practical.

Also give the modal sheet an explicit accessibility pane title so accessibility services can identify the sheet as a distinct window-like surface.

## UI-06 — Connecting Overlay Has Weak Accessibility Semantics

**Severity: P1**

`ConnectingOverlay` uses an empty `clickable` modifier to intercept touches. This creates an interactive semantic node whose action does nothing.

That is not a good accessibility representation of a temporary blocking state.

Compose accessibility guidance supports explicit semantics for important changing UI and `paneTitle` for window-like surfaces. It also cautions against using live regions for frequently changing content such as countdown timers.

### Recommended direction

Keep the input-blocking behavior, but do not represent the blocker as a meaningless clickable action. Provide a clear semantic description such as “Connecting to SmartFlow” and expose progress/status without repeatedly announcing every update.

## UI-07 — Avoid Overstating Data Fault Causes

**Severity: P2**

`TankLevelCard` announces unavailable tank level as “Check level sensor.”

The contract distinguishes telemetry unavailable/stale state from firmware-level sensor faults. Data can be unavailable because of connectivity, mapping, or upstream availability without proving that the physical level sensor is defective.

`PumpStatusCard` also collapses several conditions into a generic “unknown due to stale data” presentation.

### Recommended direction

Use cause-neutral telemetry wording unless a machine-readable fault code or sensor-availability state explicitly identifies the cause.

Examples:

- “Tank level unavailable”
- “Tank level data is stale”
- “Level sensor unavailable” only when the device state explicitly reports that condition

## UI-08 — Reduce Layout Duplication and Fixed Card Constraints

**Severity: P2**

`DashboardScreen` maintains separate compact and expanded content trees containing the same diagnostics, tank, pump, controls, and activity sections.

`TankLevelCard` also uses a fixed 200dp height.

These choices can work for the current phone/tablet target, but they make future visual refinement and responsive behavior harder because the same hierarchy exists in multiple branches.

### Recommended direction

Keep one content model/order and vary arrangement through adaptive layout containers. Use minimum/maximum size constraints where possible instead of fixed heights for information cards.

## UI-09 — Continuous Tank Animation Is Unnecessary When Data Is Not Live

**Severity: P2**

`TankLevelCard` maintains an infinite wave animation continuously. The drawing logic disables the visible wave movement for stale/unavailable data, but the transition itself still runs.

### Recommended direction

Only run the infinite animation while the telemetry is live and the tank is between empty/full bounds. This better communicates that animation represents live telemetry rather than decoration.

## UI-10 — Terminology Needs One User-Facing Vocabulary

**Severity: P2**

Contract language uses “COUNTDOWN” for the operating mode and countdown behavior, while the UI uses “Timer mode”, “Timer active”, “STOP TIMER”, and “START TIMER”. This is understandable, but mixed terminology makes the state model less obvious.

### Recommended direction

Choose a consistent user-facing vocabulary while retaining contract identifiers internally. A reasonable UI vocabulary is:

- Mode: Manual / Countdown
- Action: Start countdown / Stop countdown
- Status: Countdown active

Do not expose internal compatibility terms such as overflow for maximum-runtime protection.

## Visual and Material 3 Assessment

### Strengths

- Material 3 `Card`, `Button`, `OutlinedButton`, `SegmentedButton`, `Slider`, and top app bar components are used instead of large amounts of custom widget code.
- Primary actionable buttons are generally 56dp high.
- The emergency action is visually separated from ordinary controls.
- The theme has distinct surface/background roles and explicit light/dark/monochrome schemes.
- Important tank and pump status cards already provide accessibility descriptions.

### Areas for polish

- Reduce the number of competing surface containers on the dashboard.
- Establish a clearer visual hierarchy between device health, telemetry, control, and history.
- Prefer one primary status statement over multiple labels that repeat the same state.
- Keep safety state visually dominant without relying only on color.
- Avoid excessive animation for telemetry that is not currently live.
- Review the dashboard at compact-height landscape as well as normal portrait.

## Accessibility Assessment

Interactive controls should maintain at least 48dp touch targets. Built-in Material controls generally provide accessible defaults, but custom controls and custom blocking surfaces still require explicit review.

Current priorities:

1. remote E-STOP independence
2. connecting overlay semantics
3. modal bottom-sheet pane semantics and scrolling
4. state communication that does not rely only on color
5. truthful stale/unavailable wording
6. TalkBack review of the entire command lifecycle

## Color/Contrast Spot Check

The current core theme colors are broadly suitable for accessible text contrast:

- `BluePrimary` on `OnBrightBrand` ≈ 6.44:1
- `GreenSecondary` on `OnBrightBrand` ≈ 7.04:1
- `WarningAmber` on `OnWarning` ≈ 6.97:1
- `CriticalAction` on white ≈ 4.83:1

These spot checks do not replace full UI contrast testing across every component/state and do not verify screenshots on real devices.

## Recommended Implementation Order

### WP-08A — Safety-Critical Interaction & State Presentation

- isolate E-STOP from global ordinary-command disable state
- prevent AUTO from appearing as an equivalent validated MVP mode
- improve fault/interlock presentation

### WP-08B — Responsive Dashboard Layout

- move from raw width checks to window size class/window-aware adaptation
- consolidate duplicated compact/expanded dashboard structure
- test portrait, landscape, split-screen, and large-window layouts

### WP-08C — Accessibility & Interaction Semantics

- fix connecting overlay semantics
- add modal pane semantics
- verify 48dp+ interactive targets
- review TalkBack labels, state announcements, and focus order

### WP-08D — Configuration UX

- make configuration sheet scroll/IME safe
- improve field validation presentation
- standardize terminology

### WP-08E — Visual Polish & Motion

- refine hierarchy and spacing
- reduce unnecessary surfaces
- optimize telemetry animation
- review light/dark/monochrome themes

## Verification Boundary

This audit is source-based. It does not establish visual correctness on physical devices.

Before closing WP-08, verify at minimum:

- portrait phone
- landscape phone
- compact-height landscape
- tablet/expanded window
- split-screen or resizable window
- light theme
- dark theme
- monochrome themes
- TalkBack/Accessibility Scanner review
- large font / display scaling
- keyboard-open configuration sheet

## External Guidance

Android Developers — Compose accessibility semantics: live regions, pane titles, and accessibility semantics.

Android Developers — API defaults: minimum 48dp touch targets and Material component accessibility defaults.

Android Developers — Adaptive apps: use window size classes and available window metrics for responsive layout decisions.

Android Developers — Compose Material 3 Adaptive: adaptive layouts for changing window configurations.