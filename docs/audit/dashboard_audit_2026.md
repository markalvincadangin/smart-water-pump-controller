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
