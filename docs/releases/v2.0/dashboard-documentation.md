# Dashboard Technical Documentation · v2.0

**Smart Water Pump Controller — Next.js web dashboard**

This document describes the dashboard subsystem in detail: purpose, architecture, technology stack, data flow, and user-facing behavior. It is intended for developers, integrators, and maintainers. For the exact UI structure and interaction model, see [dashboard-ux-spec.md](./dashboard-ux-spec.md). For the Firebase RTDB contract, see [firmware-rtdb-spec.md](./firmware-rtdb-spec.md). For setup and deployment, see `dashboard/README.md` and [deploy.md](./deploy.md).

---

## 1. Scope and references

### 1.1 Scope

This documentation covers:

- Role of the dashboard within the overall Smart Water Pump Controller system
- Application architecture (App Router, components, state, Firebase)
- Technology stack and project structure
- Data flow (RTDB subscription, control writes, config, notifications)
- Security and access control
- Key features and how they map to RTDB and UX spec

It does not replace the UX spec (exact layout, alerts, controls) or the RTDB spec (schemas and write ownership).

### 1.2 References

| Document | Description |
|----------|-------------|
| [dashboard-ux-spec.md](./dashboard-ux-spec.md) | Page structure, layers, alerts, controls, diagnostics |
| [firmware-rtdb-spec.md](./firmware-rtdb-spec.md) | RTDB layout, control/status/config schemas |
| [deploy.md](./deploy.md) | End-to-end deployment (Firebase, Functions, dashboard, firmware) |
| `dashboard/README.md` | Quick start, env vars, build, deploy to Vercel |

---

## 2. System context

The dashboard is the operator interface for the Smart Water Pump Controller. It does not talk directly to the ESP32; all communication is via Firebase Realtime Database.

- **Dashboard** (Next.js PWA): Authenticates users with Google; subscribes to `/pump_system/status` and `/pump_system/control` (and related config); displays live tank level, flow, pump state, and alerts; writes control commands and device config (admin); manages notification preferences and can trigger Cloud Functions (e.g. email/push alerts).
- **Firmware** (ESP32): Reads sensors, runs pump and safety logic, pushes status to RTDB, reads control and config from RTDB.
- **Firebase RTDB:** Single source of truth for status and control; dashboard and firmware are the only writers to their respective paths.

---

## 3. Technology stack

| Layer | Technology | Notes |
|-------|------------|--------|
| Framework | Next.js 14 (App Router) | Client components for real-time UI; server where needed |
| Language | TypeScript | Typed interfaces aligned with RTDB and firmware |
| Styling | Tailwind CSS | Utility-first; responsive layout |
| Charts | Recharts | History chart (level + flow) |
| Icons | Lucide React | Consistent icon set |
| Database | Firebase Realtime Database | Real-time subscription to status/control/config |
| Auth | Firebase Auth (Google) | Optional restriction via `NEXT_PUBLIC_AUTHORIZED_UIDS` |
| Push | Firebase Cloud Messaging (FCM) | Optional; VAPID key in env; service worker in `app/api/firebase-messaging-sw/` |
| Fonts | Syne, DM Sans, JetBrains Mono | Loaded in root layout |

---

## 4. Application architecture

### 4.1 High-level structure

- **Single main page:** `app/page.tsx` is the dashboard. It is wrapped in `AuthGuard`; unauthenticated users are redirected to `/login`.
- **Real-time data:** `usePumpData()` subscribes to RTDB and exposes a snapshot (status + control + `updatedAt`), history array for the chart, and handlers for mode, clear_error, reboot, manual run, countdown, bypass.
- **Config and notifications:** `useDeviceConfig()` and `useNotificationConfig()` read/write device config and per-user notification preferences. Admin status is derived from `pump_system/config/admins/{uid}` (and optionally `NEXT_PUBLIC_AUTHORIZED_UIDS`).
- **UI composition:** The page composes `StatusBar`, `DashboardHeader`, alert banners (from `getRankedAlerts()`), `DashboardMainGrid` (tank + stat cards), run and mode controls, history section, system info, and activity panel. Modals: Device config (gear), Notifications (bell), PWA install prompt.

