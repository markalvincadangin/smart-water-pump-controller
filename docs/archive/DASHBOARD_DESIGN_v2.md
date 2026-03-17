> **Superseded by:** [./releases/v2.0/dashboard-ux-spec.md](./releases/v2.0/dashboard-ux-spec.md) — canonical v2 dashboard UX spec.

---

# Dashboard Design Review & Improvement
## Smart Water Pump Controller — UI/UX Audit v1.0

| Field | Value |
|---|---|
| **Subject** | Current `dashboard/` implementation |
| **Reference spec** | FIRMWARE_DASHBOARD_DESIGN_v2.md |
| **UX frameworks applied** | Nielsen's 10 Usability Heuristics, Fitts's Law, Hick's Law, Mental Model alignment, Progressive Disclosure, WCAG 2.1 AA, Safety-Critical HCI principles |
| **Date** | March 2026 |

---

## Part 1 — Audit of Current Implementation

### A. What Is Working Well

Before listing issues, these areas are correctly implemented and should be preserved:

- **Alert ranking (8-item ordered list)** is accurate and matches the v2 spec. Red/amber/blue severity tiers are distinct. This is the strongest part of the current design.
- **`countdown_add_time` roundtrip-confirmed disable** (`isAddingCountdownTime` cleared only when Firebase control reflects `false`) — correctly implemented per v2 §3.
- **One-shot 5-second reset** for `manual_start` / `manual_stop` — correctly implemented.
- **8-second optimistic timeout** — present in both `RunControls` and `usePendingControl`.
- **Emergency Override confirmation dialog** with checkbox and guidance copy — present and correct per v2 §6.2.
- **`AUTO_STANDBY` rendering** as `"AUTO (Standby)"` in Pump Status stat card — correct.
- **Stop button minimum 64px** with high-contrast red — correct per v2 §11.5 / Fitts's Law.
- **Touch target floor of 48px** on Quick Start / Countdown / Add Time — correct.
- **Activity log prefers `detail` field** when present — correct per audit plan.

---

### B. Issues Found

Issues are categorized by type and severity. Each cites the UX principle being violated.

---

#### B1 — Layout: Layer 1 is not always scroll-free on mobile 🔴 Critical

**Current behavior:** On mobile, the layout stacks `StatusBar → DashboardHeader → Banners/alerts → Tank card → Stats`. The `DashboardHeader` includes the system name, user email, admin badge, and four action buttons (Device settings, Notifications, Restart, Sign out). On a typical 375px mobile viewport, the header alone consumes ~120px and the alert banners stack below it. The tank card — Layer 1 primary visual — can easily push below the fold when one or more alerts are active.

**Principle violated:** Nielsen Heuristic #1 (Visibility of System Status). The most important element (current tank/pump state) must always be visible without scrolling. A user who opens the app during a dry-run error should see the tank state and the lockout simultaneously, not have to scroll to confirm the tank is not filling.

**Improvement:** The header must be reduced to its minimum footprint on mobile. The system name and connectivity status belong in a slim fixed bar (56px). The action buttons (Device settings, Notifications, Restart, Sign out) should move to a hamburger/overflow menu revealed by a single icon tap. The user email and admin badge are diagnostic — they belong in the overflow menu, not the primary header. This recovers ~60px of above-fold space.

**Header before (mobile):**
```
[System name]  [email + admin badge]  [Settings] [Notif] [Restart] [Sign out]
```

**Header after (mobile):**
```
[System name + online dot]                                    [⋮ Menu]
```

---

#### B2 — Layout: Stat cards and tank card compete for primary attention 🟡 Significant

**Current behavior:** `DashboardMainGrid` places the tank card and three stat cards side-by-side at the same visual weight:
```
[Tank card]  [Level %]
             [Flow LPM]
             [Pump Status]
```

**Principle violated:** Visual hierarchy (Gestalt: Figure/Ground). The tank visual should be the unambiguous primary figure. The three stat cards are supporting data — they give precision to what the tank already shows visually. When they're equal size and weight, the eye has no natural reading path.

**Improvement:** On mobile, the tank card should be full-width and the three stats should collapse into a single compact horizontal strip beneath it, not a vertical column beside it. On desktop, the tank can occupy the left column at 2× the stat card width. The stat cards gain visual subordination to the tank without losing their data.

**Mobile layout change:**
```
BEFORE:
[Tank (half width)]  [Level %    ]
                     [Flow LPM   ]
                     [Pump Status]

AFTER:
[      Tank (full width)        ]
[ Level % ] [ Flow LPM ] [ Mode ]
```

