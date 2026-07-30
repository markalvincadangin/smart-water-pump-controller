# Component Library

All SmartFlow components follow a shared state model:

- Enabled
- Pressed
- Focused
- Disabled
- Loading or pending
- Offline
- Stale data
- Warning
- Critical
- Success

Components must use semantic tokens rather than hardcoded colors.

## 1. GlobalStatusBanner

**Purpose:** Highest-priority system message.

**Use for:**

- critical alarm
- degraded connectivity
- stale data affecting decisions
- local-only operation

**Rules:**

- appears at the top
- pushes content down
- includes icon, title, explanation, and action when applicable
- does not disappear until the condition clears or is explicitly dismissed when dismissal is safe

## 2. StatusChip

**Purpose:** Compact confirmed state.

**States:**

- Online / Healthy
- Warning / Degraded
- Critical / Error
- Offline / Unknown

**Rules:**

- never color-only
- includes text and optional standard icon
- static unless it represents a filter control
- dark text is used on bright blue, green, and amber fills

## 3. PumpStatusCard

**Purpose:** Primary pump state and control.

**Required content:**

- pump state
- operating mode: Auto / Manual / Disabled
- command availability
- latest command state
- connection and data freshness
- primary action

**States:**

- Idle
- Starting
- Running
- Stopping
- Error
- Offline
- Interlocked
- Maintenance

The control must not show “Running” until hardware acknowledgement confirms it.

## 4. TankLevelCard

**Purpose:** Communicate water level and threshold state.

**Required content:**

- visual level
- numeric level
- timestamp or freshness
- threshold label when abnormal

**Rules:**

- use tabular figures
- animate only between confirmed values
- stale data freezes the visual and shows a stale indicator
- warning and critical thresholds include text and icons

## 5. TelemetryChart

**Purpose:** Provide trend context.

**Required behavior:**

- clear axis and units
- time range
- threshold lines
- missing-data gaps
- accessible summary
- no smoothing that falsely implies measured data

## 6. ConnectionBanner

**Purpose:** Explain remote-control capability and data source.

**States:**

- Cloud online
- Local network only
- Bluetooth/local only
- Reconnecting
- Offline
- Data stale

Use explicit language such as: “Local control available. Cloud history is unavailable.”

## 7. CommandButton

**Purpose:** Send a hardware command.

**Lifecycle:**

1. Ready
2. Pending
3. Accepted
4. Completed

Failure states:

- Rejected
- Timed out
- Offline-blocked
- Interlock-blocked

While pending, prevent duplicate submission and show the command target.

## 8. EmergencyStopDialog

**Purpose:** Execute the highest-priority destructive control action.

**Rules:**

- use the critical-action color
- state the physical outcome
- require deliberate confirmation
- do not create a multi-step maze
- remain operable with large text and TalkBack
- provide immediate command feedback
- distinguish “command sent” from “pump confirmed stopped”

## 9. SkeletonCard

**Purpose:** Preserve layout during initial loading.

Skeletons must not be used for stale or disconnected data. In those cases, show the last known value and freshness state instead.

## Iconography

- Material Symbols Rounded
- Filled for selected or active navigation
- Outlined for inactive or secondary actions
- Standard safety symbols only
- Decorative icons receive no accessibility description
- Functional icons must have a label or content description

## Component documentation requirement

Every production component must document:

- purpose
- anatomy
- variants
- state matrix
- token usage
- behavior
- accessibility
- empty/loading/offline behavior
- screenshots in dark and light themes
