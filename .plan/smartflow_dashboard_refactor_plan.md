# SmartFlow — Dashboard Refactor Plan
### Engineering Specification v1.0

**Project:** SmartFlow Dashboard
**Stack:** Next.js 14 (App Router) · TypeScript · Tailwind CSS · Firebase RTDB · Recharts · Vercel
**Plan type:** Research-first systematic refactor
**Optimized for:** AI agent execution (VS Code, Cursor, Windsurf, Claude Code)
**Depends on:** SmartFlow System Refactor Plan v2.0 (firmware schema must be stable before dashboard Phase D4)

---

> **Agent notice**
> This document is the authoritative specification for the SmartFlow dashboard refactor.
> Phase D0 (Research & Audit) is non-negotiable and must complete before any file is
> modified. All assumptions about the current codebase are unverified until D0 confirms
> or contradicts them. D0 findings supersede everything written here.

---

## Governing Principles

1. **Data contract first.** The dashboard is a consumer of the Firebase schema. Never write
   a field to Firebase that the firmware does not read. Never display a field the firmware
   does not write. Verify every field against the canonical schema before building UI for it.

2. **Safety-critical controls need friction.** Emergency stop, mode changes, and bypass
   toggles must require confirmation or have pending states. A misclick must not be able to
   start or stop the pump without a visible intermediate state.

3. **Fail visible, not silent.** Every data-dependent component must have three states:
   loading (skeleton), loaded (data), and error (boundary). Blank white space is never
   acceptable.

4. **Offline-tolerant.** The dashboard must degrade gracefully when Firebase is unreachable.
   Show the last-known data with a staleness indicator. Do not crash or show blank UI.

5. **No over-engineering.** This is a single-user, single-site system. Avoid patterns
   appropriate for multi-tenant or high-scale apps (global state managers, API layers,
   server actions for real-time data). Firebase RTDB listeners + React state is the
   correct architecture.

6. **Read before writing.** Every file must be read in full before modification. The README
   and any documentation may be outdated — trust the source files.

7. **Progressive enhancement.** Core monitoring (level, status, errors) must work on a
   3G connection. Controls and settings can require a better connection.

---

## Scope Definition

### In Scope

| # | Area | Work |
|---|---|---|
| D1 | Codebase audit | Read all dashboard files, document current state |
| D2 | Bug fixes | Fix all confirmed bugs from Phase D0 audit |
| D3 | Type safety | Replace all `any` casts with typed interfaces |
| D4 | New Firebase fields | Add UI for all fields added in firmware refactor |
| D5 | Design system | Apply SmartFlow brand tokens, Geist typography |
| D6 | Layout redesign | Restructure layout per SmartFlow design spec |
| D7 | Component rebuild | Rebuild each card/component to design spec |
| D8 | PWA | Update manifest, icons, theme color |
| D9 | Accessibility | WCAG 2.1 AA — focus states, contrast, ARIA labels |
| D10 | Performance | Listener cleanup, skeleton loaders, offline banner |
| D11 | Rebranding | Replace all "Smart Water Pump Controller" strings |
| D12 | Settings | Validate all settings forms, add new config fields |
| D13 | Responsive | Mobile 375px + desktop 1280px breakpoints |

### Out of Scope

| # | Area | Reason |
|---|---|---|
| O1 | Firebase security rules | Separate sprint |
| O2 | Server-side rendering | RTDB listeners require client-side only |
| O3 | Native mobile app | PWA remains the delivery mechanism |
| O4 | Multi-user authentication | Single Google account, single user |
| O5 | Historical data charts beyond 24h | No server-side aggregation in scope |
| O6 | Dashboard-to-firmware OTA | Firmware concern, out of dashboard scope |
| O7 | Notification provider changes | Cloud Functions concern |

---

## Phase Structure

```
Phase D0 — Research & Audit           ← MANDATORY FIRST. Never skip.
Phase D1 — Type System & Contracts    ← TypeScript interfaces, Firebase types
Phase D2 — Bug Fixes                  ← Fix confirmed bugs from D0
Phase D3 — Design System Setup        ← Tailwind tokens, fonts, globals
Phase D4 — Component Rebuild          ← Cards, controls, charts per design spec
Phase D5 — New Field Components       ← UI for all new firmware Firebase fields
Phase D6 — Settings Hardening         ← Validation, new config fields
Phase D7 — PWA & Accessibility        ← Manifest, ARIA, focus, contrast
Phase D8 — Integration Testing        ← End-to-end dashboard test protocol
```

Phases D1–D3 may run in parallel after D0 is complete.
Phases D4–D6 require D1–D3 to be complete.
D7 runs concurrently with D4–D6.
D8 gates deployment.

---

---

# Phase D0 — Research & Audit

## Objective

Establish an accurate, current baseline of the dashboard codebase before any modification.
Every assumption is unverified until this phase confirms or contradicts it.

## D0.1 — File Inventory

Read every file in the following directories. For each file, record:
- File path
- Primary responsibility (one sentence)
- Dependencies (imports, hooks used)
- Any `TODO`, `FIXME`, `HACK`, or `@ts-ignore` comments
- Any `any` type usage (TypeScript suppression)
- Any hardcoded strings that reference "Smart Water Pump Controller"

Directories to read:
```
dashboard/
  app/                    ← Next.js App Router pages and layouts
  components/             ← All UI components
  lib/                    ← Firebase client, types, hooks, utilities
  public/                 ← manifest.json, icons, fonts
  tailwind.config.ts      ← Current color palette and theme
  package.json            ← Dependencies and versions
  next.config.js/ts       ← Build configuration
  tsconfig.json           ← TypeScript configuration
  .env.local.example      ← Firebase environment variable names
```

## D0.2 — Component Tree Mapping

Produce a component tree showing every component, its parent, and what Firebase data
it consumes or writes. Format:

```
app/
  layout.tsx              → reads: [nothing] | writes: [nothing]
  page.tsx                → reads: [status, control] | writes: [control]
    Header.tsx            → reads: [status.wifi_rssi] | writes: [nothing]
    TankLevelCard.tsx     → reads: [status.water_level_percent] | writes: [nothing]
    PumpStatusCard.tsx    → reads: [status.is_running, status.run_mode] | writes: [nothing]
    ControlPanel.tsx      → reads: [control.mode] | writes: [control.mode, control.manual_desired]
    AlertsCard.tsx        → reads: [status.is_error, status.last_fault_code] | writes: [control.clear_error]
    DiagnosticsCard.tsx   → reads: [status.free_heap_bytes, ...] | writes: [nothing]
  settings/
    page.tsx              → reads: [config.device] | writes: [config.device]
```

## D0.3 — Firebase Listener Audit

For every `onValue()` or equivalent Firebase listener, record:
- File and line where the listener is created
- Firebase path listened to
- Whether an unsubscribe function is called in `useEffect` cleanup
- Whether the data is accessed with null checks
- Whether the data is typed or cast as `any`

**Flag:** Any listener without a cleanup function as a **memory leak bug**.
**Flag:** Any `snap.val()` result used without null check as a **runtime error risk**.

## D0.4 — Firebase Write Audit

For every `set()`, `update()`, or `push()` Firebase write call, record:
- File and line where the write occurs
- Firebase path being written
- Whether there is a pending state (user feedback during write)
- Whether there is error handling (try/catch or `.catch()`)
- Whether controls are disabled while the write is in-flight

**Flag:** Any write without pending state as a **UX bug**.
**Flag:** Any write without error handling as a **silent failure bug**.

## D0.5 — TypeScript Audit

Count and list every occurrence of:
- `any` type usage
- `// @ts-ignore` or `// @ts-expect-error`
- Missing return type annotations on functions
- Untyped Firebase snapshot data

## D0.6 — UI/UX Bug Inventory

Document every observable UI/UX issue. For each:
- Component affected
- Description of the bug
- Severity: Critical / High / Medium / Low
- Reproduction steps if known

Look specifically for:
- Blank states (no loading skeleton, no error boundary)
- Settings form that allows invalid values (start level ≥ stop level, empty required fields)
- Controls that can be clicked while a Firebase write is pending
- Mobile layout issues (content clipped, emergency stop hidden below fold)
- Dark/light theme inconsistencies
- Missing ARIA labels on interactive elements
- Text that does not meet WCAG 2.1 AA contrast ratio (4.5:1 for normal text)

## D0.7 — Dependency Audit

```bash
cd dashboard && npm audit --json > /tmp/npm_audit.json
```

List all critical and high severity vulnerabilities. Note any dependencies that have
major version updates available (Next.js, Firebase SDK, Tailwind, Recharts).

Check current versions against:
- Next.js: target ≥ 14.2.x (security patches)
- Firebase JS SDK: target ≥ 10.x (modular API)
- Tailwind CSS: target ≥ 3.4.x
- TypeScript: target ≥ 5.x

## D0.8 — Current vs Expected Firebase Schema Delta

Compare the fields currently read and written by the dashboard against the canonical
Firebase schema in `smartflow_refactor_plan_v2.md` Section 4.2.

Produce a gap table:

| Firebase field | In schema | Dashboard reads | Dashboard writes | Gap |
|----------------|-----------|-----------------|------------------|-----|
| `pump_cooldown_remaining_sec` | ✅ | ❌ | N/A | Missing display |
| `manual_runtime_warning` | ✅ | ❌ | N/A | Missing display |
| `bypass_flow_sensor` | ✅ | ❌ | ❌ | Missing display + control |
| `is_idle_mode` | ✅ | ❌ | N/A | Missing display |
| `debug_log_level` | ✅ | ❌ | ❌ | Missing display + control |
| `remote_level_discard_count` | ✅ | ❌ | N/A | Missing display |
| `run_mode` new values | ✅ | Partial | N/A | Missing COOLDOWN states |