---

#### B3 — Controls: Run controls and mode controls are in the same card with no visual distinction 🟡 Significant

**Current behavior:** `RunControls` and `ModeControls` are described as being in the same "Run + Mode controls card spanning width." Run controls (Quick Start, Countdown, Stop) and mode controls (AUTO, FORCE_OFF, FORCE_ON) answer different questions: "What do I want the pump to do right now?" vs "What rule should govern the pump?" Grouping them in one undifferentiated card increases cognitive load.

**Principle violated:** Hick's Law — the time to make a decision increases with the number and complexity of choices. When a user wants to stop the pump quickly, they should not have to parse both run actions and policy modes simultaneously.

**Improvement:** Separate into two distinct, visually differentiated sub-sections within the controls layer:

1. **"Run Pump" panel** — contextual action buttons (Quick Start, Countdown, Stop). This answers: "Do something now." Full-width, primary visual weight.
2. **"System Mode" panel** — policy mode selector (AUTO, FORCE_OFF, FORCE_ON). This answers: "How should the system behave?" Smaller, secondary visual weight. Rendered as a segmented control (pill-style) rather than separate buttons, which takes less space and makes mutual exclusivity visually obvious.

The Stop button always appears in the "Run Pump" panel, never in "System Mode." Emergency Stop (FORCE_OFF via mode) appears only in System Mode.

---

#### B4 — Controls: `FORCE_OFF` label is used for two different intents 🟡 Significant

**Current behavior:** The Mode Controls section uses `FORCE_OFF` as both "Emergency Stop / Resume AUTO" depending on context. The same button changes label based on current state. The v2 design spec states: "FORCE_OFF — Used as emergency stop and to block new runs until mode is changed."

**Principle violated:** Nielsen Heuristic #4 (Consistency and Standards) and Mental Model alignment. Users expect a button that is visually stable to perform a stable action. A button that alternates between "Stop Everything" and something else creates a double-take moment — especially dangerous in a stress scenario.

**Improvement:** Split `FORCE_OFF` into two contextually shown, distinctly named buttons:

- When pump is running or mode is not `FORCE_OFF`: show **"Emergency Stop"** (red, icon: ⏹) — this sets `FORCE_OFF`.
- When mode is `FORCE_OFF`: show **"Resume AUTO"** (blue/neutral, icon: ▶) — this sets `AUTO`.

These two states never appear simultaneously, so there is no redundancy. The user always sees exactly one clear action with no label ambiguity.

---

#### B5 — TankVisual: Glow color is redundant with alert banners and creates false urgency 🟡 Significant

**Current behavior:** The `TankVisual` component applies:
- Red glow when `isError`
- Green glow when running
- Cyan glow when idle

The red glow on `isError` duplicates what is already communicated by the red alert banner above. But more critically, when the tank is idle (no pump running, no error), the `cyan glow` creates a visual pulse that implies activity or urgency where there is none.

**Principle violated:** Nielsen Heuristic #8 (Aesthetic and Minimalist Design) — every visual element that does not add information adds noise. The cyan idle glow is decorative noise that dilutes the meaning of the green "running" glow.

**Improvement:** Reduce to two meaningful glow states only:
- **Green (animated pulse)** — pump is actively running, water is filling.
- **Red (static)** — error/lockout, pump is off due to fault.
- **No glow** — any other state (standby, stopped, sleeping).

The tank fill level animation is sufficient to communicate the fill state. The glow should signal only anomalous or active states, not baseline operation.

---

#### B6 — Restart feedback: "Restarting…" and "Waiting…" messages use generic text 🟡 Moderate

**Current behavior:** During reboot, the banner shows:
1. "Controller restarting…"
2. "Waiting for controller to come back…"

**Principle violated:** Nielsen Heuristic #1 (Visibility of System Status) — feedback should be informative, not just confirmatory. "Waiting…" tells the user nothing about expected duration or what to do if it doesn't come back.

**Improvement:** Add time-based context:
1. `0–10s`: "Controller restarting… (usually takes 10–20 seconds)"
2. `10–30s`: "Waiting for controller… (Xs elapsed)"
3. `>30s`: "Controller hasn't responded yet (Xs elapsed). If it doesn't reconnect, try a manual power cycle."

This uses **progressive disclosure** to add information as the situation evolves, without cluttering the initial state.

---