### 4.2 Directory layout (summary)

```
dashboard/
├── app/
│   ├── api/firebase-messaging-sw/   # Dynamic FCM service worker
│   ├── layout.tsx                   # Root layout, fonts, metadata
│   ├── page.tsx                    # Main dashboard page
│   ├── globals.css                 # Global styles + Tailwind
│   └── manifest.ts                 # PWA manifest
├── components/
│   ├── TankVisual.tsx              # Animated tank level
│   ├── ModeControls.tsx           # AUTO / FORCE_ON / FORCE_OFF / COUNTDOWN
│   ├── RunControls.tsx             # Quick Start, Countdown, Add 5 min, Stop
│   ├── HistoryChart.tsx            # Recharts area chart
│   ├── StatCard.tsx                # Metric card (level, flow, pump status)
│   ├── StatusBar.tsx               # Top bar: connection, uptime, mode, badges
│   ├── DashboardHeader.tsx         # Title, overflow menu, sign out
│   ├── DeviceConfigSettings.tsx     # Gear modal: calibration, thresholds, safety, sleep, advanced
│   ├── NotificationSettings.tsx    # Bell modal: email + push preferences
│   ├── InfoTooltip.tsx             # Help tooltips
│   ├── InstallPrompt.tsx           # PWA install banner
│   ├── AuthGuard.tsx               # Wraps page; redirects to /login if not signed in
│   ├── DashboardMainGrid.tsx       # Tank + stat strip
│   ├── DashboardHistorySection.tsx  # Chart section
│   ├── DashboardSystemInfo.tsx      # Firmware/RTDB diagnostics
│   ├── ActivityPanel.tsx           # Audit log from /pump_system/audit/events
│   └── ...
├── lib/
│   ├── firebase.ts                 # Firebase init, Google Auth
│   ├── usePumpData.ts              # RTDB subscription, snapshot, history, control handlers
│   ├── useDeviceConfig.ts         # Device config read/write
│   ├── useNotificationConfig.ts   # Notification prefs read/write
│   ├── useIsAdmin.ts              # Admin check (admins map + optional UID list)
│   ├── usePendingControl.ts        # Pending control state (e.g. countdown add time)
│   ├── types.ts                   # PumpStatus, PumpControl, DeviceConfig, NotificationConfig
│   ├── constants.ts               # e.g. ESP32_STALE_SEC (controller offline threshold)
│   ├── alertRanking.ts            # getRankedAlerts(status, esp32Online) for banners
│   ├── auth.ts                    # signIn, signOut wrappers
│   ├── fcm.ts                     # FCM token registration
│   └── toast.ts                   # Toast notifications
└── public/                        # PWA icons, static assets
```

### 4.3 Data flow

- **Inbound:** Firebase `onValue` on `/pump_system` (or status/control subtrees) updates a snapshot. The hook computes `updatedAt` (client timestamp when data arrived), builds a rolling history array for the chart, and derives controller-online state (e.g. no update for 20 s ⇒ offline).
- **Outbound:** User actions call handlers that write to RTDB: `setMode()`, `acknowledgeError()` (clear_error), `requestReboot()`, `startManualRun()`, `startCountdown()`, `addCountdownTime()`, `stopRun()`, `setBypassLevelSensor()`. Device config and notification config are written to their respective paths. One-shots (e.g. manual_start, countdown_add_time) are written then cleared by dashboard or firmware per RTDB spec.
- **Admin:** Writes to control (e.g. FORCE_ON, clear_error, reboot, bypass) and to device config are gated by admin check; non-admins see disabled buttons with tooltips.

---

## 5. Key features (summary)