## D0.9 — Phase D0 Deliverable

Produce `docs/audit/dashboard_audit_2026.md` containing:
1. File inventory (D0.1)
2. Component tree (D0.2)
3. Firebase listener audit with memory leak flags (D0.3)
4. Firebase write audit with UX bug flags (D0.4)
5. TypeScript audit — count and list of `any` usages (D0.5)
6. UI/UX bug inventory with severity (D0.6)
7. Dependency audit (D0.7)
8. Firebase schema gap table (D0.8)
9. Revised scope — what is already good, what needs fixing, what is new

**Phase D0 exit criterion:** All nine sections complete. No other phase begins until
this criterion is met. State explicitly: "Phase D0 complete. Proceeding to D1."

---

---

# Phase D1 — Type System & Contracts

## Objective

Establish a single source of truth for all TypeScript types used in the dashboard.
Every Firebase data shape must be typed. No `any`.

## D1.1 — Firebase Type Definitions

Create or replace `dashboard/lib/types.ts` with complete type definitions matching
the canonical Firebase schema exactly.

```typescript
// dashboard/lib/types.ts
// REFACTOR [D1]: Complete typed interfaces for all Firebase paths
// Source of truth: SmartFlow System Refactor Plan v2.0 §4.2

export type RunMode =
  | 'AUTO_STANDBY'
  | 'AUTO'
  | 'AUTO_COOLDOWN'
  | 'MANUAL_ON'
  | 'MANUAL_OFF'
  | 'MANUAL_COOLDOWN'
  | 'COUNTDOWN'
  | 'STOPPED';

export type FaultCode =
  | 'DRY_RUN'
  | 'OVERFLOW'
  | 'E_STOP'
  | 'COMM_LOSS'
  | 'STALE_LEVEL'
  | 'LEVEL_SENSOR'
  | 'FLOW_SENSOR'
  | 'SAFE_MODE'
  | '';

export type LogLevel = 0 | 1 | 2 | 3 | 4;

export type ControlMode = 'AUTO' | 'MANUAL' | 'COUNTDOWN';

/**
 * /pump_system/status — written by ESP32 every 3 seconds
 */
export interface PumpStatus {
  // Core state
  water_level_percent?: number;          // omitted when -1 (not yet valid)
  is_running: boolean;
  flow_rate_lpm: number;
  run_mode: RunMode;
  pump_cooldown_remaining_sec: number;   // 0 when not in cooldown

  // Error flags
  is_error: boolean;                     // DRY_RUN lockout active
  is_sensor_error: boolean;             // Ultrasonic sensor failure
  is_flow_sensor_error: boolean;
  is_overflow_error: boolean;
  last_fault_code: FaultCode;
  last_fault_message: string;

  // Operational state
  is_idle_mode: boolean;                 // slow-poll mode active
  is_sleeping: boolean;                  // scheduled sleep active
  emergency_stop_latched: boolean;
  manual_desired: boolean;
  bypass_level_sensor: boolean;
  bypass_flow_sensor: boolean;
  manual_runtime_warning: boolean;       // non-latching, info only

  // Sensor health
  remote_sensor_stable: boolean;         // 3 consecutive valid frames
  level_fresh: boolean;                  // age < staleness threshold
  level_sensor_health_pct: number;       // 0–100
  level_estimate_active: boolean;
  estimated_level_pct?: number;
  remote_level_discard_count: number;

  // Timers
  countdown_remaining_sec: number;

  // Flow stats
  flow_volume_added_l: number;

  // Connectivity
  wifi_rssi: number;

  // System
  uptime_minutes: number;
  last_boot_reason: string;
  debug_log_level: LogLevel;
  total_pump_cycles: number;
  total_pump_run_min: number;

  // Diagnostics
  ultrasonic_cycles_ok: number;
  ultrasonic_cycles_timeout: number;
  ultrasonic_last_good_cm: number;
  free_heap_bytes: number;
  min_free_heap_observed_bytes: number;
  max_alloc_heap_bytes: number;
  firebase_consecutive_failures: number;
  firebase_last_error: string;
}

/**
 * /pump_system/control — read by ESP32 every 3 seconds, written by dashboard
 */
export interface PumpControl {
  mode: ControlMode;
  manual_desired: boolean;
  emergency_stop: boolean;              // one-shot
  reset_stop: boolean;                  // one-shot
  clear_error: boolean;                 // one-shot
  countdown_start: boolean;             // one-shot
  countdown_duration_min: number;
  countdown_add_time: boolean;          // one-shot
  countdown_add_min: number;
  bypass_level_sensor: boolean;
  bypass_flow_sensor: boolean;
  reboot_request_id: number;
}

/**
 * /pump_system/config/device — read by ESP32 every 30 seconds, written by dashboard
 */
export interface DeviceConfig {
  tank_empty_cm: number;
  tank_full_cm: number;
  pump_start_level: number;             // must be < pump_stop_level
  pump_stop_level: number;
  dry_run_threshold_lpm: number;        // default 1.0
  dry_run_timeout_sec: number;
  max_pump_runtime_min: number;
  flow_calibration_factor: number;
  debug_log_level: LogLevel;
  sleep_enabled: boolean;
  sleep_start_hour: number;             // 0–23 PHT
  sleep_end_hour: number;
  sleep_emergency_level: number;
  sensor_failure_threshold: number;
  idle_sensor_interval_ms: number;
  idle_firebase_interval_ms: number;
}

/**
 * /pump_system/config/notifications_by_user/$uid
 */
export interface NotificationPrefs {
  enabled: boolean;
  email: string;
  fcmTokens: string[];
  dryRunAlert: boolean;
  lowLevelAlert: boolean;
  lowLevelThreshold: number;
  pumpStartedAlert: boolean;
  overflowAlert: boolean;
}

/**
 * Convenience: null-safe status with all fields defaulted
 * Use when a component needs guaranteed non-null values
 */
export const DEFAULT_STATUS: PumpStatus = {
  is_running: false,
  flow_rate_lpm: 0,
  run_mode: 'AUTO_STANDBY',
  pump_cooldown_remaining_sec: 0,
  is_error: false,
  is_sensor_error: false,
  is_flow_sensor_error: false,
  is_overflow_error: false,
  last_fault_code: '',
  last_fault_message: '',
  is_idle_mode: false,
  is_sleeping: false,
  emergency_stop_latched: false,
  manual_desired: false,
  bypass_level_sensor: false,
  bypass_flow_sensor: false,
  manual_runtime_warning: false,
  remote_sensor_stable: false,
  level_fresh: false,
  level_sensor_health_pct: 0,
  level_estimate_active: false,
  remote_level_discard_count: 0,
  countdown_remaining_sec: 0,
  flow_volume_added_l: 0,
  wifi_rssi: 0,
  uptime_minutes: 0,
  last_boot_reason: '',
  debug_log_level: 2,
  total_pump_cycles: 0,
  total_pump_run_min: 0,
  ultrasonic_cycles_ok: 0,
  ultrasonic_cycles_timeout: 0,
  ultrasonic_last_good_cm: 0,
  free_heap_bytes: 0,
  min_free_heap_observed_bytes: 0,
  max_alloc_heap_bytes: 0,
  firebase_consecutive_failures: 0,
  firebase_last_error: '',
};
```

## D1.2 — Firebase Hook with Full Types

Create `dashboard/lib/usePumpData.ts`:

```typescript
// dashboard/lib/usePumpData.ts
// REFACTOR [D1]: Typed, null-safe Firebase data hook with cleanup

import { useEffect, useState } from 'react';
import { ref, onValue, off } from 'firebase/database';
import { db } from './firebase';
import type { PumpStatus, PumpControl, DeviceConfig } from './types';

interface PumpDataState {
  status: PumpStatus | null;
  control: PumpControl | null;
  config: DeviceConfig | null;
  isLoading: boolean;
  isConnected: boolean;    // Firebase RTDB connection state
  lastUpdatedAt: Date | null;
  error: string | null;
}

export function usePumpData(): PumpDataState {
  const [state, setState] = useState<PumpDataState>({
    status: null, control: null, config: null,
    isLoading: true, isConnected: false,
    lastUpdatedAt: null, error: null,
  });

  useEffect(() => {
    const statusRef = ref(db, '/pump_system/status');
    const controlRef = ref(db, '/pump_system/control');
    const configRef = ref(db, '/pump_system/config/device');
    const connRef = ref(db, '.info/connected');

    const unsubStatus = onValue(statusRef, (snap) => {
      const val = snap.val() as PumpStatus | null;
      setState(prev => ({
        ...prev,
        status: val,
        isLoading: false,
        lastUpdatedAt: val ? new Date() : prev.lastUpdatedAt,
        error: null,
      }));
    }, (err) => {
      setState(prev => ({ ...prev, error: err.message, isLoading: false }));
    });

    const unsubControl = onValue(controlRef, (snap) => {
      setState(prev => ({ ...prev, control: snap.val() as PumpControl | null }));
    });

    const unsubConfig = onValue(configRef, (snap) => {
      setState(prev => ({ ...prev, config: snap.val() as DeviceConfig | null }));
    });

    const unsubConn = onValue(connRef, (snap) => {
      setState(prev => ({ ...prev, isConnected: snap.val() === true }));
    });

    // REQUIRED: cleanup all listeners on unmount
    return () => {
      off(statusRef, 'value', unsubStatus);
      off(controlRef, 'value', unsubControl);
      off(configRef, 'value', unsubConfig);
      off(connRef, 'value', unsubConn);
    };
  }, []); // Empty deps — listeners established once per mount

  return state;
}
```