#### B7 — StatusBar: Online users count is always 0 and wastes space 🟡 Moderate

**Current behavior:** The StatusBar shows "online users count (currently always 0; presence is a no-op)."

**Principle violated:** Nielsen Heuristic #8 (Minimalism). A UI element that always shows a meaningless value (0 users) actively misleads — a user might wonder if it means the controller counts as a user, or why it always shows 0, or whether they are even connected. Dead UI real estate erodes trust in the system's accuracy.

**Improvement:** Remove the online users count entirely. Per the v2 design, `presence/` was removed from the RTDB layout. The StatusBar has more valuable things to show in that space: sensor health score bar, or level data age badge when `level_last_valid_age_sec > 30`.

---

#### B8 — DeviceConfigSettings: Six-tab modal has no save-state warning 🟡 Moderate

**Current behavior:** The modal has six tabs (Tank Calibration, Pump Thresholds, Dry-Run, Schedule, Advanced, Maintenance). It is not specified whether changes on one tab are preserved when the user switches to another tab before saving, or whether closing the modal without saving warns the user.

**Principle violated:** Nielsen Heuristic #5 (Error Prevention). If a user edits the start level on Pump Thresholds, switches to Schedule to check the sleep hours, then accidentally closes the modal — their pump threshold edit is silently lost. This is particularly dangerous on mobile where a back gesture can close a modal.

**Improvement:**
- `isDirty` (already in `useDeviceConfig`) should gate modal dismissal: if `isDirty`, show a confirmation: "You have unsaved changes. Discard and close?"
- The Save button should be visually prominent and persistent across all tabs — not only visible on the active tab. A sticky footer with Save / Discard buttons is the correct pattern.
- Tab switching with unsaved changes: preserve dirty state across tabs (already implied by a single `isDirty` flag for the whole form, but should be confirmed in implementation).

---

#### B9 — Estimated level display has no clear visual differentiation from real level 🟡 Moderate

**Current behavior:** `water_level_percent` is the primary level display. When `level_estimate_active` is `true`, the v2 spec says "Dashboard should label the level display as 'Estimated'." The current dashboard description does not mention any visual distinction between real and estimated readings — only the subtext in the Level stat card.

**Principle violated:** Mental model alignment — users learn to trust the level reading. When it silently switches to an estimate, they are acting on a number with ±5–10% accuracy and don't know it. Especially dangerous in bypass mode where the estimate is the only overflow protection.

**Improvement:** When `level_estimate_active` is `true`:
- Replace the level `%` value color with amber (not the default cyan/green).
- Add a `~` prefix to the number: `~73%` (the tilde is universally understood as "approximately").
- In the TankVisual, add a dashed border or hatching pattern to the fill level instead of a solid fill — this communicates "approximate" without text.
- In the stat card subtext: "~ Estimated from flow sensor (±5%)"

---

#### B10 — History chart has no labeled Y-axis context for non-technical users 🟢 Minor

**Current behavior:** The history section shows "level and flow chart using history (last ~3 minutes at 3s resolution)." No axis labels or range context is described.

**Principle violated:** Nielsen Heuristic #6 (Recognition over Recall) — users should not have to remember what the scale means. A chart showing a line between 0–100 on the Y-axis is meaningful only if the user knows that 30% = start threshold and 100% = full.

**Improvement:** Add two horizontal reference lines to the level chart:
- Dashed amber line at `pump_start_level` (e.g. 30%) — labeled "Start threshold"
- Dashed green line at `pump_stop_level` (e.g. 100%) — labeled "Full"

This turns the chart from a raw data display into an interpretable operational context. The lines require `DeviceConfig` values which are already loaded in the dashboard.

---

#### B11 — No loading skeleton on initial page load 🟢 Minor

**Current behavior:** Not specified. On initial page load, the dashboard either shows empty/null state or blocks render until data arrives.

**Principle violated:** Nielsen Heuristic #1 (Visibility of System Status) — the user should always know the system is working. A blank page or layout shift while Firebase data loads creates uncertainty about whether the app is functional.

**Improvement:** Add skeleton screens for Layer 1 and Layer 3 that render while `status === null`. The tank visual should show an empty tank outline (not a filled state) with a loading shimmer. The stat cards show placeholder bars. This communicates "loading" without implying a particular system state.

---

## Part 2 — Improved Design Document

The sections below supersede the original "Dashboard Design & Layout" document. All improvements from Part 1 are incorporated. Changes from the original are annotated with `[IMP Bx]` referencing the issue above.

