# System Overview · v2.0

**Smart Water Pump Controller — architecture and components**

This document gives a high-level description of the system for developers and operators. Detailed technical documentation for each subsystem is in the same directory.

---

## 1. Purpose

The Smart Water Pump Controller automates a water pump (e.g. jet pump feeding a 660 L tank) using tank level and pipe flow. It:

- Starts the pump when tank level is at or below a configurable start threshold (e.g. 30%) and stops when level reaches a stop threshold (e.g. 100%).
- Protects the pump from dry-run (no flow while running), overflow (max runtime), and sensor failures.
- Exposes a web dashboard so operators can monitor status, change mode (AUTO / FORCE_OFF / FORCE_ON / COUNTDOWN), run manual or timed runs, and adjust calibration and thresholds without reflashing the controller.
- Optionally sends email and push notifications for dry-run, low level, pump started, and overflow events.

---

## 2. Architecture

The system has three main parts:

| Component | Role | Location |
|-----------|------|----------|
| **Firmware** | Runs on ESP32; reads level and flow sensors, drives relay, enforces safety, syncs with cloud | `firmware/` (Arduino or PlatformIO) |
| **Dashboard** | Next.js PWA; operators sign in with Google, view live data, send commands | `dashboard/` (Node.js, deployable to Vercel) |
| **Firebase** | Realtime Database (status + control + config), Authentication (Google + Email/Password), optional Cloud Functions (notifications) | Firebase project |

There is **no direct link** between the ESP32 and the dashboard. All interaction is through Firebase Realtime Database:

- **Firmware** writes `/pump_system/status` (level, flow, pump state, errors, telemetry) and reads `/pump_system/control` and `/pump_system/config/device`.
- **Dashboard** reads status and control, writes control (mode, clear_error, manual/countdown one-shots, bypass) and device config. Optionally writes notification preferences and audit events.

---

## 3. Data flow (simplified)

```
Sensors (level, flow) → Firmware → status → Firebase RTDB → Dashboard (display)
Dashboard (user action) → control/config → Firebase RTDB → Firmware (apply)
```

- **Status:** Updated roughly every 3 seconds by the ESP32. Dashboard subscribes in real time and shows tank graphic, flow, pump state, alerts, and diagnostics.
- **Control:** Dashboard writes mode and one-shots; firmware reads every few seconds and updates relay and internal state.
- **Config:** Stored under `/pump_system/config/device` (calibration, thresholds, safety, sleep). Dashboard (admin) writes; firmware reads every 30 seconds and persists to NVS when online.

---

## 4. Documentation map

| Document | Audience | Content |
|----------|-----------|---------|
| [system-overview.md](./system-overview.md) (this file) | All | Purpose, architecture, data flow |
| [firmware-documentation.md](./firmware-documentation.md) | Developers, maintainers | Firmware architecture, hardware, safety, config, RTDB integration |
| [dashboard-documentation.md](./dashboard-documentation.md) | Developers, maintainers | Dashboard architecture, stack, data flow, features, security |
| [firmware-rtdb-spec.md](./firmware-rtdb-spec.md) | Developers | RTDB layout, status/control/config schemas, modes, one-shots |
| [dashboard-ux-spec.md](./dashboard-ux-spec.md) | Developers, UX | Page structure, alerts, controls, loading states |
| [deploy.md](./deploy.md) | Operators, DevOps | End-to-end deployment (Firebase, Functions, dashboard, firmware) |
| `firmware/README.md` | Builders | Pinout, build steps, calibration, troubleshooting |
| `dashboard/README.md` | Builders | Env, run, deploy, validation |

---

## 5. Safety and operations

- **Hardware:** TOR (thermal overload relay) protects the motor regardless of firmware. Manual bypass switch can override software; TOR remains in circuit.
- **Firmware:** Dry-run, overflow, and sensor-failure protections can force the pump off. Errors require explicit acknowledgment (dashboard "Clear Error") before the pump can run again.
- **Operations:** Pre-energization checklist, troubleshooting, and notification setup are in the project root `README.md`, `docs/operations/troubleshooting.md`, `docs/operations/safety.md`, and `docs/operations/NOTIFICATIONS_SETUP.md`.