## D1.3 — Firebase Write Utilities

Create `dashboard/lib/pumpActions.ts`:

```typescript
// dashboard/lib/pumpActions.ts
// REFACTOR [D1]: Typed, error-handled Firebase write actions

import { ref, set, update } from 'firebase/database';
import { db } from './firebase';
import type { ControlMode, LogLevel } from './types';

// All actions return Promise<void> and throw on error.
// Callers must wrap in try/catch and set pending state.

export const setMode = (mode: ControlMode) =>
  set(ref(db, '/pump_system/control/mode'), mode);

export const setManualDesired = (desired: boolean) =>
  set(ref(db, '/pump_system/control/manual_desired'), desired);

export const triggerEmergencyStop = () =>
  set(ref(db, '/pump_system/control/emergency_stop'), true);

export const resetEmergencyStop = () =>
  set(ref(db, '/pump_system/control/reset_stop'), true);

export const clearError = () =>
  set(ref(db, '/pump_system/control/clear_error'), true);

export const startCountdown = (durationMin: number) =>
  update(ref(db, '/pump_system/control'), {
    countdown_start: true,
    countdown_duration_min: durationMin,
  });

export const addCountdownTime = (addMin: number) =>
  update(ref(db, '/pump_system/control'), {
    countdown_add_time: true,
    countdown_add_min: addMin,
  });

export const setBypassLevel = (bypass: boolean) =>
  set(ref(db, '/pump_system/control/bypass_level_sensor'), bypass);

export const setBypassFlow = (bypass: boolean) =>
  set(ref(db, '/pump_system/control/bypass_flow_sensor'), bypass);

export const setRemoteLogLevel = (level: LogLevel) =>
  set(ref(db, '/pump_system/config/device/debug_log_level'), level);

export const requestReboot = (currentId: number) =>
  set(ref(db, '/pump_system/control/reboot_request_id'), currentId + 1);
```

## D1.4 — Phase D1 Exit Criteria

- [ ] `lib/types.ts` contains all interfaces matching canonical schema — zero `any`
- [ ] `lib/usePumpData.ts` implemented with cleanup functions for all listeners
- [ ] `lib/pumpActions.ts` implemented with typed, error-raising action functions
- [ ] `npm run type-check` (or `tsc --noEmit`) produces zero TypeScript errors on new files

---

---

# Phase D2 — Bug Fixes

## Objective

Fix all confirmed bugs from Phase D0. Apply only to confirmed bugs — do not refactor
working code on suspicion.

## D2.1 — Memory Leak: Firebase Listener Cleanup

**Pattern:** Apply to every `onValue` call found in D0.3 without cleanup.

```typescript
// REFACTOR [D2-LEAK]: Firebase listener must be unsubscribed on unmount
useEffect(() => {
  const unsubscribe = onValue(ref(db, '/pump_system/status'), handler);
  return () => unsubscribe(); // ← cleanup
}, []);
```

For class-based Firebase SDK (`firebase/compat`), the pattern is `off()` instead.
For modular SDK (`firebase/database`), the returned function from `onValue` IS the
unsubscribe — call it in the cleanup.

## D2.2 — Runtime Error: Unguarded Firebase Data Access

**Pattern:** Every `snap.val()` result can be null. Every nested field access must be
null-safe.

```typescript
// BEFORE (runtime error risk):
const level = snap.val().water_level_percent;

// AFTER (null-safe):
// REFACTOR [D2-NULL]: null-safe data access
const data = snap.val() as PumpStatus | null;
const level = data?.water_level_percent ?? null;
```

## D2.3 — UX Bug: Firebase Writes Without Pending State

**Pattern:** Apply to every write found in D0.4 without pending state.

```typescript
// REFACTOR [D2-PENDING]: disable controls during Firebase write
const [isPending, setIsPending] = useState(false);

const handleAction = async () => {
  if (isPending) return;
  setIsPending(true);
  try {
    await someFirebaseAction();
  } catch (err) {
    setWriteError(err instanceof Error ? err.message : 'Write failed');
  } finally {
    setIsPending(false);
  }
};
```

Controls must be visually disabled (`disabled={isPending}`) and non-interactive during
the pending window.

## D2.4 — Settings Validation: Invalid Level Configuration

The settings form must prevent saving when:
- `pump_start_level >= pump_stop_level`
- `tank_full_cm >= tank_empty_cm`
- `dry_run_threshold_lpm < 0.1` or `> 10`
- `dry_run_timeout_sec < 10` or `> 300`
- `max_pump_runtime_min < 30` or `> 480`
- Any required numeric field is empty or NaN

```typescript
// REFACTOR [D2-VALIDATE]: settings form validation before Firebase write
function validateDeviceConfig(config: Partial<DeviceConfig>): string | null {
  if ((config.pump_start_level ?? 0) >= (config.pump_stop_level ?? 100)) {
    return 'Start level must be less than stop level';
  }
  if ((config.tank_full_cm ?? 0) >= (config.tank_empty_cm ?? 200)) {
    return 'Tank full distance must be less than tank empty distance';
  }
  // ... additional validations
  return null; // null = valid
}
```

## D2.5 — Additional Bugs from D0.6

Apply fixes for all bugs discovered in D0.6, working from highest severity to lowest.
Document each fix with: `// REFACTOR [D2-<BUG_ID>]: description`

## D2.6 — Phase D2 Exit Criteria

- [ ] Zero memory leak patterns remaining (all `onValue` calls have cleanup)
- [ ] Zero unguarded `snap.val()` accesses
- [ ] All Firebase write paths have pending state and error handling
- [ ] Settings form validates all fields before writing to Firebase
- [ ] All Critical and High bugs from D0.6 resolved
- [ ] `npm run type-check` passes

---

---

# Phase D3 — Design System Setup

## Objective

Establish the SmartFlow design foundation — tokens, typography, global styles — before
rebuilding any components. Components must use tokens, never hardcoded values.

## D3.1 — Tailwind Color Tokens

Replace or extend `dashboard/tailwind.config.ts`:

```typescript
// dashboard/tailwind.config.ts
// REFACTOR [D3]: SmartFlow brand token system
import type { Config } from 'tailwindcss';

const config: Config = {
  content: ['./app/**/*.{ts,tsx}', './components/**/*.{ts,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        sf: {
          blue:         '#185FA5',
          'blue-mid':   '#378ADD',
          'blue-light': '#E6F1FB',
          'blue-dark':  '#0C447C',
          teal:         '#0F6E56',
          'teal-light': '#E1F5EE',
          'teal-dark':  '#085041',
          amber:        '#BA7517',
          'amber-light':'#FAEEDA',
          'amber-dark': '#854F0B',
          red:          '#A32D2D',
          'red-light':  '#FCEBEB',
          'red-dark':   '#791F1F',
          green:        '#3B6D11',
          'green-light':'#EAF3DE',
          gray: {
            50:  '#F1EFE8',
            100: '#D3D1C7',
            200: '#B4B2A9',
            400: '#888780',
            600: '#5F5E5A',
            900: '#2C2C2A',
          },
        },
      },
      fontFamily: {
        sans:  ['Geist', 'system-ui', 'sans-serif'],
        mono:  ['Geist Mono', 'ui-monospace', 'monospace'],
      },
      borderRadius: {
        card: '12px',
        chip: '6px',
      },
      boxShadow: {
        card: '0 1px 3px 0 rgb(0 0 0 / 0.04), 0 1px 2px -1px rgb(0 0 0 / 0.04)',
        'card-hover': '0 4px 6px -1px rgb(0 0 0 / 0.07)',
      },
    },
  },
  plugins: [],
};

export default config;
```

## D3.2 — Typography Setup (Self-Hosted Geist)

Download Geist and Geist Mono fonts from `https://vercel.com/font` and place in
`dashboard/public/fonts/`. Use Next.js `localFont` for zero-layout-shift loading:

```typescript
// dashboard/app/layout.tsx
import localFont from 'next/font/local';

const geist = localFont({
  src: [
    { path: '../public/fonts/GeistVF.woff2', weight: '100 900' },
  ],
  variable: '--font-geist',
  display: 'swap',
});

const geistMono = localFont({
  src: [
    { path: '../public/fonts/GeistMonoVF.woff2', weight: '100 900' },
  ],
  variable: '--font-geist-mono',
  display: 'swap',
});
```

Apply font variables to the root `<html>` element:
```tsx
<html lang="en" className={`${geist.variable} ${geistMono.variable}`}>
```

## D3.3 — Global CSS

Update `dashboard/app/globals.css`:

```css
/* REFACTOR [D3]: SmartFlow global styles */
@tailwind base;
@tailwind components;
@tailwind utilities;

@layer base {
  :root {
    --page-bg: theme('colors.sf.gray.50');
    --card-bg: #ffffff;
    --card-border: theme('colors.sf.gray.100');
    --text-primary: theme('colors.sf.gray.900');
    --text-secondary: theme('colors.sf.gray.600');
    --text-muted: theme('colors.sf.gray.400');
  }

  .dark {
    --page-bg: #1a1918;
    --card-bg: #232220;
    --card-border: #3a3835;
    --text-primary: #e8e6df;
    --text-secondary: #a09e97;
    --text-muted: #6b6965;
  }

  html {
    font-family: var(--font-geist), system-ui, sans-serif;
    background-color: var(--page-bg);
    color: var(--text-primary);
    -webkit-font-smoothing: antialiased;
  }

  /* Focus ring — visible for keyboard navigation (WCAG 2.1 SC 2.4.7) */
  :focus-visible {
    outline: 2px solid theme('colors.sf.blue');
    outline-offset: 2px;
    border-radius: 4px;
  }
}

@layer components {
  .card {
    @apply bg-[var(--card-bg)] border border-[var(--card-border)]
           rounded-card shadow-card;
  }

  .card-header {
    @apply text-sm font-medium text-[var(--text-secondary)]
           uppercase tracking-wider;
  }

  .status-chip {
    @apply inline-flex items-center gap-1.5 px-2.5 py-1
           rounded-chip text-xs font-medium;
  }

  .btn-primary {
    @apply bg-sf-blue text-white font-medium px-4 py-2 rounded-chip
           hover:bg-sf-blue-dark transition-colors
           disabled:opacity-40 disabled:cursor-not-allowed
           focus-visible:outline focus-visible:outline-2
           focus-visible:outline-sf-blue focus-visible:outline-offset-2;
  }

  .btn-danger {
    @apply bg-sf-red text-white font-medium px-4 py-2 rounded-chip
           hover:bg-sf-red-dark transition-colors
           disabled:opacity-40 disabled:cursor-not-allowed;
  }

  .btn-ghost {
    @apply bg-transparent text-[var(--text-secondary)] border border-[var(--card-border)]
           font-medium px-4 py-2 rounded-chip hover:bg-sf-gray-50
           transition-colors disabled:opacity-40 disabled:cursor-not-allowed;
  }

  /* Skeleton pulse for loading states */
  .skeleton {
    @apply bg-sf-gray-100 dark:bg-[#2e2c2a] rounded animate-pulse;
  }
}
```

## D3.4 — Dark Mode Implementation

Dark mode uses the `class` strategy (set on `<html>`). Persist preference in `localStorage`:

```typescript
// dashboard/lib/useTheme.ts
import { useEffect, useState } from 'react';

export function useTheme() {
  const [theme, setTheme] = useState<'light' | 'dark'>('light');

  useEffect(() => {
    const stored = localStorage.getItem('sf-theme') as 'light' | 'dark' | null;
    const preferred = window.matchMedia('(prefers-color-scheme: dark)').matches
      ? 'dark' : 'light';
    const active = stored ?? preferred;
    setTheme(active);
    document.documentElement.classList.toggle('dark', active === 'dark');
  }, []);

  const toggle = () => {
    const next = theme === 'light' ? 'dark' : 'light';
    setTheme(next);
    localStorage.setItem('sf-theme', next);
    document.documentElement.classList.toggle('dark', next === 'dark');
  };

  return { theme, toggle };
}
```

## D3.5 — Phase D3 Exit Criteria

- [ ] `tailwind.config.ts` contains complete `sf.*` token set
- [ ] Geist + Geist Mono self-hosted and loading correctly (verify in Network tab)
- [ ] `globals.css` has card, chip, button, skeleton component classes
- [ ] Dark mode toggles correctly, persists across page reloads
- [ ] `npm run build` passes with no errors

---

---

# Phase D4 — Component Rebuild

## Objective

Rebuild every major dashboard component to the SmartFlow design specification.
Read the existing component implementation before rewriting — preserve working logic,
replace only layout, styling, and data handling.

## D4.1 — Application Shell

**File:** `dashboard/app/layout.tsx`

Requirements:
- Apply Geist font variables to `<html>`
- Page background: `bg-[var(--page-bg)]`
- Max content width: `max-w-screen-lg mx-auto px-4`
- No horizontal overflow at any viewport width

**File:** `dashboard/components/Header.tsx`

Requirements:
- Sticky top header (`position: sticky; top: 0; z-index: 50`)
- Background: `bg-[var(--card-bg)]` with `border-b border-[var(--card-border)]`
- Left: SmartFlow wordmark — `font-sans font-bold text-sf-blue text-xl`
- Right cluster (left to right):
  1. Connection status indicator — green dot when Firebase connected, amber when reconnecting
  2. RSSI badge — `{rssi} dBm` in `font-mono text-xs`, colored by signal strength
     - ≥ -60 dBm: `text-sf-teal` (good)
     - -60 to -75 dBm: `text-sf-amber` (fair)
     - < -75 dBm: `text-sf-red` (poor)
  3. Last updated — `font-mono text-xs text-[var(--text-muted)]` showing relative time
     ("3s ago", "just now")
  4. Dark/light theme toggle button

**Offline Banner:**
When `isConnected === false`, show a full-width amber banner below the header:
```
⚠ Reconnecting to SmartFlow... Showing last known data from [time].
```
This must never be hidden or dismissable — it is safety-critical information.

## D4.2 — Tank Level Card

**File:** `dashboard/components/TankLevelCard.tsx`

This is the primary sensor display. It must communicate water level at a glance.

**Visual design:**
```
┌─────────────────────────────────┐
│  WATER LEVEL          [header]  │
│                                 │
│  ┌──────┐  82%                  │
│  │ SVG  │  45.2 cm from sensor  │
│  │ tank │                       │
│  │ fill │  ──── Stop  90% ──── │
│  │      │  ──── Start 20% ──── │
│  └──────┘                       │
│                                 │
│  Level fresh  •  Sensor OK      │
└─────────────────────────────────┘
```

**SVG Tank requirements:**
- Viewbox: `0 0 80 160`
- Outer tank shape: rounded rect, `stroke: var(--card-border)`, `stroke-width: 1.5`
- Fill rect: animate height via CSS `transition: height 0.6s ease-in-out`
- Fill color:
  - 0–20%: `fill: #A32D2D` (sf-red)
  - 20–50%: `fill: #BA7517` (sf-amber)
  - 50–100%: `fill: #0F6E56` (sf-teal)
- Level percentage: large centered text, `font-sans font-bold text-3xl`
- Distance sub-label: `font-mono text-sm text-[var(--text-secondary)]`
- Start/Stop level markers: horizontal dashed lines at correct % positions

**Level estimate state** (when `level_estimate_active: true`):
- Percentage shows `~82%` (tilde prefix)
- Sub-label: `Flow estimate · +X.X L` in `sf-amber`
- SVG fill switches to `sf-amber` color

**Status row below SVG:**
- `level_fresh === true` → green dot + "Fresh"
- `level_fresh === false` → amber dot + "Stale"
- `is_sensor_error === false` → green dot + "Sensor OK"
- `is_sensor_error === true` → red dot + "Sensor Error"

**Loading skeleton:**
```tsx
// When status is null (initial load)
<div className="skeleton h-40 w-20 rounded" />       // SVG area
<div className="skeleton h-8 w-16 rounded mt-2" />   // percentage
<div className="skeleton h-4 w-24 rounded mt-1" />   // distance
```

## D4.3 — Pump Status Card

**File:** `dashboard/components/PumpStatusCard.tsx`

```
┌─────────────────────────────────┐
│  PUMP STATUS                    │
│                                 │
│  [AUTO — Running chip]          │
│                                 │
│  Flow    8.30 LPM               │
│  Uptime  2h 14m                 │
│  Boot    Power-on               │
│  Cycles  142 total              │
└─────────────────────────────────┘
```

**Run mode chip** — map `run_mode` string to label + color using MODE_LABELS constant:

```typescript
const MODE_LABELS: Record<string, { label: string; chipClass: string }> = {
  'AUTO_STANDBY':    { label: 'AUTO — Standby',       chipClass: 'bg-sf-gray-50 text-sf-gray-600' },
  'AUTO':            { label: 'AUTO — Running',        chipClass: 'bg-sf-teal-light text-sf-teal' },
  'AUTO_COOLDOWN':   { label: 'AUTO — Cooldown',       chipClass: 'bg-sf-blue-light text-sf-blue-mid' },
  'MANUAL_ON':       { label: 'MANUAL — On',           chipClass: 'bg-sf-teal-light text-sf-teal' },
  'MANUAL_OFF':      { label: 'MANUAL — Off',          chipClass: 'bg-sf-gray-50 text-sf-gray-600' },
  'MANUAL_COOLDOWN': { label: 'MANUAL — Cooldown',     chipClass: 'bg-sf-blue-light text-sf-blue-mid' },
  'COUNTDOWN':       { label: 'Countdown',             chipClass: 'bg-sf-blue-light text-sf-blue' },
  'STOPPED':         { label: 'Emergency Stop',        chipClass: 'bg-sf-red-light text-sf-red' },
};
```

**Cooldown countdown:** When `run_mode` ends in `_COOLDOWN`, show a live countdown
using `pump_cooldown_remaining_sec` from Firebase as the initial value, then decrement
client-side with `setInterval(1000)`. Refresh from Firebase on each update. Display:
`AUTO — Cooldown 47s` within the chip.

**Flow rate:** `font-mono text-lg font-medium`. Show `— LPM` (em dash) when
`is_running === false`.

**Uptime:** Format minutes → `Xh Ym` or `Xm` for < 1 hour. Use `font-mono`.

## D4.4 — Controls Panel

**File:** `dashboard/components/ControlPanel.tsx`

```
┌─────────────────────────────────┐
│  CONTROLS                       │
│                                 │
│  Mode:  [AUTO] [MANUAL] [TIMER] │
│                                 │
│  [● Start Pump]  (MANUAL only)  │
│                                 │
│  Timer: [  15  ] min  [Start]   │
│         [+5 min]  (while active)│
│                                 │
│  ─────────────────────────────  │
│  [🛑 Emergency Stop]            │
└─────────────────────────────────┘
```