---

# Dashboard Design & Layout v2.0
## Smart Water Pump Controller — Next.js PWA

---

### High-Level Goals

- **Layered information architecture**: System state → Alerts → Controls → Diagnostics. The most critical layer is always visible without scrolling.
- **Mobile-first**: Single-column portrait layout as the primary design surface. Desktop layout is an enhancement.
- **Safety-first**: Clear error and lockout states with explicit, one-tap recovery paths. No ambiguous button labels under stress.
- **Clarity over density**: Each UI element earns its space. Decorative or redundant elements are removed.

---

### Page Structure (`app/page.tsx`)

`DashboardPage` is wrapped in `AuthGuard` and renders:

```
<AuthGuard>
  <SlimStatusBar />               ← Fixed, 56px, mobile-optimized [IMP B1]
  <DashboardHeader />             ← Reduced: name + overflow menu [IMP B1]
  <main>
    <OfflineBanner />             ← Dashboard/Firebase disconnected
    <RestartFeedback />           ← Progressive reboot messaging [IMP B6]
    <AlertBanners />              ← Ranked, severity-tiered [confirmed correct]
    <DashboardMainGrid />         ← Layer 1 full-width tank, Layer 3 controls [IMP B2, B3]
    <DiagnosticsSection />        ← Layer 4: collapsible on mobile
  </main>
  <Modals>
    <DeviceConfigSettings />      ← With sticky Save footer + unsaved-changes guard [IMP B8]
    <NotificationSettings />
    <InstallPrompt />
    <OverflowMenu />              ← New: houses header actions on mobile [IMP B1]
  </Modals>
</AuthGuard>
```

---

### Layer 1 — System State

#### `SlimStatusBar` (`components/StatusBar.tsx`) [IMP B1]

Fixed top bar, maximum 56px height on all screen sizes.

**Left side — Connectivity:**
- **Dashboard offline**: Red dot + `Offline` label (2 words max).
- **ESP32 online**: Green dot + WiFi icon + RSSI (color-coded: green ≥ -70dBm, amber -70 to -85dBm, red < -85dBm). Uptime abbreviation (e.g. `3d 2h`) when > 1 day.
- **Controller offline**: Amber dot + `Controller offline · Xs ago`.

**Right side — Mode + time:**
- Current policy `mode` badge (pill: AUTO / FORCE_OFF / FORCE_ON / COUNTDOWN) — color-coded per §Visual State Model.
- Sensor health icon: level sensor health dot (hidden when healthy, amber when degraded, red when failed). Flow sensor stuck icon (hidden when OK).
- `updatedAt` age: hidden when < 5s; `Xs` when 5–60s; `Xm` when > 60s.

**Removed from StatusBar** [IMP B7]:
- Online users count (presence removed per v2 design).
- User email and admin badge (moved to overflow menu).

#### `DashboardHeader` (`components/DashboardHeader.tsx`) [IMP B1]

Slim header below StatusBar. Mobile shows only system name and overflow menu icon. Desktop can expand to show user email.

```
Mobile:   [ Smart Pump Controller ]            [ ⋮ ]
Desktop:  [ Smart Pump Controller ]  [ user@email.com  Admin ▾ ]  [Settings] [Notif] [Restart] [Sign out]
```

**Overflow menu (mobile only):**
- Device settings
- Notifications
- Restart controller (admin only — grayed with tooltip if not admin)
- Sign out
- Current user email + admin badge (informational)

#### `DashboardMainGrid` (`components/DashboardMainGrid.tsx`) [IMP B2]

The core visual panel. Layout differs by screen size.

**Mobile (single column):**
```
┌───────────────────────────────────────┐
│                                       │
│         TankVisual (full width)       │  min-height: 240px
│         Level % + mode badge          │
│         (animated fill)               │
│                                       │
├──────────────┬──────────────┬─────────┤
│  Level %     │  Flow LPM    │  Mode   │  Compact stat strip, 56px tall
└──────────────┴──────────────┴─────────┘
```

**Desktop (3-column grid):**
```
┌─────────────────────┬──────────────┬───────────────────────────┐
│                     │  Level %     │                           │
│   TankVisual        │  Flow LPM    │   Run + Mode Controls     │
│   (2 cols wide)     │  Pump Status │   (full height)           │
│                     │              │                           │
└─────────────────────┴──────────────┴───────────────────────────┘
```

