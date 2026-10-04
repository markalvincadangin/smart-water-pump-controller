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


## Runtime Visual Evidence

**Evidence source:** user-provided screenshots captured from the running Android application.

The screenshots are treated as runtime UX evidence rather than as a replacement for source inspection. They confirm several source-level findings and expose additional state-presentation issues that are difficult to judge from code alone.

### Evidence A — Device list / Manage Device

The device-list screenshot shows a large amount of unused vertical space below the device card, while the bottom navigation and floating action button occupy a strong visual area. This supports the broader hierarchy/layout review but is not itself a functional defect.

The Manage Device screenshot shows two large explanatory cards for Wi-Fi configuration and ownership transfer. The copy is useful, but the page is information-dense and would benefit from stronger grouping between routine device configuration and high-impact ownership operations.

The ownership transfer area also places a disabled “Start transfer” action directly among active actions. The disabled state is visible, but its reason is not immediately apparent from the button alone. The validation/error explanation should remain close to the action when the user cannot proceed.

### Evidence B — Device Configuration

The configuration screenshot confirms the compact-height/scrollability concern identified as UI-05.

The visible content reaches the Maximum Pump Runtime description near the bottom edge, with the explanatory text visibly clipped by the viewport. This demonstrates that the configuration surface needs adaptive vertical scrolling rather than relying on a fixed-height presentation.

The configuration screen also has a large amount of vertical spacing between controls. Once scrolling is introduced, spacing can be tightened selectively so users can review the complete safety configuration without excessive travel.

The current labels themselves are substantially aligned with the canonical contract after WP-07C:

- Pump Start Level
- Pump Stop Level
- Dry-Run Threshold
- Maximum Pump Runtime

This is an improvement over the previously identified legacy “Max Overflow” wording.

### Evidence C — Main Dashboard

The dashboard screenshot confirms that AUTO is visibly exposed as a normal selectable mode. This elevates UI-01 from a source-level concern to a directly observed runtime issue.

The screenshot also shows a potentially confusing combination of states:

- `Manual` remains visually selected
- `Switching to Auto...` is displayed
- `STARTING PUMP...` and `Stopping...` appear as disabled/pending controls
- `Awaiting device confirmation` is displayed

These elements demonstrate that the command lifecycle is present, but the hierarchy between **desired state**, **pending command**, and **reported physical state** is not yet sufficiently explicit.

## UI-11 — Desired, Pending, and Reported State Need Clearer Visual Separation

**Severity: P1**

Runtime evidence shows that a user can see a selected mode, a mode-transition message, pending command controls, and a device-confirmation message at the same time.

SmartFlow's contract intentionally separates:

1. user intent / desired state,
2. command pending,
3. reported device state,
4. completed or rejected outcome.

The UI should preserve that distinction rather than allowing multiple independent indicators to compete for attention.

### Recommended direction

Use a clear hierarchy:

**Primary state**

- Pump Running
- Pump Stopped
- Safety Interlocked
- Fault

**Secondary device state**

- Manual
- Countdown — remaining time
- Connected / Offline / Data stale

**Transient command state**

- Starting pump…
- Stopping pump…
- Switching to Countdown…
- Awaiting device confirmation

The transient state should be visually subordinate to the authoritative reported state until the device confirms the requested change.

A pending command should not imply that the physical pump has already changed state.

### Acceptance criteria

- A pending command never visually masquerades as confirmed physical state.
- Reported pump state remains the authoritative primary status.
- Mode transition feedback identifies the requested transition without replacing reported mode.
- Safety interlock/fault state overrides ordinary pending-command messaging.
- E-STOP remains independently available according to UI-02.
- AUTO cannot be presented as a normal validated MVP operation.

## Runtime Evidence Summary

| Evidence | Source-level finding confirmed | New insight |
|---|---|---|
| AUTO selector visible | UI-01 | Confirms the MVP boundary is visible to users, not merely present in code |
| `Switching to Auto...` while Manual remains selected | Command lifecycle review | Desired/pending/reported hierarchy needs stronger visual separation |
| Disabled starting/stopping controls + confirmation message | WP-07B lifecycle implementation | Pending state needs clearer contextual grouping |
| Configuration content clipped near bottom | UI-05 | Scroll/height problem is observable on a real compact-height viewport |
| Tank `--` / `Unavailable` | UI-07 | Unavailable telemetry should remain cause-neutral |
| Large card-based dashboard surfaces | UI-08 / visual polish | Hierarchy can be simplified after safety/state issues are fixed |
| Manage Device long explanatory cards | General visual review | Routine and high-impact device-management actions should be more clearly grouped |

## Runtime Evidence Handling

The supplied screenshots are evidence for this audit but are **not recommended as-is for the public repository**. They contain a visible device identifier and should be redacted/cropped before becoming public documentation assets.

For the eventual README/portfolio documentation, use sanitized screenshots with:

- device IDs replaced by non-identifying examples,
- personal/status information removed where unnecessary,
- consistent screenshot framing,
- representative connected and offline states,
- one configuration example,
- one command-state example.

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