**Mode selector:**
- Three-way segmented control: AUTO · MANUAL · TIMER
- Active mode highlighted with `sf-blue` background
- Write to `/pump_system/control/mode` on change
- Disable all segments while `isPending === true`

**MANUAL pump toggle:**
- Only visible when `control.mode === 'MANUAL'`
- Label: "Start Pump" when `status.is_running === false`, "Stop Pump" when `is_running === true`
- Color: `btn-primary` when starting, `btn-ghost` when stopping
- Write `manual_desired: true/false` to control path

**Countdown:**
- Only visible when `control.mode === 'COUNTDOWN'`
- Number input: 1–120 minutes, default 15
- "Start Timer" button: writes `countdown_start: true` and `countdown_duration_min: N`
- While countdown active: show remaining time + "+5 min" button

**Emergency Stop:**
- Always visible, always at the bottom of the panel
- `btn-danger` styling — `bg-sf-red text-white`
- On click: show confirmation popover "Stop the pump now?" with Confirm / Cancel
- On Confirm: write `emergency_stop: true`
- After E-stop latched (`emergency_stop_latched === true`): show Reset button
  that writes `reset_stop: true` with same confirmation flow

**Confirmation popover** (not a modal — use an inline popover near the button):
```tsx
// Inline confirmation pattern — no modals (no position:fixed)
{confirmPending && (
  <div className="border border-sf-red-light bg-sf-red-light rounded-chip p-3
                  flex items-center gap-3 mt-2">
    <span className="text-sf-red text-sm">Stop the pump now?</span>
    <button onClick={handleConfirm} className="btn-danger text-xs py-1 px-3">
      Stop
    </button>
    <button onClick={() => setConfirmPending(false)} className="btn-ghost text-xs py-1 px-3">
      Cancel
    </button>
  </div>
)}
```

## D4.5 — Alerts Card

**File:** `dashboard/components/AlertsCard.tsx`

Shows all active error and warning states. Empty state: `"System operating normally"` in
`text-[var(--text-muted)]`.

**Alert types (show in this priority order):**

1. **Emergency Stop** (`emergency_stop_latched === true`):
   - `bg-sf-red text-white` full-width chip
   - "Emergency stop active. Pump locked out."
   - [Reset] button inline

2. **DRY_RUN / OVERFLOW / E_STOP** (`is_error === true`):
   - `bg-sf-red-light` card with `text-sf-red` heading
   - Show `last_fault_message`
   - [Clear Error] button → writes `clear_error: true`

3. **Sensor error** (`is_sensor_error === true`):
   - `bg-sf-amber-light` card, `text-sf-amber` heading
   - "Ultrasonic sensor error. Level readings may be unreliable."

4. **Flow sensor error** (`is_flow_sensor_error === true`):
   - `bg-sf-amber-light` card
   - "Flow sensor error. Dry-run protection may not function correctly."

5. **Manual runtime warning** (`manual_runtime_warning === true`):
   - `bg-sf-amber-light` non-blocking card
   - "Manual run has exceeded [max_pump_runtime_min] minutes. Operator supervision recommended."
   - No dismiss. No pump action. Information only.

6. **Comm loss** (`remote_sensor_stable === false`):
   - `bg-sf-amber-light` card
   - "Sensor node not responding. Pump may stop if connection is not restored."

7. **Level bypass** (`bypass_level_sensor === true`):
   - `bg-sf-amber-light` informational chip
   - "Level sensor bypassed. Operating on flow guard only."

8. **Flow bypass** (`bypass_flow_sensor === true`):
   - `bg-sf-amber-light` informational chip
   - "Flow sensor bypassed. Dry-run protection disabled."

9. **Idle mode** (`is_idle_mode === true`):
   - Subtle `bg-sf-blue-light` chip
   - "Idle mode active. Sensor updates every [idle_sensor_interval_ms / 1000]s."

## D4.6 — Diagnostics Card (Collapsible)

**File:** `dashboard/components/DiagnosticsCard.tsx`

Collapsed by default. Expand toggle shows/hides content.

**Sections:**

**System health:**
```
Free heap    182,450 B   Min observed  175,230 B
Uptime       2h 14m      Boot reason   Power-on
Total cycles 142         Total run     14h 32m
```

**RS-485 / Sensor:**
```
Sensor stable    ✓ Yes      Level fresh  ✓ Yes
Sensor health    94%        Last good    45.2 cm
Level discards   0 / cycle
Ultrasonic OK    4,820      Timeouts     12
```

**Firebase:**
```
Connection       ✓ Live     Last error   (none)
Consec. failures 0          Log level    INFO [2]
```

**Log level control:**
```tsx
// Segmented: ERROR · WARN · INFO · DEBUG · VERBOSE
// Writes to /pump_system/config/device/debug_log_level
// Shows amber warning when level > 2 (INFO):
// "Verbose logging may increase Firebase bandwidth"
// Display reflects status.debug_log_level (what firmware is actually using)
```

## D4.7 — Phase D4 Exit Criteria

- [ ] All components use `sf-*` tokens — no hardcoded hex colors
- [ ] All components use `font-sans` / `font-mono` — no system-font fallbacks visible
- [ ] Every component has a loading skeleton state
- [ ] Every component is wrapped in a React error boundary
- [ ] E-stop button visible at all scroll positions on mobile (375px)
- [ ] Cooldown countdown runs client-side smoothly (no Firebase poll jitter)
- [ ] Offline banner appears when Firebase disconnects
- [ ] Mode change confirmation flow works for E-stop and Reset

---

---

# Phase D5 — New Field Components

## Objective

Build UI for all Firebase fields added in the firmware refactor (Phase D0.8 gap table).
Every new firmware field must be displayed and/or controlled from the dashboard.

## D5.1 — Cooldown State (covered in D4.3 and D4.5)

Requirement already specified. Confirm `pump_cooldown_remaining_sec` drives the countdown
and `run_mode === 'AUTO_COOLDOWN' || 'MANUAL_COOLDOWN'` triggers the display.

## D5.2 — Manual Runtime Warning (covered in D4.5)

Requirement already specified. Confirm `manual_runtime_warning` field is read from status
and renders the non-blocking amber alert.

## D5.3 — Flow Sensor Bypass Control

**Location:** `dashboard/app/settings/page.tsx` → Advanced section

```
Flow Sensor Bypass
[Toggle OFF/ON]
⚠ Disabling flow sensor bypass enables dry-run protection.
  With bypass ON, the pump will not auto-stop if water runs dry.
```

Write path: `/pump_system/control/bypass_flow_sensor`
Read path: `status.bypass_flow_sensor` (confirm state) + `control.bypass_flow_sensor`

The toggle must show the *confirmed* bypass state from `status`, not the optimistic state
from `control`, to avoid misleading the operator.

Warning text is required whenever `bypass_flow_sensor === true`.

## D5.4 — Idle Mode Badge

**Location:** Header (or Diagnostics card)

When `status.is_idle_mode === true`:
- Small amber badge next to WiFi indicator: `Idle`
- Tooltip/title: `"Sensor and Firebase updates reduced to ${idleIntervalS}s. Tank full and pump off."`

## D5.5 — Remote Log Level Control

**Location:** `dashboard/components/DiagnosticsCard.tsx` → Log level row

```
Log level  [ERROR · WARN · INFO · DEBUG · VERBOSE]
           ↑ reads status.debug_log_level (what firmware is using now)
           ↑ writes config/device/debug_log_level (what to set next read)
```

```tsx
const LOG_LEVEL_LABELS = ['ERROR', 'WARN', 'INFO', 'DEBUG', 'VERBOSE'] as const;
const SAFE_LEVEL = 2; // LOG_INFO

// Show warning when above INFO
{activeLogLevel > SAFE_LEVEL && (
  <p className="text-xs text-sf-amber mt-1">
    Verbose logging may increase Firebase bandwidth and serial output.
  </p>
)}
```

Note: There will be a delay between writing to config and seeing it reflected in status
(ESP32 reads config every 30 seconds). The UI should show the status value (current) and
note the config value as "pending" if they differ.

## D5.6 — Level Discard Count

**Location:** `dashboard/components/DiagnosticsCard.tsx` → Sensor section

```
Level discards   {remote_level_discard_count} / cycle
```

When > 0: show in `text-sf-amber`.
When 0: show in `text-[var(--text-muted)]`.

## D5.7 — Level Estimate Visual

**Location:** `TankLevelCard.tsx` (see D4.2) and Level History Chart (if present)

When `status.level_estimate_active === true`:
- Tank SVG fill color: switch from `sf-teal` → `sf-amber`
- Percentage label: `~{level}%` (tilde prefix, italic)
- Sub-label: `"Flow estimate · +{flow_volume_added_l.toFixed(1)} L"` in `text-sf-amber`
- Chart: dashed amber line during estimate period, vertical dashed marker at bypass start

## D5.8 — Phase D5 Exit Criteria

- [ ] All 6 gap items from D0.8 have corresponding UI
- [ ] `bypass_flow_sensor` toggle visible in settings with warning text
- [ ] `is_idle_mode` badge visible in header when active
- [ ] `debug_log_level` control reads from status and writes to config
- [ ] `remote_level_discard_count` visible in diagnostics, amber when > 0
- [ ] Level estimate visual (tilde, amber, "Flow estimate") applied correctly
- [ ] Cooldown countdown running client-side in run mode chip

---

---

# Phase D6 — Settings Hardening

## Objective