**`TankVisual` glow states** [IMP B5]:
- **Green animated pulse** — pump is actively running.
- **Red static glow** — error/lockout.
- **No glow** — standby, stopped, sleeping, maintenance. (Cyan idle glow removed — it was decorative noise.)

**Level display when `level_estimate_active`** [IMP B9]:
- Level value prefixed with `~` (e.g. `~73%`).
- Value color: amber instead of default cyan.
- Tank fill uses dashed/hatched outline instead of solid fill.
- Stat card subtext: `~ Estimated from flow sensor (±5%)`.

**Stat strip (3 cards):**

1. **Tank Level**
   - Value: `XX%` (or `~XX%` if estimated) [IMP B9]
   - Color: amber ≤ start threshold; cyan otherwise; amber if estimated.
   - Subtext: `Starts at ≤X% · Stops at ≥Y%` from DeviceConfig.

2. **Flow Rate**
   - Value: `X.X LPM`
   - Color: red when pump running and flow < `dry_run_threshold_lpm`; green when pump running and flow normal; dim gray when pump off.
   - Subtext: `⚠ Low flow — lockout in Xs` (countdown from dry_run_timeout_sec) when red; `Normal` when green; `Pump idle` when dim.

3. **Pump Status**
   - Value: `ERROR` / `ON` / `OFF`
   - Color: red / green / muted.
   - Subtext: `Policy: {mode} · Run: {run_mode_label}` where `AUTO_STANDBY` → `AUTO (Standby)`.

---

### Layer 2 — Alerts & Banners

#### Offline / Restart Banners

**Dashboard offline (`!connected && error`):**
- Red card at top of `<main>`, above all other content.
- Title: **"Dashboard Offline"**
- Body: "Cannot reach the cloud service. {error message}."
- No action button — this is a network condition, not a user-resolvable error.

**Restart feedback** [IMP B6]:
- Phase 1 (0–10s): Cyan banner with spinner. "Controller restarting… (usually completes in 10–20 seconds)"
- Phase 2 (10–30s): Cyan banner with elapsed counter. "Waiting for controller… (Xs elapsed)"
- Phase 3 (> 30s): Amber banner. "Controller hasn't responded yet (Xs elapsed). If it doesn't reconnect in the next 30 seconds, try a manual power cycle."
- Auto-clears and toasts success when fresh status resumes with `uptime_minutes` near 0.

#### Ranked Alert Cards (`lib/alertRanking.ts`)

`getRankedAlerts(status, esp32Online)` returns alerts in strict severity order. Rendered as stacked cards between header and main grid.

| Rank | Condition | Severity | Title | Description | Action |
|---|---|---|---|---|---|
| 1 | `!esp32Online` | 🔴 Red | Controller Offline | "No update received in Xs." | — (informational) |
| 2 | `is_error` | 🔴 Red | Dry-Run Lockout | "No water flow detected for 30s. Pump stopped." | "Clear Error" |
| 3 | `is_overflow_error` | 🔴 Red | Max Runtime Exceeded | "Pump ran for {max}min without reaching full. Check tank sensor." | "Clear Error" |
| 4 | `auto_bypass_active` | 🟡 Amber | Auto-Maintenance Active | "Level sensor offline. System switched to flow-only mode." | "View Diagnostics" |
| 5 | `bypass_level_sensor && !auto_bypass_active` | 🟡 Amber | Maintenance Mode | "Level sensor bypassed. Auto-fill paused." | "Disable Bypass" (admin) |
| 6 | `is_level_sensor_error` | 🟡 Amber | Level Sensor Offline | "Ultrasonic sensor not responding. Level data may be stale." | "Enable Bypass" (admin) |
| 7 | `is_flow_sensor_error` | 🟡 Amber | Flow Sensor Abnormal | "Flow reported while pump is off. Sensor may be stuck." | "View Diagnostics" |
| 8 | `is_sleeping` | 🔵 Blue | Sleep Mode Active | "Auto-fill paused until {resume_time}." | — |

**Visual treatments:**
- **Red cards**: Full-width, solid left border 4px red, red-tinted background, high-contrast title.
- **Amber cards**: Solid left border 4px amber, amber-tinted background.
- **Blue card**: Subtle, smaller height, lighter background. Not alarming — sleep is expected.
- All cards: Icon (left) + Title (bold) + Description (regular) + Action button (right-aligned, minimum 44px touch target).

**Stale data dimming [IMP B11 ref]:**
When `esp32Online` is false OR `level_last_valid_age_sec > 20`, all stat values in Layer 1 are visually dimmed (opacity 0.5) and show a `"Last seen Xs ago"` label beneath.