| Feature | Description | RTDB / implementation |
|---------|-------------|------------------------|
| Live status | Tank level, flow, pump state, errors, uptime, WiFi | `status` subscription; StatusBar, TankVisual, StatCards |
| Alerts | Ranked banners: offline, dry-run, overflow, maintenance, sensor errors, sleep | `getRankedAlerts(status, esp32Online)`; red/amber/blue cards |
| Mode control | AUTO, FORCE_OFF, FORCE_ON, COUNTDOWN | Write `control/mode`; ModeControls |
| Run control | Manual run, countdown run, add 5 min, stop | manual_start, manual_stop, countdown_duration_min, countdown_add_time; RunControls |
| Clear error | Acknowledge dry-run/overflow lockout | Write `control/clear_error = true`; ACK in alert banner |
| Device config | Calibration, thresholds, safety, sleep, advanced | Read/write `config/device`; DeviceConfigSettings modal |
| Notifications | Email + push (dry-run, low level, pump started, overflow) | Read/write `config/notifications_by_user/{uid}`; Cloud Functions send email/push; NotificationSettings modal |
| History chart | Rolling level + flow | History array from usePumpData; HistoryChart (Recharts) |
| System info | Firmware telemetry, heap, RTDB failures | Status fields in DashboardSystemInfo |
| Activity log | Audit trail of control actions | Read `audit/events`; ActivityPanel |
| PWA | Installable app; offline shell | manifest.ts, service worker, InstallPrompt |
| Restart feedback | Phases while controller restarts | restartSentAt, uptime reset; RestartBanner |

---

## 6. Security and access control

- **Authentication:** All dashboard routes that need it are protected by Google sign-in. Unauthorized users are redirected to `/login`.
- **Optional UID restriction:** `NEXT_PUBLIC_AUTHORIZED_UIDS` (comma-separated) can restrict which Google UIDs can sign in. If set, only those UIDs are allowed.
- **Admin:** Admin status is determined by `pump_system/config/admins/{uid} = true` in RTDB. Optionally combined with authorized UIDs. Admins can change mode (including FORCE_ON), clear errors, reboot, set bypass, and edit device config. Non-admins see a reduced set of controls (e.g. FORCE_ON disabled with tooltip).
- **Firebase rules:** Server-side rules in `database.rules.json` enforce that only authenticated dashboard users (with admin allowlist for control/config writes) can write control and config; ESP32 (Email/Password) can write status and read control/config. Deploy with `firebase deploy --only database`.

---

## 7. Environment and deployment

- **Environment variables:** See `dashboard/.env.local.example`. Required: Firebase API key, auth domain, database URL, project ID, storage bucket, messaging sender ID, app ID. Optional: authorized UIDs, tank label, VAPID key (push). Never commit `.env.local`.
- **Build:** `npm run build`. Validate with `npm run validate` (lint + build + Lighthouse CI).
- **Deploy:** Recommended on Vercel. Add env vars in project settings; deploy from Git or CLI. Add the deployment URL to Firebase Authentication authorized domains.

For full deployment order (Firebase, Functions, dashboard, firmware), see [deploy.md](./deploy.md). For notifications (email + push) setup, see `docs/operations/NOTIFICATIONS_SETUP.md`.

---

## 8. Consistency with firmware and RTDB

- **Types:** `lib/types.ts` defines `PumpStatus`, `PumpControl`, `DeviceConfig`, and related interfaces to match the RTDB and firmware. Backward-compatibility fields (e.g. `is_sensor_error`) are retained for older firmware.
- **Staleness:** Controller is considered offline if no status update within the threshold (e.g. 20 s) defined in `constants.ts`; controls are then disabled and the UI shows "Controller offline".
- **One-shots:** Dashboard follows the RTDB spec for who resets one-shots (e.g. countdown_add_time is reset by firmware; dashboard disables "Add 5 min" until it sees the flag false or after a timeout).

For full UX details (layers, alerts, controls, loading states), see [dashboard-ux-spec.md](./dashboard-ux-spec.md).