Make the settings page robust, validated, and complete. Add all new config fields.

## D6.1 — Settings Page Layout

**File:** `dashboard/app/settings/page.tsx`

Organize into collapsible sections:

```
/settings

  [← Back to Dashboard]

  Tank Calibration
    Empty distance (cm)   [    ]    ← sensor distance when tank is empty
    Full distance (cm)    [    ]    ← sensor distance when tank is full
    Start pumping at      [ 20 ]%
    Stop pumping at       [ 90 ]%

  Pump Protection
    Dry-run threshold     [1.0 ] LPM   (min: 0.1, max: 10.0)
    Dry-run timeout       [ 30 ] s     (min: 10, max: 300)
    Max pump runtime      [120 ] min   (min: 30, max: 480)

  Flow Calibration
    Calibration factor    [7.5 ]       (pulses per liter; YF-G1 default: 7.5)

  Sleep Schedule
    [Toggle: Enable sleep schedule]
    Sleep from [ 22 ]:00 to [ 06 ]:00  PHT
    Emergency level       [ 10 ]%       (pump if below this level even during sleep)

  Sensor Thresholds
    Sensor failure threshold  [   ]    (CRC failure count before sensor error)
    Idle sensor interval      [   ] ms
    Idle Firebase interval    [   ] ms

  Advanced Controls
    Level sensor bypass   [Toggle]
    Flow sensor bypass    [Toggle]  ← new field from firmware refactor
    Debug log level       [ERROR · WARN · INFO · DEBUG · VERBOSE]  ← new field

  Notifications
    [Notification prefs — existing implementation, verify still works]

  [Save All Changes]    [Cancel]
```

## D6.2 — Settings Form Validation

All validation runs on the client before any Firebase write:

```typescript
// dashboard/lib/validateSettings.ts
// REFACTOR [D6]: complete settings validation

interface ValidationError {
  field: string;
  message: string;
}

export function validateDeviceConfig(config: Partial<DeviceConfig>): ValidationError[] {
  const errors: ValidationError[] = [];

  // Tank geometry
  if (config.tank_full_cm !== undefined && config.tank_empty_cm !== undefined) {
    if (config.tank_full_cm >= config.tank_empty_cm) {
      errors.push({ field: 'tank_full_cm',
        message: 'Full distance must be less than empty distance' });
    }
    if (config.tank_full_cm < 1) {
      errors.push({ field: 'tank_full_cm',
        message: 'Full distance must be at least 1 cm' });
    }
    if (config.tank_empty_cm > 200) {
      errors.push({ field: 'tank_empty_cm',
        message: 'Empty distance cannot exceed 200 cm' });
    }
  }

  // Level thresholds
  if (config.pump_start_level !== undefined && config.pump_stop_level !== undefined) {
    if (config.pump_start_level >= config.pump_stop_level) {
      errors.push({ field: 'pump_start_level',
        message: 'Start level must be less than stop level' });
    }
  }

  // Protection values
  if (config.dry_run_threshold_lpm !== undefined) {
    if (config.dry_run_threshold_lpm < 0.1 || config.dry_run_threshold_lpm > 10) {
      errors.push({ field: 'dry_run_threshold_lpm',
        message: 'Dry-run threshold must be 0.1–10.0 LPM' });
    }
  }

  if (config.dry_run_timeout_sec !== undefined) {
    if (config.dry_run_timeout_sec < 10 || config.dry_run_timeout_sec > 300) {
      errors.push({ field: 'dry_run_timeout_sec',
        message: 'Dry-run timeout must be 10–300 seconds' });
    }
  }

  if (config.max_pump_runtime_min !== undefined) {
    if (config.max_pump_runtime_min < 30 || config.max_pump_runtime_min > 480) {
      errors.push({ field: 'max_pump_runtime_min',
        message: 'Max runtime must be 30–480 minutes' });
    }
  }

  // Sleep hours
  if (config.sleep_start_hour !== undefined) {
    if (config.sleep_start_hour < 0 || config.sleep_start_hour > 23) {
      errors.push({ field: 'sleep_start_hour',
        message: 'Sleep start hour must be 0–23' });
    }
  }

  return errors; // Empty array = valid
}
```

Show validation errors inline next to each field, not just as a toast.

## D6.3 — Settings Save Pattern

```typescript
// REFACTOR [D6]: settings save with validation + pending state + error handling
const handleSave = async () => {
  const errors = validateDeviceConfig(localConfig);
  if (errors.length > 0) {
    setValidationErrors(errors);
    return;
  }

  setIsSaving(true);
  setValidationErrors([]);
  try {
    await update(ref(db, '/pump_system/config/device'), localConfig);
    setSuccessMessage('Settings saved. Firmware will apply within 30 seconds.');
    setTimeout(() => setSuccessMessage(''), 5000);
  } catch (err) {
    setSaveError(err instanceof Error ? err.message : 'Save failed');
  } finally {
    setIsSaving(false);
  }
};
```

## D6.4 — Phase D6 Exit Criteria

- [ ] Settings form has all config fields including new `bypass_flow_sensor` and `debug_log_level`
- [ ] All validation rules implemented and showing inline errors
- [ ] Save shows success message: "Firmware will apply within 30 seconds"
- [ ] Save disabled while `isSaving === true`
- [ ] No settings can be saved that would violate safety invariants (e.g. start ≥ stop)

---

---

# Phase D7 — PWA & Accessibility

## Objective

Ensure the dashboard meets basic PWA installation requirements and WCAG 2.1 AA
accessibility standards. These run concurrently with D4–D6.

## D7.1 — PWA Manifest

**File:** `dashboard/public/manifest.json`

```json
{
  "name": "SmartFlow",
  "short_name": "SmartFlow",
  "description": "Automated water pump controller — Leon, Iloilo",
  "theme_color": "#185FA5",
  "background_color": "#F1EFE8",
  "display": "standalone",
  "orientation": "portrait-primary",
  "start_url": "/",
  "scope": "/",
  "icons": [
    { "src": "/icons/icon-192.png", "sizes": "192x192", "type": "image/png", "purpose": "any maskable" },
    { "src": "/icons/icon-512.png", "sizes": "512x512", "type": "image/png", "purpose": "any maskable" }
  ]
}
```

Generate icons from the SmartFlow brandmark SVG:
- 192×192 PNG: icon-192.png
- 512×512 PNG: icon-512.png
- Apple touch icon: apple-touch-icon.png (180×180)

## D7.2 — Accessibility Requirements

**WCAG 2.1 AA — Non-negotiable minimums:**

