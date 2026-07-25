# SmartFlow Dashboard Refactor Audit 2026

**Date:** 2026-03-31
**Phase:** D0
**Status:** COMPLETE

---

## D0.1 — File Inventory

**`app/`**
- `layout.tsx` — Global Next.js app layout, fonts, theme provider.
- `page.tsx` — Main dashboard grid.
- `globals.css` — Global standard styles.
- `login/page.tsx` — Authentication page.
- `error.tsx` / `global-error.tsx` — Root error boundaries.

**`components/`**
- `DashboardHeader.tsx` — sticky header with status.
- `DashboardMainGrid.tsx` — Core layout grid for cards.
- `TankVisual.tsx` / `FlowStrip.tsx` / `StatCard.tsx` — Operational components.
- `RunControls.tsx` / `ModeControls.tsx` — Input components to change pump state.
- `DeviceConfigSettings.tsx` / `NotificationSettings.tsx` — Settings forms.
- `ErrorBoundary.tsx` — React boundary wrapper.

**`lib/`**
- `firebase.ts` — Firebase RTDB initialization.
- `usePumpData.ts` — Core RTDB subscription hook.
- `types.ts` — Partial type definitions.
- `useDeviceConfig.ts` — Config subscription.
- `audit.ts` / `useAuditEvents.ts` — Audit logging.

## D0.2 — Component Tree Mapping

```text
app/
  layout.tsx           → reads: [] | writes: []
  page.tsx             → reads: [status, control]
    DashboardHeader.tsx        → reads: [status.wifi_rssi, auth]
    TankVisual.tsx             → reads: [status.water_level_percent, status.is_sensor_error]
    DashboardSystemInfo.tsx    → reads: [status.uptime_minutes, status.free_heap_bytes]
    ModeControls.tsx           → reads: [control.mode] | writes: [control.mode]
    RunControls.tsx            → reads: [control, status] | writes: [control.manual_start/stop, countdown]
  settings/page.tsx    → reads: [config.device, config.notifications] | writes: [config.device, config.notifications]
```

## D0.3 — Firebase Listener Audit

- `lib/usePumpData.ts`: Unsubscribe missing in cleanup `useEffect`. **[LEAK]**
- `lib/useAuditEvents.ts`: Unsubscribe missing in cleanup. **[LEAK]**
- `lib/useDeviceConfig.ts`: Unsubscribe missing in cleanup. **[LEAK]**
- `lib/useNotificationConfig.ts`: Unsubscribe missing in cleanup. **[LEAK]**

*Result:* Null checks on `.val()` are inconsistent. Hardcasting with `as` is used infrequently, exposing undefined reads.

## D0.4 — Firebase Write Audit

- `components/DashboardSystemInfo.tsx`: Reboot request lacks try/catch. **[SILENT FAIL]**
- `lib/alertRanking.ts`: Missing try/catch on status clears. **[SILENT FAIL]**
- `lib/useDeviceConfig.ts`: Settings writes lack pending UI states internally. **[UX BUG]**
- `lib/useNotificationConfig.ts`: Missing catch blocks on config updates. **[SILENT FAIL]**

## D0.5 — TypeScript Audit

- Occurrences of `any` type: **6**
- Occurrences of `@ts-ignore`: **0**
- Occurrences of `@ts-expect-error`: **0**
- Missing types: Yes, many Firebase returns are untyped or loosely typed `unknown`.

## D0.6 — UI/UX Bug Inventory

| Component | Bug Description | Severity |
|-----------|-----------------|----------|
| Entire App | Components lack skeleton states on initial load (shows blank briefly). | Medium |
| Settings | Missing validation for `pump_start_level` vs `pump_stop_level`. | High |
| ModeControls | Controls remain clickable while mode write is pending. | High |
| Global | Hardcoded colors instead of semantic Tailwind theme tokens. | Low |
| TankVisual | Missing ARIA labels for accessibility. | Low |

## D0.7 — Dependency Audit

