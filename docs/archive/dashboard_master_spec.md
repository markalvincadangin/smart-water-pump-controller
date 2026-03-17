# Smart Water Pump Controller — Dashboard Master Specification
## Dashboard v2.0 UI/UX · Next.js 14 · Firebase RTDB

> **Document Type:** Engineering Reference Manual — UI Architecture, UX Behavior, Firmware Integration  
> **Client:** Next.js 14 PWA (TypeScript, Tailwind)  
> **Backend:** Firebase Realtime Database · Firebase Auth · Optional Cloud Functions/FCM  
> **Author:** Mark Alvin Cadangin  
> **Related Firmware Docs:** `docs/archive/firmware_master_spec.md`, `docs/releases/v2.0/firmware-rtdb-spec.md`, `docs/releases/v3.0/firmware-spec.md`

---

## Table of Contents

1. [System Architecture Overview](#1-system-architecture-overview)  
2. [Information Architecture & Layout Layers](#2-information-architecture--layout-layers)  
3. [Page Composition & Component Map](#3-page-composition--component-map)  
4. [Layer 1 — System State (StatusBar, Header, Tank & Stats)](#4-layer-1--system-state-statusbar-header-tank--stats)  
5. [Layer 2 — Alerts & Banners](#5-layer-2--alerts--banners)  
6. [Layer 3 — Run & Mode Controls](#6-layer-3--run--mode-controls)  
7. [Layer 4 — Diagnostics (History, System Info, Activity)](#7-layer-4--diagnostics-history-system-info-activity)  
8. [Offline & Restart Behavior](#8-offline--restart-behavior)  
9. [Admin Model, Security & Audit](#9-admin-model-security--audit)  
10. [RTDB Field Mapping (Dashboard ↔ Firmware)](#10-rtdb-field-mapping-dashboard--firmware)  
11. [Mobile vs Desktop Behavior](#11-mobile-vs-desktop-behavior)  
12. [Notifications & PWA](#12-notifications--pwa)  

---

## 1. System Architecture Overview

### 1.1 Three-Layer Communication Model

```text
┌────────────────────────────────────────────────────────┐
│  LAYER 3 — DASHBOARD (this document)                  │
│  Next.js 14 PWA  ←──read/write──►  Firebase RTDB      │
└──────────────────────┬─────────────────────────────────┘
                       │  WebSocket / long-poll (RTDB SDK)
┌──────────────────────▼─────────────────────────────────┐
│  LAYER 2 — FIRMWARE (ESP32, see firmware_master_spec) │
│  Reads:  /pump_system/control   (JSON, 1 round-trip)  │
│  Writes: /pump_system/status    (JSON, 20+ fields)    │
└──────┬──────────────────────────────────┬──────────────┘
       │ Sensors (level/flow)            │ Relay/Pump
┌──────▼──────────────────────────────────▼──────────────┐
│  LAYER 1 — HARDWARE                                    │
│  JSN-SR04T (Level)   YF-G1 (Flow)   GPIO 4 (Relay)    │
└────────────────────────────────────────────────────────┘
```

### 1.2 Core Design Principles

| Principle | Implementation |
|----------|----------------|
| **Layered UI** | 4-layer IA: System State → Alerts → Controls → Diagnostics. Tank + stats form the visual anchor. |
| **Firmware as Source of Truth** | Dashboard treats `/status` as canonical; it never infers pump behavior locally. All control writes are idempotent and observable via status. |
| **Offline-First UX** | Three offline states: dashboard offline, controller offline, and degraded Firebase. UI shows last known status with clear banners and disables unsafe controls. |
| **Safety-Critical Clarity** | Emergency Override and Stop are visually distinct, gated by confirmations, and always respect firmware P1 safety. Error lockouts gate run controls. |
| **Responsive & Touch-Friendly** | Mobile-first layout, ≥48px touch targets, condensed header, overflow menu on small screens. |

---

## 2. Information Architecture & Layout Layers

The dashboard follows the v2.0 design spec (`dashboard-ux-spec.md`), implemented in `app/page.tsx`:

1. **Layer 1 — System state**  
   Connectivity, current mode, tank level, flow, pump status, key stats.
2. **Layer 2 — Alerts & banners**  
   Ranked alerts and system banners (offline, restart, dry-run, overflow, maintenance, sleep).
3. **Layer 3 — Controls**  
   Run controls (MANUAL ON/OFF toggle, Semi-Auto Timer COUNTDOWN, Stop) and mode controls (AUTO, MANUAL, plus collapsible FORCE_OFF/FORCE_ON emergency controls).
4. **Layer 4 — Diagnostics**  
   History chart, system telemetry, and audit activity.

On mobile, Layer 1 is designed to remain visible without scrolling whenever possible (StatusBar + header + tank + stat strip).

---

## 3. Page Composition & Component Map

Rendered from `dashboard/app/page.tsx`:

```text
<AuthGuard>
  <StatusBar />                     # L1 – connectivity + high-level status
  <DashboardHeader />               # L1 – system label + overflow/actions
  <main>
    <OfflineBanner />               # L2 – dashboard cloud errors
    <RestartBanner />               # L2 – 3-phase reboot feedback
    <AlertBanners />                # L2 – ranked alerts (alertRanking.ts)
    <DashboardMainGrid />           # L1/L3 – tank, stats, run/mode controls
    <DashboardHistorySection />     # L4 – level/flow history
    <DashboardSystemInfo />         # L4 – telemetry + health
    <ActivityPanel />               # L4 – audit log
  </main>
  <NotificationSettings />          # modal (bell)
  <DeviceConfigSettings />          # modal (gear)
  <InstallPrompt />                 # PWA install helper
</AuthGuard>
```

Data and behavior are provided by hooks in `dashboard/lib/`:

- `usePumpData()` — subscribes to `/pump_system/status` and `/pump_system/control`, exposes `snapshot`, `history`, connectivity flags, and write methods (`setMode`, `startManualRun`, `startCountdown`, `addCountdownTime`, `stopRun`, `setBypassLevelSensor`, `acknowledgeError`, `requestReboot`).
- `useDeviceConfig()` — reads/writes `/pump_system/config/device`; exposes `config`, `save`, `isDirty`.
- `useIsAdmin()` — resolves admin from `pump_system/config/admins/{uid}` and `NEXT_PUBLIC_AUTHORIZED_UIDS`.
- `useAuditEvents()` — streams `/pump_system/audit/events` for `ActivityPanel`.
- `getRankedAlerts()` — pure function mapping `PumpStatus` + `esp32Online` to ranked alert objects (Layer 2).

---

## 4. Layer 1 — System State (StatusBar, Header, Tank & Stats)

### 4.1 `StatusBar`

Thin bar at the very top; always visible:

- **Left:** dashboard ↔ cloud connectivity and controller presence.
  - Dashboard offline (`!connected`): red banner “Dashboard offline. Can’t connect to the cloud.” All controls disabled.
  - Controller offline (`!esp32Online`): amber indicator + “Controller offline (Xs)”.
  - Controller online: green indicator + Wi‑Fi RSSI + uptime summary.
- **Right:** badges derived from `/status`:
  - **Policy mode badge:** `control.mode` (AUTO / FORCE_OFF / FORCE_ON / COUNTDOWN).
  - **Error badges:** `is_level_sensor_error`, `is_flow_sensor_error`, `is_overflow_error`.
  - **Sleep badge:** `is_sleeping`.
  - **Level freshness & health:** “Level Xs old” from `level_last_valid_age_sec`, small health bar from `level_sensor_health_pct`.

Presence/online user counts are intentionally not shown.

### 4.2 `DashboardHeader`

Row beneath `StatusBar`, focused on identity and global actions:

- **Left:** system label (e.g. “Smart Water Pump System”) + tank label (`NEXT_PUBLIC_TANK_LABEL`).
- **Right (desktop ≥768px):** inline buttons for:
  - Device settings (gear) → `DeviceConfigSettings`.
  - Notifications (bell) → `NotificationSettings`.
  - Restart (admin-only) → sets `reboot_request_id` via `usePumpData`.
  - Sign out.
  - Current user email + “Admin” badge (for admins).
- **Right (mobile <768px):** a single `⋮` **OverflowMenu** icon:
  - Contains Device settings, Notifications, Restart (admin-only, disabled for non-admins), Sign out, current user email + Admin badge.
  - Provides a single 44×44px touch target to access Layer 3/4 tools.

### 4.3 `DashboardMainGrid` — Tank & Stats & Controls

Responsive grid implemented in `DashboardMainGrid.tsx`:

- **Mobile (<768px):**
  - Tank card full-width, min-height 240px. Uses `TankVisual`:
    - Fill height from `status.water_level_percent` or `estimated_level_pct` when `level_estimate_active`.
    - Color bands:
      - 0–10%: red (critical low — “Critical Low” label).
      - 11–30%: amber (low — “Low Water” label).
      - 31–99%: cyan/neutral (“Normal” label).
      - 100%: green (“Full” label after reaching 100%).
    - Glow:
      - Green animated pulse when `is_running && !is_error && !is_overflow_error`.
      - Red static glow when `is_error || is_overflow_error`.
      - No glow in standby, stopped, sleep, or maintenance.
    - When `level_estimate_active`:
      - Dashed/hatched fill, amber level label, prefixed with `~` (e.g. `~73%`).
  - Stat strip directly beneath tank — 3 compact cards:
    - Tank Level % (amber + `~` when estimate).
    - Flow Rate LPM (red warning when below `dry_run_threshold_lpm` while running).
    - Pump status (ERR/ON/OFF).
- **Desktop (≥768px):**
  - 3-column grid:
    - Col 1 (2fr): Tank card.
    - Col 2 (1fr): vertical `StatCard`s for Tank Level, Flow Rate, Pump Status (subtext shows AUTO thresholds and run mode: `"AUTO_STANDBY"` rendered as `"AUTO (Standby)"`).
    - Col 3 (1fr): `RunControls` + `ModeControls` stacked inside a card.

---

## 5. Layer 2 — Alerts & Banners

### 5.1 Offline & Restart Banners

Handled in `page.tsx`:

- **Dashboard offline (cloud unreachable):**
  - Condition: `!connected && error`.
  - Red banner: “Dashboard offline. Can’t connect to the cloud.” + error detail.
  - All controls disabled (via `connected`/`esp32Online` flags).
- **RestartBanner:**
  - Triggered by Restart action in header → `requestReboot()`; `restartSentAt` captured locally.
  - 3 phases:
    - **0–10s:** cyan info banner with spinner — “Controller restarting… (usually completes in 10–20 seconds)”.
    - **10–30s:** cyan info — “Waiting for controller… (Xs elapsed)”.
    - **>30s:** amber warning — “Controller hasn’t responded yet (Xs elapsed). If it doesn’t reconnect, try a manual power cycle.”
  - Auto-clears when status resumes and `uptime_minutes` resets near zero; success toast emitted.

### 5.2 Ranked Alert Cards (`getRankedAlerts`)

`alertRanking.ts` inspects `PumpStatus` and `esp32Online` and classifies each active alert into one of three tiers:

- **Critical** (requires action): controller offline, dry-run lockout, overflow error, FORCE_ON override.  
- **Warning** (monitor / may require configuration or maintenance): level sensor error, auto-maintenance active, maintenance (manual bypass).  
- **Informational** (no immediate action required): flow sensor error, sleeping.

Within each tier, alerts maintain a stable sort order via a numeric `rank`, with **critical always surfaced before warnings, and warnings before informational**.

Each alert includes:

- A severity color (red for critical, amber for warnings, blue/cyan for informational).
- Title + description + optional recovery hint.
- Tier metadata (`"critical"`, `"warning"`, `"info"`) used by the UI to group and collapse alerts.

**Critical & warning alerts** render as individual cards:

- **Critical (red)**: full-width red cards (controller offline, dry-run, overflow, FORCE_ON override).
- **Warnings (amber)**: amber cards (auto-maintenance, maintenance, level sensor error) with any relevant actions.
- **Actions:**
  - Dry-run and overflow: button that calls `acknowledgeError()` (writes `clear_error=true`); button shows an 8s optimistic busy state with timeout toast. Label:
    - `"Clear Error"` when `control.mode !== "MANUAL"`.
    - `"Clear Error & Restart"` when `control.mode === "MANUAL"`, to make clear that clearing the lockout will immediately restart the pump in MANUAL (per firmware `firmware_master_spec` §7.2 S‑03b).
  - Level sensor error / maintenance: “Enable Bypass” button → calls `setBypassLevelSensor(true)`; visible but disabled for non-admins; `title` explains admin requirement and current state.

For **critical lockout alerts** (dry-run and overflow), the description + recovery text presents a short **numbered physical-action checklist**, for example:

1. Check water source and inlet pipe connections.  
2. Restore normal flow / fix any blockage or leak.  
3. Tap **Clear Error** (or **Clear Error & Restart** in MANUAL) only when safe to resume.  

This reduces cognitive load under stress and keeps recovery behavior consistent with the Run Controls error card in §6.1.

**Informational alerts** (flow sensor error, sleeping) are **collapsed into a single compact card**:

- Title: “System notices (N)” where N is the number of informational alerts.
- Body: short bullet list of `{title}: {description}` for each info alert.
- Styling: compact blue/cyan card, visually subordinate to the red and amber cards.

This 3‑tier model is designed to minimise alarm fatigue (only 3–4 prominent alerts at once) while preserving diagnostic richness via the collapsed informational list.

---

## 6. Layer 3 — Run & Mode Controls

### 6.1 Run Controls (`RunControls.tsx`)

Purpose: context-sensitive **run** operations on the pump, driven by firmware `run_mode` and `control.mode`.

State inputs:

- `runMode`: derived from firmware (`status.run_mode`): `"OFF"`, `"AUTO_STANDBY"`, `"AUTO"`, `"MANUAL"`, `"MANUAL_OFF"`, `"COUNTDOWN"`, `"FORCE_ON"`.
- `controlMode`: `control.mode` (`"AUTO"`, `"MANUAL"`, `"FORCE_OFF"`, `"FORCE_ON"`, `"COUNTDOWN"`).
- `remainingSec`: `status.countdown_remaining_sec`.
- `is_error`, `is_overflow_error`, `esp32Online`, `isAddingCountdownTime`, `pendingAck`, `isAdmin`.

Behavior (v5 MANUAL/COUNTDOWN model):

- **Header badge:**
  - Shows current `runMode`, with special labels:
    - `"MANUAL_OFF"` rendered as `MANUAL (Off)` (mode active, pump off).
    - `"FORCE_ON"` rendered as `⚡ OVERRIDE`.
    - `"AUTO_STANDBY"` rendered as `Standby`.
  - COUNTDOWN appends `MM:SS` countdown to the badge text.

- **FORCE_ON state notice:**
  - When `controlMode === "FORCE_ON"`, a red warning card explains that all protections are bypassed and that exit must be done via **Emergency Controls** in `ModeControls`. No extra Stop button is shown here.

- **FORCE_OFF state notice:**
  - When `controlMode === "FORCE_OFF"`, a red card shows “Pump locked out — switch to AUTO or MANUAL to enable controls”. All run actions are disabled.

- **Error lockout (inline Clear Error card):**
  - When `is_error || is_overflow_error`, RunControls collapse to a single inline card with:
    - Title: “Dry-run lockout” or “Overflow lockout”.
    - Description: “Clear the error to resume. Pump is stopped.”
    - Button behavior:
      - When `controlMode !== "MANUAL"`: button label “Clear Error” → `onAcknowledge()` (writes `clear_error=true`). Disabled while `pendingAck` or `!esp32Online`.
      - When `controlMode === "MANUAL"`: button label “Clear Error & Restart” and tooltip text explaining that in MANUAL mode the pump will automatically restart once the error is cleared (per firmware P3 behavior).

- **AUTO mode (informational only):**
  - When `controlMode === "AUTO"`:
    - If `runMode === "AUTO"`, an info card says “Pump is running automatically based on water level.”
    - If `runMode === "AUTO_STANDBY"`/`"OFF"`, an info card explains “Automatic mode — pump starts when water drops below threshold.”
    - A separate **Stop** button is available only when `runMode === "AUTO"`; it calls `stopRun()` (see below).

- **MANUAL mode — ON/OFF toggle (binary control):**
  - Visible when `controlMode === "MANUAL"`.
  - Two large buttons:
    - **ON**:
      - Calls `startManualRun()`:
        - Writes `manual_start=true` (5s auto-reset); firmware sets `pumpMode="MANUAL"` and starts pump.
      - Disabled when already `MANUAL_ON` (`controlMode === "MANUAL" && runMode === "MANUAL"`), when busy, or when not allowed (admin/lockout/offline).
    - **OFF**:
      - Calls `stopRun()`:
        - Writes `manual_stop=true` (5s auto-reset); firmware stops pump and leaves `pumpMode="MANUAL"`, producing `run_mode="MANUAL_OFF"`.
      - Disabled when already in `MANUAL_OFF` (or pump otherwise off in MANUAL), when busy, or when not allowed.
  - Helper text: “Manual mode — all safety protections remain active.”

- **COUNTDOWN active — timer, Stop, and Add time:**
  - When `runMode === "COUNTDOWN"`:
    - A highlighted timer row shows `MM:SS` (`remainingSec` or locally ticked copy).
    - **Stop** button:
      - Calls `stopRun()`:
        - Writes `manual_stop=true` (one stop path for simplicity); firmware stops pump, clears countdown, and reverts `pumpMode` to `"AUTO"`.
    - **Add time** row:
      - Preset buttons (e.g. +1/+5/+10/+15/+20/+30 min) and a numeric input.
      - `Add` button calls `onAddCountdownTime(minutes)`:
        - Writes `countdown_add_min` and `countdown_add_time=true`; firmware extends `countdownEndMs` and resets flag.
      - Disabled while `isAddingCountdownTime` or offline.

- **Semi-Auto Timer — action-first COUNTDOWN start:**
  - Visible when the pump is **idle** (`runMode` in `"OFF"`, `"AUTO_STANDBY"`, `"MANUAL_OFF"`) and not currently in COUNTDOWN.
  - Allows user to select a duration (presets 5/10/15/30/60 min or custom 1–120 min).
  - `Start {N} min Timer` button:
    - Calls `onStartCountdown(duration)`:
      - Writes `countdown_duration_min` then `mode="COUNTDOWN"`.
    - Available only to admins and when not locked out.
  - Helper text: “Semi-auto — pump stops when timer expires, tank is full, or safety triggers.”

- **Stop in AUTO when running:**
  - When `controlMode === "AUTO"` and `runMode === "AUTO"`, a red **Stop** button is available:
    - Calls `stopRun()`:
      - Writes `manual_stop=true` and sets `mode="AUTO"` from the dashboard (except when current mode is MANUAL, per firmware v5 contract).

All actions:

- Are disabled while `!esp32Online`, when in FORCE_OFF, or when a P1 error lockout is active.
- Use an 8s optimistic timeout with a warning toast (`"Command timed out"`) but rely on `/status` as source of truth; UI states are driven by `run_mode` and error flags, not by local assumptions.

### 6.2 Mode Controls (`ModeControls.tsx`)

v5 mode selector: a **2-segment pill** for normal modes (AUTO | MANUAL) plus a collapsible **Emergency Controls** section for FORCE_OFF and FORCE_ON.

Normal mode segments:

```text
[ AUTO ]   [ MANUAL ]
```

- **AUTO segment:**
  - Maps to `mode="AUTO"`.
  - Cyan styling when active; subtle hover on desktop when inactive.
  - On tap: `onSetMode("AUTO")` → `control/mode="AUTO"`.
  - Tooltip: “Automatic — follows water level thresholds.”

- **MANUAL segment:**
  - Maps to `mode="MANUAL"`.
  - Green styling when active.
  - On tap: `onSetMode("MANUAL")` → `control/mode="MANUAL"`.
  - Tooltip: “Manual — operator ON/OFF with full safety.”

Both segments:

- Show a small “Synced / Sending…” badge in the header based on `pendingMode`.
- Are disabled while a mode change is pending (`pendingMode !== null`).

Emergency controls (collapsible section below the pill):

- Titled **“Emergency Controls”** with a chevron; collapsed by default (progressive disclosure).
- Always visible to communicate presence, but certain actions require admin or clear-error first.

Inside Emergency Controls:

- **FORCE_OFF button:**
  - Maps to `mode="FORCE_OFF"`.
  - Red styling; when active, shows “FORCE OFF (Active)”.
  - On tap: `onSetMode("FORCE_OFF")` → `control/mode="FORCE_OFF"`.
  - Helper text when active: “Pump stopped. Tap AUTO to resume automatic mode.”

- **FORCE_ON (Safety Override) flow (2-step typed confirmation):**
  - Only available when:
    - `allowForceOn === true` (admin).
    - No error lockout (`isError === false`).
    - No pending mode change.
  - Step 0 (button):
    - Label: “FORCE ON”.
    - Tooltip: “FORCE ON — absolute override (admin only)”.
  - Step 1 (warning card):
    - Title: “Safety Override Warning”.
    - Body explains that **all safety protections are bypassed** and the pump will run until manually stopped or FORCE_ON auto-timeout expires.
    - Buttons: **Cancel** and **I Understand →** (proceeds to step 2).
  - Step 2 (typed confirmation):
    - Prompts user to type `FORCE` (hard-coded keyword, shortened from `OVERRIDE` to reduce typing error rate under stress) to enable the **Activate Override** button.
    - On confirm: calls `onSetMode("FORCE_ON")` → `control/mode="FORCE_ON"`.
  - When `currentMode === "FORCE_ON"`:
    - A pulsing red banner labeled **“Emergency Override — Run Without Safety”** indicates FORCE_ON is active.
    - Two exit buttons:
      - “Exit to AUTO” → `onSetMode("AUTO")`.
      - “E‑Stop” → `onSetMode("FORCE_OFF")`.

Pending state:

- While `pendingMode` is non-null, the pill segments and emergency buttons honor disabled state and show “Sending…” subtext under the relevant segment.
- An 8s timeout at the hook level shows “Command timed out” if RTDB hasn’t confirmed `control.mode`.

---

## 7. Layer 4 — Diagnostics (History, System Info, Activity)

### 7.1 History (`DashboardHistorySection` + `HistoryChart`)

- Data:
  - `history: HistoryEntry[]` from `usePumpData` (recent samples of level + flow).
  - `events: HistoryEvent[]` from `usePumpData` (lightweight markers for mode changes, run start/stop, and safety faults).
- UI: card with header “Level & Flow History” and Real-time indicator (spinning icon when `connected`).
- `HistoryChart`:
  - Recharts `AreaChart`:
    - Series: `Water Level (%)` and `Flow (LPM)`.
    - Reference lines at `pump_start_level` (“Start (X%)”) and `pump_stop_level` (“Full (X%)`) passed from `DeviceConfig`.
    - Vertical event markers:
      - Red lines for `fault` events (`last_fault_code` transitions such as `DRY_RUN`, `OVERFLOW`).
      - Amber dashed lines for `mode_change` events (changes in `run_mode`).
      - Blue lines for `run_start` / `run_stop` events (edges in `is_running`).
  - Responsive tick density based on container width.
  - When `data.length < 2`: shows “Waiting for data…” skeleton placeholder.

### 7.2 System Info (`DashboardSystemInfo`)

- Compact grid of cards, combining static and dynamic telemetry:
  - Static: “Controller: ESP32”, “Sensors: Level · Flow”, “Protection: Overload · No-flow shutdown”, “Sync: Real-time”.
  - Dynamic (when available from `status`):
    - `total_pump_cycles`.
    - `total_pump_run_min`.
    - `level_sensor_health_pct` (%).
    - `level_last_valid_age_sec` rendered as seconds or minutes (“Xs” / “Xm”).

### 7.3 Activity (`ActivityPanel`)

- Streams last ~10 audit events from `/pump_system/audit/events`.
- Each event:
  - Icon derived from `action` (mode change, manual_start, countdown_start, countdown_add_time, reboot, config saves, bypass toggles).
  - Primary text prefers `event.detail` (e.g. “Mode changed from AUTO to COUNTDOWN”, “Manual run started”, “Controller reboot requested”).
  - Secondary text: `email` or shortened `uid`.
  - Right-aligned relative time (`Just now`, `Xs`, `Xm`, `Xh`).
- Empty state: explanatory copy about how actions will appear here.

All Layer 4 sections (`DashboardHistorySection`, `DashboardSystemInfo`, `ActivityPanel`) are wrapped in a shared `CollapsibleSection`:

- **Mobile (<768px):** collapsed by default; tap header to expand.
- **Desktop (≥768px):** always expanded; chevron hidden.

---

## 8. Offline & Restart Behavior

Summarized from `usePumpData` + `page.tsx`:

1. **Dashboard offline (cloud unreachable):**
   - Red banner at top.
   - All run and mode controls disabled.
   - Last known status remains visible with timestamp.
2. **ESP32 offline (`esp32Online === false`):**
   - Amber Rank 1 alert via `getRankedAlerts`.
   - Run/mode controls disabled; status values dimmed with “Last seen Xs ago” badge.
3. **Firebase degraded (no updates, no error):**
   - Local timer in `usePumpData` tracks time since last successful `onValue`.
   - If >30s without update and no explicit error:
     - Banner “Connection issues — checking…”.
     - **All run and mode controls are disabled** (treated like offline) to avoid sending commands while state is unknown.
   - After 60s, escalates to dashboard-offline state.

Restart behavior is described in [Layer 2 — Alerts & Banners](#5-layer-2--alerts--banners).

---

## 9. Admin Model, Security & Audit

- **Auth:** Google Sign-In via Firebase Auth; optional restriction via `NEXT_PUBLIC_AUTHORIZED_UIDS` (comma-separated UID allowlist).
- **Admin resolution:**
  - Primary source of truth: `pump_system/config/admins/{uid} = true`.
  - `useIsAdmin()` merges `admins` map with `NEXT_PUBLIC_AUTHORIZED_UIDS`.
- **Admin-only actions:**
  - Emergency Override (FORCE_ON segment).
  - Restart (header action).
  - Device configuration writes.
  - Enabling `bypass_level_sensor`.
- **Audit (`dashboard/lib/audit.ts`):**
  - Writes `pump_system/audit/events/{pushId}` with:
    - `ts` (timestamp), `uid`, `email`, `action`, and detailed `detail` string.
  - Standardized detail strings:
    - `mode_change`: “Mode changed from {prev} to {next}”.
    - `manual_start`: “Manual run started”.
    - `manual_stop`: “Manual run stopped”.
    - `countdown_start`: “Countdown started: {N} min”.
    - `countdown_add_time`: “{addMin} min added (was {remaining} remaining)”.
    - `error_cleared`: “Dry-run/overflow error acknowledged and cleared”.
    - `bypass_enabled` / `bypass_disabled`.
    - `reboot_requested`.
    - `config_saved`: “Device config updated: {changed fields}”.

### 9.1 Firebase RTDB Rules — Intended Model

The Firebase Realtime Database is the enforcement point for who may read and write pump state. Project credentials in the client bundle are **not** secret; security relies entirely on RTDB rules.

High-level rules (pseudocode):

- `/pump_system/status`  
  - `.read`: `auth != null` (any signed-in user).  
  - `.write`: only the ESP32 service identity (e.g. a special email or custom claim) may write status.

- `/pump_system/control`  
  - `.read`: `auth != null`.  
  - `.write`: allowed when:
    - `auth != null`, and  
    - (a) user is an admin for **any** control write, or  
    - (b) for non-admins, writes are limited to safe keys (e.g. cannot write `mode="FORCE_ON"`).
  - Additional FORCE_ON guard:
    - If `newData.val() == "FORCE_ON"` for `/pump_system/control/mode`, require `admins/{uid} === true`.

- `/pump_system/config/device` and `/pump_system/config/admins`  
  - `.read`: `auth != null`.  
  - `.write`: `admins/{uid} === true` (admins only).

- `/pump_system/audit/events/{eventId}`  
  - `.read`: `auth != null`.  
  - `.write`: allowed **only when the event does not yet exist** (append-only, no edits or deletes).

An example rules snippet to capture the intent:

```jsonc
{
  "rules": {
    "pump_system": {
      "status": {
        ".read": "auth != null",
        ".write": "auth.token.email === 'ESP32_SERVICE_ACCOUNT@example.com'"
      },
      "control": {
        ".read": "auth != null",
        "mode": {
          ".write": "auth != null && (newData.val() != 'FORCE_ON' || root.child('pump_system/config/admins').child(auth.uid).val() === true)"
        },
        "$other": {
          ".write": "auth != null"
        }
      },
      "config": {
        "device": {
          ".read": "auth != null",
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        },
        "admins": {
          ".read": "auth != null",
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        }
      },
      "audit": {
        "events": {
          "$eventId": {
            ".read": "auth != null",
            ".write": "!data.exists()"  // append-only
          }
        }
      }
    }
  }
}
```

These rules MUST be applied (and adapted with real identifiers) in the Firebase console; this document only specifies the intended security model.

#### 9.1.1 Optional re-authentication for FORCE_ON

For higher-assurance overrides, the dashboard may require a fresh authentication step immediately before entering the typed-confirmation phase of FORCE_ON:

- When the user taps **FORCE ON** (Step 0 in Mode Controls), call a `reauthenticate` helper that wraps Firebase Auth’s `reauthenticateWithCredential()` API for the current user.
- If re-authentication:
  - **Succeeds**: proceed to the existing 2-step flow (warning card → typed `FORCE` keyword).
  - **Fails**: show an inline error (“Session could not be verified; sign in again to use Emergency Override”) and do not advance to the confirmation dialog.
- This pattern ensures the identity at override time is verified, not just the identity at initial login. The exact implementation (prompting for password, device biometric, etc.) is deferred to a later security-focused sprint; this spec only defines the intended UX and security contract.

---

## 10. RTDB Field Mapping (Dashboard ↔ Firmware)

Refer to `firmware-rtdb-spec.md` §2–5 for full schemas. Key fields used by the dashboard:

-- **Status (`/pump_system/status`):**
  - Core: `water_level_percent`, `is_running`, `flow_rate_lpm`, `run_mode` (including `"MANUAL_OFF"` for MANUAL-mode pump-off).
  - Errors: `is_error`, `is_level_sensor_error`, `is_flow_sensor_error`, `is_overflow_error`.
  - Bypass: `bypass_level_sensor`, `auto_bypass_active`.
  - Countdown: `countdown_remaining_sec`.
  - Sleep: `is_sleeping`.
  - Fault: `last_fault_code`, `last_fault_message`.
  - Telemetry: `total_pump_cycles`, `total_pump_run_min`, `level_sensor_health_pct`, `level_last_valid_age_sec`, `wifi_rssi`, `uptime_minutes`, `last_boot_reason`.
  - Resilience: `estimated_level_pct`, `level_estimate_active`, `flow_volume_added_l`.
- **Control (`/pump_system/control`):**
  - `mode`: `"AUTO"`, `"MANUAL"`, `"FORCE_OFF"`, `"FORCE_ON"`, `"COUNTDOWN"`.
  - One-shots: `manual_start`, `manual_stop`, `countdown_add_time`, `clear_error`.
  - Countdown: `countdown_duration_min`.
  - Maintenance: `bypass_level_sensor`.
  - Management: `reboot_request_id`.
- **Device config (`/pump_system/config/device`):**
  - Hydraulics: `pump_start_level`, `pump_stop_level`, `tank_empty_cm`, `tank_full_cm`.
  - Safety: `dry_run_threshold_lpm`, `dry_run_timeout_sec`, `max_pump_runtime_min`.
  - Sleep: `sleep_enabled`, `sleep_start_hour`, `sleep_end_hour`, `sleep_emergency_level`.
  - Resilience: `level_sensor_failure_threshold` / `sensor_failure_threshold`, `auto_bypass_on_sensor_fail`, `auto_bypass_delay_sec`.
  - Idle telemetry: `idle_sensor_interval_ms`, `idle_firebase_interval_ms`.

The dashboard’s TypeScript types in `dashboard/lib/types.ts` mirror these fields in `PumpStatus`, `PumpControl`, and `DeviceConfig`, ensuring compile-time alignment with firmware.

---

## 11. Mobile vs Desktop Behavior

- **Mobile:**
  - Slim header (`StatusBar` + `DashboardHeader` with overflow menu).
  - `TankVisual` full-width, min-height 240px.
  - Stat strip as a single 3-column strip (height ~56px).
  - Run/Mode controls stacked in a single card, Stop button full-width and min-height 64px.
  - Layer 4 sections collapsed by default (`CollapsibleSection`).
- **Desktop:**
  - Three-column `DashboardMainGrid` ([2fr, 1fr, 1fr]).
  - Full `DashboardHeader` buttons visible inline.
  - All Layer 4 diagnostics expanded by default.

---

## 12. Notifications & PWA

- **Notifications:**
  - Configured via `NotificationSettings` modal.
  - Per-user preferences stored under `pump_system/config/notifications_by_user/{uid}`.
  - Cloud Functions (Resend + FCM) send email and/or push notifications on dry-run, low tank, overflow, controller offline, etc., based on status transitions.
- **Push (optional):**
  - Requires `NEXT_PUBLIC_FIREBASE_VAPID_KEY` and HTTPS.
  - Service worker provided by `app/api/firebase-messaging-sw/route.ts`.
  - “Enable push on this device” in NotificationSettings registers FCM token and stores it in the user’s notification config.
- **PWA:**
  - `manifest.ts` and service worker enable “Install app” prompts.
  - `InstallPrompt` component surfaces a gentle prompt after initial use.

This document, together with `dashboard-ux-spec.md`, `dashboard-documentation.md`, and the firmware specs, fully describes the current dashboard behavior, its UX contract, and its integration with the firmware’s offline-first RTDB design.