1. **Contrast ratios** (SC 1.4.3):
   - Normal text (< 18pt): minimum 4.5:1
   - Large text (≥ 18pt or 14pt bold): minimum 3:1
   - Test all `sf-*` color combinations used as text on background

   Pre-verified combinations:
   - `sf-teal` (#0F6E56) on `sf-teal-light` (#E1F5EE): 5.8:1 ✅
   - `sf-amber` (#BA7517) on `sf-amber-light` (#FAEEDA): 4.6:1 ✅
   - `sf-red` (#A32D2D) on `sf-red-light` (#FCEBEB): 6.1:1 ✅
   - `sf-blue` (#185FA5) on white: 7.2:1 ✅
   - `sf-gray-600` (#5F5E5A) on `sf-gray-50` (#F1EFE8): 4.8:1 ✅

2. **Focus indicators** (SC 2.4.7):
   - Every interactive element must have a visible focus ring
   - Use `:focus-visible` (not `:focus`) — from D3.3 global CSS

3. **ARIA labels on icon-only buttons** (SC 4.1.2):
   ```tsx
   <button aria-label="Toggle dark mode" onClick={toggle}>
     <SunIcon aria-hidden="true" />
   </button>
   <button aria-label="Emergency stop — stops pump immediately" onClick={handleEStop}>
     Stop
   </button>
   ```

4. **Form labels** (SC 1.3.1):
   Every `<input>` must have an associated `<label>` with `htmlFor` matching the input `id`.
   No placeholder-only labeling.

5. **Status announcements** (SC 4.1.3):
   When pump state changes (start, stop, error), announce to screen readers:
   ```tsx
   <div role="status" aria-live="polite" aria-atomic="true" className="sr-only">
     {announceMessage}
   </div>
   ```

6. **Keyboard navigation**:
   - Mode selector must be operable with arrow keys
   - Emergency stop must be reachable via Tab without mouse
   - No keyboard traps

## D7.3 — Mobile Layout Requirements

Test at exactly 375px viewport width (iPhone SE — smallest common modern phone):
- Tank level card: fully visible without horizontal scroll
- Run mode chip: text not truncated
- Control panel: all buttons accessible
- Emergency stop: **visible above the fold without scrolling**
- Settings: all inputs have sufficient tap target size (minimum 44×44px — WCAG SC 2.5.5)
- Header does not collapse below one line

**Emergency stop mobile pinning:**
```tsx
{/* Mobile-only sticky E-stop bar at bottom */}
<div className="fixed bottom-0 left-0 right-0 p-3 bg-[var(--card-bg)]
                border-t border-[var(--card-border)] md:hidden z-40">
  <button
    className="btn-danger w-full"
    aria-label="Emergency stop — stops pump immediately"
    onClick={handleEStop}
    disabled={isPending}
  >
    🛑 Emergency Stop
  </button>
</div>
```

## D7.4 — Phase D7 Exit Criteria

- [ ] `manifest.json` updated with SmartFlow name, colors, icons
- [ ] PWA installable on Chrome Android ("Add to Home Screen" prompt appears on HTTPS)
- [ ] All interactive elements have visible focus rings
- [ ] Emergency stop has `aria-label` describing the action
- [ ] All form inputs have associated `<label>` elements
- [ ] `role="status"` live region announces pump state changes
- [ ] 375px layout passes: no horizontal scroll, E-stop above fold
- [ ] Minimum contrast ratios verified for all text/background pairs

---

---

# Phase D8 — Integration Testing

## Objective

Verify the complete dashboard works correctly against a live Firebase instance and
a running ESP32 before marking the refactor complete.

## D8.1 — Dashboard Integration Test Protocol

**Environment:** Live Firebase project, ESP32 sending status updates, dashboard running
on `localhost:3000` or deployed Vercel preview URL.

| # | Test | Expected result | Pass criteria |
|---|------|----------------|---------------|
| DT-01 | Cold load | Dashboard opens, skeleton loaders appear | Data populates within 5s |
| DT-02 | Firebase disconnect | WiFi disabled on device | Amber offline banner appears, last data shown |
| DT-03 | Reconnect | WiFi re-enabled | Banner disappears, data updates resume |
| DT-04 | Mode change AUTO→MANUAL | Click MANUAL in mode selector | Mode chip updates within 6s (2 poll cycles) |
| DT-05 | Pump start in MANUAL | Click Start Pump | `is_running: true` within 6s |
| DT-06 | E-stop | Click Emergency Stop → Confirm | Pump stops, STOPPED chip, Reset button appears |
| DT-07 | E-stop reset | Click Reset → Confirm | System returns to normal mode |
| DT-08 | Error clear | Trigger dry-run error, clear via dashboard | Error card disappears within 6s |
| DT-09 | Settings save | Change pump_start_level, save | ESP32 applies within 30s (config poll cycle) |
| DT-10 | Invalid settings | Set start ≥ stop, attempt save | Inline validation error, save blocked |
| DT-11 | Countdown mode | Set 5 min countdown, start | Countdown chip shows, counts down |
| DT-12 | Cooldown state | Let pump stop, confirm off-timer | COOLDOWN chip shows, countdown runs |
| DT-13 | Log level change | Set to DEBUG via diagnostics | ESP32 serial shows [D] messages within 30s |
| DT-14 | Mobile layout (375px) | Resize to 375px | E-stop visible, no horizontal scroll |
| DT-15 | Dark mode | Toggle theme | All elements readable, no invisible text |
| DT-16 | PWA install | Open on Android Chrome over HTTPS | "Add to Home Screen" prompt appears |
| DT-17 | Keyboard nav | Tab through all controls | Focus rings visible on every stop |
| DT-18 | Level estimate | Enable level bypass | `~82%` prefix, amber SVG fill |
| DT-19 | Idle mode badge | Tank full, pump off 5+ min | `Idle` badge appears in header |
| DT-20 | Flow bypass warning | Enable flow bypass | Alert card shows bypass warning |

## D8.2 — Lighthouse Audit

Run Lighthouse in Chrome DevTools on the deployed Vercel URL:

| Metric | Target |
|--------|--------|
| Performance | ≥ 80 |
| Accessibility | ≥ 95 |
| Best Practices | ≥ 90 |
| PWA | ≥ 80 |
| First Contentful Paint | < 2.0s on 3G |
| Time to Interactive | < 4.0s on 3G |

## D8.3 — Phase D8 Exit Criteria

- [ ] All 20 integration tests pass
- [ ] Lighthouse Accessibility score ≥ 95
- [ ] Lighthouse PWA score ≥ 80
- [ ] No console errors during normal operation
- [ ] No memory leaks (Chrome DevTools Memory → Record heap snapshots, no growing trend)

---

---

# Rebranding Checklist

Apply after all functional changes are complete. String substitution only — no logic changes.

| File | Find | Replace |
|------|------|---------|
| `app/layout.tsx` | title in metadata | `SmartFlow` |
| `components/Header.tsx` | brand text | `SmartFlow` |
| `public/manifest.json` | `name`, `short_name` | `SmartFlow` |
| `public/manifest.json` | `description` | `Automated water pump controller — Leon, Iloilo` |
| `public/manifest.json` | `theme_color` | `#185FA5` |
| `package.json` | `name` field | `smartflow` |
| Any `<title>` or `<meta name="description">` | old name | SmartFlow |
| Any visible heading or label | `Smart Water Pump Controller` | `SmartFlow` |

Verify with: `grep -r "Smart Water Pump Controller" dashboard/` — must return zero results.

---

---

# Acceptance Criteria Matrix

| Phase | Deliverable | Acceptance criteria |
|-------|-------------|---------------------|
| D0 | Audit report | `docs/audit/dashboard_audit_2026.md` complete, all 9 sections |
| D1 | Type system | `lib/types.ts` complete, `tsc --noEmit` clean, zero `any` in new files |
| D2 | Bug fixes | Zero memory leak patterns, all writes have pending state, settings validates |
| D3 | Design system | `sf-*` tokens in Tailwind, Geist loading, dark mode working |
| D4 | Components | All cards built, skeletons, error boundaries, E-stop pinned mobile |
| D5 | New fields | All 6 gap items have UI, bypass warnings present |
| D6 | Settings | Inline validation, all new fields present, save success message |
| D7 | PWA + a11y | PWA installable, a11y ≥ 95, keyboard nav complete, mobile layout pass |
| D8 | Integration | All 20 DT-xx tests pass, no console errors |
| All | Rebranding | `grep` for old name returns zero results |

---

---

# AI Agent Prompt

Copy the block below and use it as your prompt to an AI coding agent with full access
to the dashboard source tree.

---

```
You are a senior Next.js and Firebase engineer working on SmartFlow — an IoT pump
controller dashboard for a 1.5HP water pump in Leon, Iloilo, Philippines. The dashboard
is a Next.js 14 App Router PWA connected to Firebase RTDB, displaying live pump and
sensor data from an ESP32 microcontroller.

You are executing the SmartFlow Dashboard Refactor Plan v1.0. You are methodical,
precise, and conservative. You fix what is confirmed broken. You do not add features
not in the plan. You never break working functionality.

=============================================================================
CRITICAL CONTEXT — READ BEFORE ANYTHING ELSE
=============================================================================

The dashboard is a SAFETY MONITORING INTERFACE for physical infrastructure.
The pump it controls is a 220V AC motor. These rules are non-negotiable:

1. Emergency stop must ALWAYS be visible and ALWAYS be functional.
   Never hide, disable permanently, or add routing that could prevent E-stop access.

2. Safety-critical controls (E-stop, mode change, bypass toggles) must have a
   confirmation step or pending state. A misclick must not immediately affect the pump.

3. Firebase writes to /pump_system/control/ directly affect the running pump.
   Every write path must have: pending state (UI feedback) + error handling (try/catch).
   Never write to control without both.

4. Do not write Firebase fields that the firmware does not read.
   The canonical Firebase schema is in the refactor plan §4.2. Any field written to
   Firebase that is not in that schema will be silently ignored by the firmware.

5. Offline state must be visible. When Firebase disconnects, show an amber banner with
   last-known data. Never show blank UI or crash on Firebase disconnect.

=============================================================================
MANDATORY PHASE D0 — RESEARCH FIRST. NO CODE UNTIL D0 IS COMPLETE.
=============================================================================

Before modifying any file, complete all research tasks and produce the audit report.

TASK D0.1 — FILE INVENTORY
Read every file in:
  dashboard/app/           — Next.js App Router pages and layouts
  dashboard/components/    — All UI components
  dashboard/lib/           — Firebase client, types, hooks, utilities
  dashboard/public/        — manifest.json, icons
  dashboard/tailwind.config.ts
  dashboard/package.json
  dashboard/tsconfig.json

For each file: path, responsibility, dependencies, any TODO/FIXME/@ts-ignore,
any 'any' type usage, any "Smart Water Pump Controller" string.

TASK D0.2 — COMPONENT TREE
Map every component to what Firebase data it reads and writes:
  ComponentName.tsx → reads: [path.field, ...] | writes: [path.field, ...]

TASK D0.3 — FIREBASE LISTENER AUDIT
For every onValue() listener:
- Is there an unsubscribe in the useEffect cleanup? (memory leak if not)
- Is snap.val() accessed with null checks? (runtime error if not)
- Is the data typed or cast as 'any'? (type safety issue)

TASK D0.4 — FIREBASE WRITE AUDIT
For every set()/update()/push() call:
- Is there a pending state (controls disabled during write)? (UX bug if not)
- Is there error handling (try/catch)? (silent failure if not)

TASK D0.5 — TYPESCRIPT AUDIT
Count and list every 'any' usage, @ts-ignore, and untyped Firebase snapshot access.

TASK D0.6 — UI/UX BUG INVENTORY
Document every observable bug with severity: Critical / High / Medium / Low.
Look specifically for:
- Components with no loading state (no skeleton, no spinner)
- Components with no error boundary
- Settings that allow invalid values (start level >= stop level)
- Controls clickable while Firebase write is pending
- E-stop hidden on mobile
- Missing ARIA labels on interactive elements

TASK D0.7 — DEPENDENCY AUDIT
Run: npm audit --json (in dashboard/)
List all critical and high severity findings.
Check: Next.js ≥14.2.x, Firebase JS SDK ≥10.x, Tailwind ≥3.4.x, TypeScript ≥5.x

TASK D0.8 — FIREBASE SCHEMA GAP TABLE
Compare what the dashboard currently reads/writes against the canonical schema.
Fields to verify are displayed (new from firmware refactor):
  pump_cooldown_remaining_sec — needs countdown display in run mode chip
  manual_runtime_warning      — needs non-blocking amber alert
  bypass_flow_sensor          — needs toggle in settings + alert when active
  is_idle_mode                — needs badge in header
  debug_log_level             — needs read/write control in diagnostics
  remote_level_discard_count  — needs display in diagnostics
  run_mode: AUTO_COOLDOWN / MANUAL_COOLDOWN — needs chip mapping

TASK D0.9 — AUDIT REPORT
Produce docs/audit/dashboard_audit_2026.md with all 9 sections.
State "Phase D0 complete" before writing any code.

=============================================================================
IMPLEMENTATION SEQUENCE — IN ORDER AFTER D0
=============================================================================

State which phase you are executing at the start of each work block.
Complete exit criteria before starting the next phase.

PHASE D1 — TYPE SYSTEM (do before any component work)

Create dashboard/lib/types.ts with:
  - PumpStatus interface — all fields from canonical schema §4.2 status table
  - PumpControl interface — all fields from control table
  - DeviceConfig interface — all fields from config table
  - RunMode type union — all 8 valid values
  - FaultCode type union — all 8 fault codes
  - LogLevel type (0|1|2|3|4)
  - DEFAULT_STATUS constant with all fields defaulted to safe values

Create dashboard/lib/usePumpData.ts:
  - Single hook returning {status, control, config, isLoading, isConnected, lastUpdatedAt, error}
  - onValue listeners for status, control, config, .info/connected paths
  - REQUIRED: all listeners unsubscribed in useEffect cleanup
  - All snap.val() results typed, null-safe

Create dashboard/lib/pumpActions.ts:
  - setMode(mode: ControlMode)
  - setManualDesired(desired: boolean)
  - triggerEmergencyStop()
  - resetEmergencyStop()
  - clearError()
  - startCountdown(durationMin: number)
  - addCountdownTime(addMin: number)
  - setBypassLevel(bypass: boolean)
  - setBypassFlow(bypass: boolean)
  - setRemoteLogLevel(level: LogLevel)
  - requestReboot(currentId: number)
  All return Promise<void> and throw on error. Callers handle pending state.

PHASE D2 — BUG FIXES (fix only confirmed bugs from D0)

For each memory leak (no listener cleanup): add cleanup function.
For each unguarded snap.val(): add null check and typed cast.
For each write without pending state: add isPending state + disable controls.
For each write without error handling: wrap in try/catch.
Settings form: implement validateDeviceConfig() blocking invalid saves.
Comment every fix: // REFACTOR [D2-<description>]: what changed and why

PHASE D3 — DESIGN SYSTEM

tailwind.config.ts: Add complete sf.* color token set (blue, teal, amber, red, green, gray).
Typography: Install Geist + Geist Mono via Next.js localFont, self-hosted in public/fonts/.
app/layout.tsx: Apply font variables to <html> element.
globals.css: Add .card, .card-header, .status-chip, .btn-primary, .btn-danger, .btn-ghost,
             .skeleton component classes. CSS custom properties for --page-bg, --card-bg, etc.
lib/useTheme.ts: Dark mode hook using 'class' strategy, persisted to localStorage.
Dark mode colors: --page-bg, --card-bg, --card-border for dark class.
Verify: Geist loads from /fonts/ (no external network request), dark mode persists on reload.

PHASE D4 — COMPONENT REBUILD

For each component: read existing implementation first, preserve working logic, replace
layout/styling/data handling. Comment: // REFACTOR [D4]: component rebuilt to SmartFlow spec

Header.tsx:
- Sticky, sf-blue wordmark "SmartFlow"
- Right: connection indicator, RSSI badge (colored by strength), last-updated time, theme toggle
- Offline banner: full-width amber, shows when isConnected===false, NOT dismissable

TankLevelCard.tsx:
- Animated SVG fill (transition 0.6s ease-in-out)
- Color: 0-20% sf-red, 20-50% sf-amber, 50-100% sf-teal
- Level % in large font-sans font-bold
- Distance in font-mono text-sm
- Start/stop level markers as dashed SVG lines
- Level estimate state: tilde prefix, sf-amber fill, "Flow estimate" sub-label
- Status row: level_fresh and is_sensor_error as colored dots
- Loading skeleton for when status===null

PumpStatusCard.tsx:
- RunMode chip using MODE_LABELS constant for all 8 modes
- Cooldown countdown: setInterval client-side from pump_cooldown_remaining_sec initial value
- Flow rate in font-mono, em-dash when not running
- Uptime formatted Xh Ym

ControlPanel.tsx:
- Mode selector: three-way segmented (AUTO/MANUAL/TIMER)
- MANUAL pump toggle: only when mode===MANUAL
- Countdown: only when mode===COUNTDOWN, number input + Start + +5min buttons
- E-stop: always visible, inline confirmation popover (no modal), btn-danger
- Reset: visible when emergency_stop_latched===true, inline confirmation popover
- All controls disabled when isPending===true
- Mobile: fixed bottom bar with E-stop (md:hidden)

AlertsCard.tsx:
- Priority-ordered alert list (emergency_stop > is_error > sensor errors > warnings)
- Empty state: "System operating normally"
- manual_runtime_warning: non-blocking amber, no dismiss, no pump action
- bypass warnings: informational amber chips

DiagnosticsCard.tsx:
- Collapsible (closed by default)
- System, RS-485, Firebase sections
- Log level segmented control (reads status.debug_log_level, writes config/device/debug_log_level)
- remote_level_discard_count in amber when > 0
- Warning when log level set above INFO

React error boundary: Wrap every card in an ErrorBoundary component showing
"Unable to load [section name]" with a Retry button.

PHASE D5 — NEW FIELD COMPONENTS

After D4 is complete, verify and implement all 6 gap items from D0.8:
bypass_flow_sensor: settings toggle + AlertsCard warning when active
is_idle_mode: header badge (amber "Idle") with tooltip
debug_log_level: DiagnosticsCard segmented control with pending state note
remote_level_discard_count: DiagnosticsCard row, amber when > 0
pump_cooldown_remaining_sec: PumpStatusCard countdown (may already be done in D4)
level_estimate_active: TankLevelCard tilde + amber (may already be done in D4)

PHASE D6 — SETTINGS HARDENING

Settings page sections: Tank Calibration, Pump Protection, Flow Calibration,
Sleep Schedule, Sensor Thresholds, Advanced Controls, Notifications.
Include new fields: bypass_flow_sensor toggle, debug_log_level selector.
Create lib/validateSettings.ts with validateDeviceConfig() — all rules listed in plan.
Show validation errors inline next to each field.
Save pattern: validate → setIsSaving → try update() → success message → catch error → finally setIsSaving(false)
Success message: "Settings saved. Firmware will apply within 30 seconds."

PHASE D7 — PWA & ACCESSIBILITY (run concurrently with D4-D6)

manifest.json: name=SmartFlow, short_name=SmartFlow, theme_color=#185FA5,
               background_color=#F1EFE8, display=standalone, icons 192+512
ARIA: aria-label on all icon buttons, E-stop button, bypass toggles
Focus: :focus-visible rings on all interactive elements (from globals.css)
Mobile: E-stop pinned to bottom on mobile (fixed bottom-0, md:hidden)
Live region: role="status" aria-live="polite" for pump state change announcements
Form labels: every input has associated <label> with htmlFor

PHASE D8 — INTEGRATION TESTING

Run all 20 DT-xx tests against live Firebase.
Run Lighthouse: target Accessibility ≥ 95, PWA ≥ 80.
Verify no console errors, no memory leaks.

REBRANDING (final pass after all functional changes)

Replace all "Smart Water Pump Controller" with "SmartFlow" in visible strings.
Verify: grep -r "Smart Water Pump Controller" dashboard/ returns zero results.

=============================================================================
UNIVERSAL RULES
=============================================================================

1. READ BEFORE WRITING. Read every file before modifying it.

2. COMMENT EVERY CHANGE.
   // REFACTOR [D<phase>]: description of what changed and why

3. SAFETY CONTROLS NEVER BREAK. Emergency stop, clear error, and reset paths
   must work in every state. Never add conditional rendering that could hide E-stop.

4. FIREBASE WRITES HAVE TWO REQUIREMENTS: pending state + error handling. Both.
   Never write to Firebase without both.

5. NULL SAFETY EVERYWHERE. Every Firebase snap.val() is nullable. Every nested
   field access uses optional chaining (status?.water_level_percent ?? defaultValue).

6. NO SCOPE CREEP. If you find something improvable but not in the plan,
   document it in docs/audit/out_of_scope_findings.md and do not implement it.

7. PHASE GATES. Report exit criteria met before starting the next phase.

=============================================================================
OUTPUT FORMAT
=============================================================================

For each file created or modified:
  1. State the full file path
  2. Explain what changed and why (cite phase and any bug ID)
  3. Provide complete file content or precise diff for large files

When a phase completes:
  PHASE D[N] COMPLETE
  Files modified: [list]
  Exit criteria met: [list each with confirmation]
  Out-of-scope findings: [list or "none"]

Begin with Phase D0. Do not write any code until the audit report is complete.
```

---

*SmartFlow Dashboard Refactor Plan v1.0*
*Engineering basis: Nielsen Heuristics (1994) — visibility of system status, error prevention,
user control and freedom; WCAG 2.1 AA (W3C, 2018); Google PWA checklist (2024);
Firebase RTDB best practices — listener cleanup, offline persistence, type safety.*
*All Firebase field references verified against SmartFlow System Refactor Plan v2.0 §4.2.*
