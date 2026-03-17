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

---