---

### Layer 3 — Controls

Controls are split into two visually distinct, clearly labeled sub-sections. Run actions and policy modes are never mixed in the same visual group. [IMP B3]

#### 3.1 Run Controls (`components/RunControls.tsx`)

**Header:** `Run Pump` + state badge (shows active `run_mode` label + countdown ring for COUNTDOWN).

**Contextual display** — only valid actions shown per current state:

| State | Buttons shown |
|---|---|
| AUTO Standby / FORCE_OFF | Quick Start, Start Countdown |
| FORCE_ON (Emergency Override) | Emergency Override active notice (see §3.3) |
| Manual Running | **STOP** (full-width red, 64px min) |
| Countdown Running | **STOP** (full-width red, 64px min), Add 5 min, countdown display |
| Error active | Section shows fault message + "Clear Error" only; run buttons hidden |
| Controller offline | All buttons disabled, "Controller offline" overlay |

**Quick Start button:**
- Label: "Quick Start (Manual)"
- Min size: 56×48px
- Optimistic: immediately shows spinner + "Starting…" badge
- 8s timeout → "Command timed out" toast

**Countdown button:**
- Opens a duration picker (preset options: 5 min, 10 min, 15 min, 30 min, 60 min, Custom)
- Selected duration shown on button after selection
- On confirm: `startCountdown(minutes)` — optimistic "Starting countdown…"

**Add 5 min button:**
- Visible only when `run_mode === "COUNTDOWN"`
- Min size: 48×48px
- Disabled (`isAddingCountdownTime`) until Firebase confirms `countdown_add_time = false`
- Label during pending: "Adding…"

**Stop button:**
- Min size: full-width × 64px
- Background: solid red, white text, no border-radius (rectangular = urgency)
- Label: "STOP"
- Shows in: Manual Running and Countdown Running states only
- On tap: `stopRun()` → reverts to AUTO [FIX A4]
- 8s optimistic: "Stopping…" overlay on button; rollback if unconfirmed

#### 3.2 Mode Controls (`components/ModeControls.tsx`) [IMP B4]

**Header:** `System Mode`

Rendered as a segmented pill control (3 options). Takes less space than 3 separate buttons and makes mutual exclusivity visually obvious.

```
[ ▶ AUTO ]  [ ⏹ Stopped ]  [ ⚡ Override (Admin) ]
```

- **AUTO**: Blue fill when active. Sets `mode = "AUTO"`. Available to all users.
- **Stopped** (FORCE_OFF): Gray fill when active. Label context: [IMP B4]
  - When mode is not FORCE_OFF: shows **"Emergency Stop"** with a stop icon.
  - When mode is FORCE_OFF: shows **"Stopped — tap AUTO to resume"** (inline nudge, not a button label change — the button remains labeled "Stopped").
  - After tapping Emergency Stop, shows inline note below the control: *"Pump stopped. Tap AUTO above to return to automatic mode."* [IMP B3, Issue 3 from plan review]
- **Override** (FORCE_ON): Admin-only. Red/warning fill when active. Disabled with tooltip "Admin access required" for non-admin users.

**Pending state:**
- When `pendingMode` is set, the target segment shows a spinner inside the pill.
- 8s → `pendingMode` clears + "Command timed out" toast.

#### 3.3 Emergency Override Flow [FIX B1]

When an admin taps the Emergency Override segment:

**Step 1 — Pre-activation confirmation dialog:**
```
┌─────────────────────────────────┐
│  ⚠ Emergency Override           │
│                                 │
│  This mode runs the pump        │
│  regardless of tank level.      │
│  The dry-run guard remains       │
│  active.                        │
│                                 │
│  To stop: use Emergency Stop    │
│  in System Mode below.          │
│                                 │
│  ☐ I understand — stop          │
│    requires Emergency Stop      │
│                                 │
│  [Cancel]        [Activate ▶]  │
│  (Activate disabled until ☑)    │
└─────────────────────────────────┘
```

**Step 2 — While FORCE_ON is active:**
- Mode segment shows "Override" with pulsing red indicator.
- Below ModeControls, prominent red banner:
  > ⚡ **Emergency Override is active.** Tank level is not being monitored. Use Emergency Stop (the "Stopped" segment) to stop the pump.
- In RunControls: hides Quick Start and Countdown. Shows notice: *"Pump running in Emergency Override. Use Emergency Stop in System Mode below to stop."*

