# Dashboard UX Spec · v2.0

Authoritative reference for the web dashboard UI and interaction model for
version **2.0** of the Smart Water Pump Controller.

It is based on the “Improved Design Document” in `docs/DASHBOARD_DESIGN_v2.md`
and describes the implemented, mobile‑first layout and behaviors.

---

## 1. Information Architecture (Layers)

The dashboard uses a four‑layer information hierarchy (top to bottom):

1. **Layer 1 — System state**  
   Connectivity, current mode, tank and pump status, key stats.
2. **Layer 2 — Alerts & banners**  
   Ranked error/warning/info banners (offline, dry‑run, overflow, maintenance, etc.).
3. **Layer 3 — Controls**  
   Run controls (Quick Start, Countdown, Stop) and policy mode controls (AUTO, FORCE_OFF, FORCE_ON, COUNTDOWN).
4. **Layer 4 — Diagnostics**  
   History chart, system info, activity/audit log.

On mobile, Layer 1 should always be visible without scrolling when possible.

---

## 2. Page Structure

Rendered by `dashboard/app/page.tsx`:

```text
<AuthGuard>
  <StatusBar />                  # fixed, slim top bar
  <DashboardHeader />            # system name + controls/overflow
  <main>
    <OfflineBanner />            # dashboard/cloud connectivity issues
    <RestartBanner />            # 3-phase reboot messaging
    <AlertBanners />             # ranked alerts from alertRanking.ts
    <DashboardMainGrid />        # tank + stats + controls
    <DashboardHistorySection />  # level/flow history chart
    <DashboardSystemInfo />      # firmware + RTDB diagnostics
    <ActivityPanel />            # audit log
  </main>
  <Modals>
    <DeviceConfigSettings />
    <NotificationSettings />
    <InstallPrompt />
  </Modals>
</AuthGuard>
```

---

## 3. Layer 1 — System State

### 3.1 `StatusBar`

Compact bar at the very top:

- **Left:** dashboard connectivity and controller online/offline:
  - Dashboard offline (`!connected`): red icon and “Offline” label.
  - Controller online (`esp32Online`): green icon, uptime badge, Wi‑Fi RSSI.
  - Controller offline: amber icon + “Controller offline (Xs)”.
- **Center:** product name (full string on desktop, short “Pump” label on mobile).
- **Right:**
  - Last update age from `updatedAt` (“Just now” / `Xs`).
  - Current policy `mode` badge (AUTO / FORCE_ON / FORCE_OFF / COUNTDOWN).
  - Sensor error and overflow badges when `is_level_sensor_error` / `is_flow_sensor_error` / `is_overflow_error`.
  - Sleep badge when `is_sleeping`.
  - Level telemetry summary:
    - “Level Xs old” when `level_last_valid_age_sec` is high.
    - A small “sensor health” indicator derived from `level_sensor_health_pct`.

Presence / online user counts are **not** displayed.

### 3.2 `DashboardHeader`

Header just below `StatusBar`:

- Left: system label (e.g. “Smart Water Pump System”) and tank label (`NEXT_PUBLIC_TANK_LABEL`).
- Right:
  - On **mobile**: a single overflow `⋮` menu housing:
    - Device settings, Notifications, Restart (admin‑only), Sign out, current user email + Admin badge.
  - On **desktop**: inline buttons for Settings, Notifications, Restart, Sign out, plus current user email.

### 3.3 `DashboardMainGrid` — Tank & Stats

Grid layout:

- **Tank card** (left/top):
  - `TankVisual` full‑height visualization of `water_level_percent` / `estimated_level_pct`.
  - Uses meaningful glow states only:
    - Green animated glow when pump is running (and no error).
    - Red glow when `is_error` or `is_overflow_error`.
    - No glow in idle/standby/sleep/maintenance.
  - When `level_estimate_active` is true:
    - Amber styling, dashed fill edge, and `~XX%` level label (“Estimated · ±5%”).
- **Stat strip** (right/below): three `StatCard`s:
  - **Tank Water Level** — value in `%`, with `~` prefix and amber color when estimated; subtext shows start/stop thresholds.
  - **Flow Rate** — `X.X LPM`, color‑coded for low vs normal flow, with “pump may stop” warning when below threshold.
  - **Pump Status** — value `ERROR` / `ON` / `OFF`, with subtext `Policy: {mode} · Run: {run_mode_label}`, where `"AUTO_STANDBY"` is rendered as `"AUTO (Standby)"`.

On mobile, `TankVisual` is full‑width with the stat strip compressed into a small three‑column row beneath it.

---

## 4. Layer 2 — Alerts & Banners

### 4.1 Offline & Restart

- **Dashboard offline**: when the app cannot reach Firebase at all:
  - Red banner: “Dashboard offline. Can’t connect to the cloud.”
  - Controls disabled.
- **Restart feedback** (`RestartBanner`):
  - Phase 1 (0–10s): “Controller restarting… (usually completes in 10–20 seconds)” with spinner.
  - Phase 2 (10–30s): “Waiting for controller… (Xs elapsed)”.
  - Phase 3 (>30s): “Controller hasn’t responded yet (Xs elapsed)… try a manual power cycle.”
  - Clears when new status arrives and uptime resets.

### 4.2 Ranked Alert Cards

Derived via `getRankedAlerts(status, esp32Online)` in `dashboard/lib/alertRanking.ts`:

1. Controller offline
2. Dry‑run lockout (`is_error`)
3. Overflow (`is_overflow_error`)
4. Auto‑maintenance active (`auto_bypass_active`)
5. Maintenance active (`bypass_level_sensor && !auto_bypass_active`)
6. Level sensor error (`is_level_sensor_error`)
7. Flow sensor error (`is_flow_sensor_error`)
8. Sleeping (`is_sleeping`)

Visual treatment:

- Red cards: full‑width, strong red border, high‑contrast text.
- Amber cards: amber‑tinted, next after red.
- Blue card: smaller info banner for sleep.

Each card includes:

- Icon + title.
- Description.
- Optional action text (e.g. “Clear Error”, “View diagnostics”).

---

## 5. Layer 3 — Controls

Controls are split into two clearly labeled groups.

### 5.1 Run Controls (`RunControls`)

Purpose: “Do something now” — start/stop runs.

- **Quick Start (Manual run)**:
  - Writes `manual_start = true` one‑shot.
  - Uses 5‑second busy window to avoid double‑fire.
  - `run_mode` becomes `"MANUAL"`.
- **Countdown run**:
  - User chooses duration (1–120 min).
  - Dashboard writes `countdown_duration_min` then `mode = "COUNTDOWN"`.
  - Shows `MM:SS` remaining based on `countdown_remaining_sec`.
- **Add 5 min**:
  - Writes `countdown_add_time = true` and waits for firmware to reset it to `false` before allowing another tap.
- **Stop**:
  - For Manual or Countdown run, writes `manual_stop = true` (Manual) or stops countdown appropriately.

Buttons are disabled when:

- `is_error` or `is_overflow_error` (lockout) is true.
- `controlMode === "FORCE_OFF"` (policy blocks runs).
- User is not admin for any admin‑only action.

### 5.2 Mode Controls (`ModeControls`)

Purpose: “How should the system behave” — policy mode selection.

Modes:

- `"AUTO"` — hysteresis control (start at ≤ `pump_start_level`, stop at ≥ `pump_stop_level`).
- `"FORCE_OFF"` — emergency stop / block all AUTO starts.
- `"FORCE_ON"` — Emergency Override (admin‑only; still subject to hard safety like dry‑run lockout).
- `"COUNTDOWN"` — timed run; set via Run Controls.

---