- `next`: `14.2.35` (Meets requirement ≥14.2)
- `firebase`: `11.0.2` (Meets requirement ≥10.x)
- `tailwindcss`: `3.4.17` (Meets requirement ≥3.4)
- `typescript`: `5.7.2` (Meets requirement ≥5.x)
- *Note:* No critical audit vulnerabilities found in current tree.

## D0.8 — Firebase Schema Gap Table

| Firebase field | In schema | Dashboard reads | Dashboard writes | Gap |
|----------------|-----------|-----------------|------------------|-----|
| `pump_cooldown_remaining_sec` | ✅ | ❌ | N/A | Missing display |
| `manual_runtime_warning` | ✅ | ✅ | N/A | Missing display (Already added manually previously) |
| `bypass_flow_sensor` | ✅ | ❌ | ❌ | Missing display + control |
| `is_idle_mode` | ✅ | ❌ | N/A | Missing display |
| `debug_log_level` | ✅ | ❌ | ❌ | Missing display + control |
| `remote_level_discard_count` | ✅ | ❌ | N/A | Missing display |
| `run_mode` new values | ✅ | Partial | N/A | Missing COOLDOWN states |

## D0.9 — Scope Revision

- Replace missing/partial `types.ts` with canonical schema (Phase D1).
- Add cleanup to all `onValue` react hooks (Phase D2).
- Apply try/catch + `isPending` to all Firebase writes (Phase D2).
- Establish Geist typography and semantic sf.* colors (Phase D3).
- Implement D0.8 missing GAP fields in UI components (Phase D4 & D5).

**Phase D0 complete. Proceeding to D1.**

---

## Live Validation Update — 2026-04-03

**Status:** COMPLETE

### Scope
- Performed click-by-click live dashboard validation against active RTDB.
- Verified control-path reflection after each UI action.
- Executed edge-case inputs and rapid mode transitions.

### Live Tests Executed
- Mode transitions: `AUTO -> MANUAL -> AUTO -> COUNTDOWN -> AUTO`
- Countdown boundaries:
  - Input `0` then start -> clamped to `1`
  - Input `999` then start -> clamped to `120`
- Rapid transition cleanup: `COUNTDOWN -> MANUAL -> AUTO`
- Emergency stop safety path: open confirm, then cancel (non-actuating)
- Device Settings open/close no-op check

### RTDB Reflection Results
- `/pump_system/control/mode` reflected each mode transition correctly.
- Countdown start writes reflected:
  - `countdown_duration_min` clamped to `[1, 120]`
  - `countdown_start=true`, `countdown_stop=false` on start
- Returning to `AUTO` cleared stale countdown one-shot flags:
  - `countdown_start=false`
  - `countdown_stop=false`
  - `countdown_add_time=false`
  - `countdown_add_min=0`
- Emergency stop cancel path left:
  - `emergency_stop=false`
  - `mode="AUTO"`

### Issues Found and Fixed
| ID | Area | Finding | Resolution | Status |
|----|------|---------|------------|--------|
| DL-01 | `components/ControlPanel.tsx` | Duplicate Emergency Stop CTA visible on mobile (in-panel + sticky bar). | Kept sticky CTA as primary mobile trigger; made in-panel non-latched E-stop button desktop-only and hid sticky CTA while confirmation panel is open. | ✅ Resolved |

### Post-Fix Validation
- Dashboard tests: **PASS** (`3/3 suites`, `17/17 tests`)
- Next.js production build: **PASS**
- Live RTDB control node restored to safe baseline after test:
  - `mode="AUTO"`
  - `manual_desired=false`
  - `emergency_stop=false`
  - `countdown_start=false`
  - `countdown_stop=false`
  - `bypass_level_sensor=false`
  - `bypass_flow_sensor=false`

---

## Live Validation Continuation — 2026-04-03 (Second Pass)

**Status:** COMPLETE

### Scope
- Continued edge-case validation for desync/hold behavior with live RTDB-backed UI.
- Focused on safety invariant: Emergency Stop must remain operable during control hold.

### Live Tests Executed
- Reproduced active control hold state from stale one-shot command path.
- Triggered Emergency Stop while hold banner was active.
- Confirmed confirmation dialog still opens and accepts action during hold.
- Verified activity feed recorded a new "Emergency stop requested" event.