**Step 3 — After Emergency Stop (FORCE_OFF):**
- Banner changes to amber:
  > ⏹ **Pump stopped.** Tap AUTO in System Mode to return to automatic operation.
- Banner auto-dismisses when mode changes to AUTO.

---

### Layer 4 — Diagnostics

Collapsed by default on mobile. Expanded by default on desktop. [v2 §11.1]

#### History Section (`components/DashboardHistorySection.tsx`)

- Level chart and flow chart for the last ~3 minutes (3s resolution).
- **Reference lines on level chart** [IMP B10]:
  - Dashed amber line at `pump_start_level` — labeled `Start (X%)`.
  - Dashed green line at `pump_stop_level` — labeled `Full (X%)`.
- Update cadence label: "Updated every 3s" / "Updated every ~Xs (sleep mode)" based on current interval.

#### System Info (`components/DashboardSystemInfo.tsx`)

- ESP32 diagnostics: WiFi RSSI, uptime, heap usage, min free heap.
- Sensor health:
  - Level sensor: health score bar (0–100), cycles OK vs timeout count, `level_last_valid_age_sec`.
  - Flow sensor: stuck-high events, discarded readings.
  - `total_pump_cycles`, `total_pump_run_min` — operational telemetry.

#### Activity Log (`components/ActivityPanel.tsx`)

- Shows last 10 audit events via `useAuditEvents`.
- Each entry:
  - Icon by action type.
  - Primary: `detail` (if present) else `formatAction(action)`.
  - Secondary: user email / UID stub.
  - Right-aligned: relative time (`Just now` / `Xs` / `Xm` / `Xh`).

---

### Loading State (`status === null`) [IMP B11]

On initial load before Firebase data arrives, render skeleton screens:

- **TankVisual**: empty tank outline with shimmer animation, no fill.
- **Stat strip**: 3 cards with gray placeholder bars (no values).
- **Run Controls**: buttons rendered but disabled, "Loading…" text.
- **Mode Controls**: segmented control rendered but disabled.

No blank pages. No layout shifts. The user sees the structure immediately and knows the app is loading.

---

### Connection & State Model

#### `usePumpData` Hook

Handles all data subscriptions and control writes. No component writes directly to Firebase.

```typescript
interface UsePumpDataReturn {
  // State
  status: PumpStatus | null;
  control: PumpControl | null;
  isOnline: boolean;           // dashboard has Firebase connection
  esp32Online: boolean;        // status updated within 20s
  isAdmin: boolean;
  lastUpdatedAt: Date | null;
  history: HistoryEntry[];
  isAddingCountdownTime: boolean;  // roundtrip-confirmed countdown_add_time state

  // Run controls
  startManualRun: () => Promise<void>;           // one-shot, 5s reset, 8s optimistic
  stopRun: () => Promise<void>;                  // one-shot, 5s reset, reverts to AUTO
  startCountdown: (minutes: number) => Promise<void>;
  addCountdownTime: () => Promise<void>;         // disabled until Firebase confirms false

  // Mode controls
  setModeAuto: () => Promise<void>;
  setModeForceOff: () => Promise<void>;
  setModeForceOn: () => Promise<void>;           // throws if !isAdmin

  // Error / maintenance
  clearError: () => Promise<void>;
  setBypassLevelSensor: (enabled: boolean) => Promise<void>;  // throws if !isAdmin

  // Admin
  requestReboot: () => Promise<void>;            // throws if !isAdmin
}
```

**One-shot timing:**
- `manual_start`, `manual_stop`: set `true`, disable button, reset to `false` after **5 seconds**.
- `countdown_add_time`: set `true`, set `isAddingCountdownTime = true`, clear only when Firebase control node confirms `countdown_add_time = false`.

**Optimistic UI (8s timeout):**
All control writes apply an immediate optimistic state. If no confirming status update within 8 seconds, roll back and show "Command timed out" toast.

#### `usePendingControl` Hook

Tracks `pendingMode` and `pendingAck`. Clears on mode confirmation or 8s timeout.

---

### Device Config Modal (`components/DeviceConfigSettings.tsx`) [IMP B8]

Six tabs: Tank Calibration / Pump Thresholds / Dry-Run Protection / Schedule / Advanced / Maintenance.