### UI and RTDB Response Mapping
- Hold banner before actuation:
  - `Control hold active: One-shot control command is pending beyond expected window.`
- Hold banner after Emergency Stop confirm:
  - `Control hold active: Emergency stop command is pending without latch confirmation.`
- Control panel behavior during hold:
  - Mode and timer controls remain disabled.
  - Emergency Stop remains enabled (desktop and mobile trigger paths).
- RTDB/operator reflection:
  - Emergency stop dispatch accepted during hold (evidenced by new activity event).
  - Dashboard switched to emergency-stop-specific hold reason immediately after confirm.

### Issues Found and Fixed (Second Pass)
| ID | Area | Finding | Resolution | Status |
|----|------|---------|------------|--------|
| DL-02 | `components/ControlPanel.tsx`, `app/page.tsx` | Emergency Stop became disabled/blocked when safety hold was active, preventing emergency actuation in the exact condition where override is most critical. | Removed hold-based disable predicates from Emergency Stop controls and removed hold guard in `onEmergencyStop`, while keeping hold gates on non-emergency controls. | ✅ Resolved |

### Post-Fix Validation
- Dashboard tests: **PASS** (`3/3 suites`, `17/17 tests`)
- Next.js production build: **PASS**
- Live UI check: Emergency Stop confirmation flow remains reachable and actionable under active hold.

---

## Pre-Flash Readiness Recheck — 2026-04-03 (Second Pass B)

**Status:** CONDITIONAL (Hold)

### Scope
- Re-ran firmware compile readiness checks for both master and sensor nodes.
- Performed a non-invasive live dashboard snapshot to confirm present RTDB/control safety state before physical flash.

### Build Verification
- Master firmware (`platformio_smart_water_pump_controller`, env `esp32dev`): **PASS**
  - RAM: 15.0% (49,208 / 327,680)
  - Flash: 36.1% (1,134,449 / 3,145,728)
- Sensor firmware (`platformio_sensor_node`): **PASS** across configured environments
  - `nodemcuv2`, `nodemcuv2_debug_usb`, `nodemcuv2_ota`, `nodemcuv2_ota_usb` all succeeded.
  - OTA env printed an `upload_port` notice but build artifacts still completed successfully.

### Live State Snapshot (No Writes)
- Dashboard remained online and authenticated.
- Active hold banner observed:
  - `Control hold active: Emergency stop command is pending without latch confirmation.`
- System alerts observed:
  - `Sensor Comm Loss` and stale data indicators.

### Flash Gate Decision
- **NO-GO until control hold is cleared**.
- Rationale:
  - Pending emergency-stop/desync hold indicates command/status mismatch at this moment.
  - Pre-flash baseline must return to safe defaults before live hardware run.

### Required Unblock Before Flash
- Confirm these RTDB control/config values are set before flashing:
  - `/pump_system/control/emergency_stop = false`
  - `/pump_system/control/countdown_stop = false`
  - `/pump_system/control/bypass_level_sensor = false`
  - `/pump_system/control/bypass_flow_sensor = false`
  - `/pump_system/control/mode = "AUTO"`
  - `/pump_system/config/device/flow_calibration_factor = 7.5`
  - `/pump_system/config/device/tank_full_cm = 30`
- Note: automatic cleanup script could not be executed in this session because admin credentials (ADC/service-account) were unavailable in local environment.

### Unblock Execution Result (Same Day)
- Service-account credentials were provided and cleanup script was executed successfully.
- Dry-run identified pending fixes:
  - `/pump_system/control/emergency_stop: true -> false`
  - `/pump_system/control/countdown_stop: true -> false`
- Apply completed successfully with those updates written.
- Follow-up dry-run result:
  - `No changes needed. RTDB already matches target pre-flash values.`
- Live dashboard verification after cleanup:
  - Control hold banner cleared.
  - Mode and timer controls re-enabled.
  - Firebase remained connected.

### Flash Gate (Updated)
- **GO for controlled live flashing/testing**, with standard safety observation checklist during first boot.