**Unsaved changes protection:**
- `isDirty` flag is global to the modal (not per-tab).
- Sticky footer on all tabs: `[Discard Changes]  [Save Changes ▶]` — Save disabled when `!isDirty` or `!isAdmin`.
- Modal close when `isDirty`: confirmation dialog: "You have unsaved changes. Discard and close?"
- Back gesture / escape key intercept when `isDirty`.

---

### Mobile vs Desktop Behavior

**Mobile (< 768px, single column):**
```
1. SlimStatusBar (fixed, 56px)
2. DashboardHeader (system name + ⋮ menu)
3. OfflineBanner / RestartFeedback (if active)
4. AlertBanners (ranked, if active)
5. TankVisual (full width, 240px min)
6. Stat strip (3-column, 56px)
7. Run Controls (full width)
8. Mode Controls (segmented pill, full width)
9. ▼ History (collapsible)
10. ▼ System Info (collapsible)
11. ▼ Activity Log (collapsible)
```

**Desktop (≥ 768px, 3-column grid):**
```
Row 1:  StatusBar (full width)
Row 2:  Header (full width, expanded)
Row 3:  AlertBanners (full width, if active)
Row 4:  [Tank (2 cols)] | [Stats (1 col)] | [Run + Mode Controls (1 col)]
Row 5:  [History] | [System Info] | [Activity Log]
```

**Touch targets (all screen sizes):**

| Element | Minimum |
|---|---|
| Stop button | 64px height, full width |
| Quick Start | 56×48px |
| Countdown, Add Time | 48×48px |
| Mode segments | 48×48px each |
| Alert action buttons | 44×44px |
| Settings inputs | 44px height |
| Header overflow icon | 44×44px |

All interactive elements use `touch-manipulation` CSS to eliminate 300ms delay on mobile browsers.

---

### Visual State Reference

| `run_mode` | Tank color | Badge label | Badge color |
|---|---|---|---|
| `"AUTO_STANDBY"` | Blue | `AUTO — Standby` | Blue |
| `"AUTO"` | Animated blue-green | `Filling…` | Green |
| `"MANUAL"` | Green | `Manual Run` | Green |
| `"COUNTDOWN"` | Green + ring | `Xs remaining` | Green |
| `"OFF"` | Gray | `Stopped` | Gray |
| — (error) | Red | `Lockout` | Red |
| — (sleeping) | Indigo | `Sleeping · resumes HH:MM` | Indigo |
| — (maintenance) | Amber | `Maintenance Mode` | Amber |
| — (offline) | Charcoal | `Controller Offline` | Dark red |

**Color semantics:**
- 🔴 Red — safety lockout, operator action required
- 🟡 Amber — degraded state, system operational but monitoring reduced
- 🟢 Green — pump actively running
- 🔵 Blue — AUTO mode, healthy, pump in standby
- ⬛ Gray — deliberately stopped (FORCE_OFF)
- 🟣 Indigo — scheduled sleep (expected, non-alarming)
- 🌫 Charcoal — controller unreachable

---

## Part 3 — Change Summary

| Ref | Area | Change |
|---|---|---|
| IMP B1 | Header / StatusBar | Header reduced to slim bar + overflow menu on mobile. User email and admin badge moved to overflow. Online user count removed. |
| IMP B2 | Tank / stat layout | Tank full-width on mobile. Stats collapse to horizontal strip beneath tank (not column beside it). |
| IMP B3 | Controls grouping | Run Controls and Mode Controls visually separated into distinct labeled sub-sections. |
| IMP B4 | FORCE_OFF labeling | "Emergency Stop" and "Stopped → tap AUTO" are contextual labels, not a button that changes function. Mode shown as segmented control, not separate buttons. |
| IMP B5 | TankVisual glow | Idle cyan glow removed. Only green (running) and red (error) glow states retained. |
| IMP B6 | Restart feedback | Time-phased messaging (0–10s, 10–30s, >30s) with elapsed counter and manual power cycle guidance. |
| IMP B7 | Online user count | Removed (presence removed per v2 design; always-0 value erodes trust). |
| IMP B8 | DeviceConfig modal | Sticky Save/Discard footer on all tabs. Modal close guard when `isDirty`. |
| IMP B9 | Estimated level | `~` prefix, amber color, dashed tank fill, `±5%` subtext when `level_estimate_active`. |
| IMP B10 | History chart | Reference lines for start threshold and stop threshold added to level chart. |
| IMP B11 | Loading state | Skeleton screens for all Layer 1 and Layer 3 elements while `status === null`. |

---

*End of Document — Dashboard Design & Layout v2.0*
