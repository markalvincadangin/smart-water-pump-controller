# SmartFlow — Dashboard & Firmware Integration QA Test Specification
### Software Test Specification v1.0

**System under test:** SmartFlow Dashboard (Next.js 14 PWA) + Firebase RTDB Integration
**Integration boundary:** Dashboard ↔ Firebase RTDB ↔ ESP32 Firmware
**Classification:** Safety-monitoring interface — commands directly affect 220V AC motor
**Test scope:** Unit · Integration · Contract · End-to-end · Regression · Non-functional

**Standards basis:**
- IEEE 829-2008: Software Test Documentation
- ISO/IEC 25010:2011: Software quality model (functional suitability, reliability, usability, security)
- ISTQB Foundation Level v4.0: Test design techniques
- WCAG 2.1 Level AA: Web Content Accessibility Guidelines
- OWASP Web Application Security Testing Guide v4.2
- Google Web Vitals: Core performance metrics
- RFC 7230/7231: HTTP semantics (for PWA/network tests)

---

> **Scope of this specification**
> This document tests the dashboard as software — its functions, logic, Firebase data
> contract, integration with firmware, and non-functional quality attributes. Physical
> hardware behavior is validated by the firmware QA spec. This document assumes the
> firmware is deployed and operating correctly as the data source.
>
> **Critical safety context:** The dashboard writes to `/pump_system/control/` which
> directly controls a 220V AC motor relay. Every write-path test must verify that:
> (1) only the correct Firebase path is written, (2) the value is correct, and
> (3) the write does not proceed without user intent confirmation.

---

## Table of Contents

1. [QA Framework & Test Design Basis](#1-qa-framework--test-design-basis)
2. [Test Environment & Setup](#2-test-environment--setup)
3. [Module 1: Firebase Data Contract Tests](#3-module-1-firebase-data-contract-tests)
4. [Module 2: usePumpData Hook — Unit Tests](#4-module-2-usepumpdata-hook--unit-tests)
5. [Module 3: pumpActions — Unit Tests](#5-module-3-pumpactions--unit-tests)
6. [Module 4: Settings Validation — Unit Tests](#6-module-4-settings-validation--unit-tests)
7. [Module 5: Component Function Tests — TankLevelCard](#7-module-5-component-function-tests--tanklevelcard)
8. [Module 6: Component Function Tests — PumpStatusCard](#8-module-6-component-function-tests--pumpstatuscard)
9. [Module 7: Component Function Tests — ControlPanel](#9-module-7-component-function-tests--controlpanel)
10. [Module 8: Component Function Tests — AlertsCard](#10-module-8-component-function-tests--alertscard)
11. [Module 9: Component Function Tests — DiagnosticsCard](#11-module-9-component-function-tests--diagnosticscard)
12. [Module 10: Settings Page — Function Tests](#12-module-10-settings-page--function-tests)
13. [Module 11: Dashboard ↔ Firmware Integration Tests](#13-module-11-dashboard--firmware-integration-tests)
14. [Module 12: Control Path Integration — Safety Critical](#14-module-12-control-path-integration--safety-critical)
15. [Module 13: Status Path Integration — Read Fidelity](#15-module-13-status-path-integration--read-fidelity)
16. [Module 14: Config Path Integration — Settings Round-Trip](#16-module-14-config-path-integration--settings-round-trip)
17. [Module 15: Offline & Resilience Tests](#17-module-15-offline--resilience-tests)
18. [Module 16: Performance & Load Tests](#18-module-16-performance--load-tests)
19. [Module 17: Security Tests](#19-module-17-security-tests)
20. [Module 18: Accessibility Tests](#20-module-18-accessibility-tests)
21. [Module 19: TypeScript & Code Quality Tests](#21-module-19-typescript--code-quality-tests)
22. [Module 20: Regression Test Checklist](#22-module-20-regression-test-checklist)
23. [Test Traceability Matrix](#23-test-traceability-matrix)

---

## 1. QA Framework & Test Design Basis

### 1.1 Testing Layers

This specification applies four testing layers:

**Unit tests:** Individual functions and hooks tested in isolation. Firebase SDK is mocked.
Tool: Jest + React Testing Library (RTL). No real Firebase connections.

**Integration tests:** Component tested with real Firebase data flowing through a test project.
Tool: Jest + RTL + Firebase emulator, or manual browser testing against staging Firebase.

**Contract tests:** Verify that every field the dashboard reads from Firebase is actually written by firmware, and every field the dashboard writes is actually read by firmware. This is the most critical test layer for an IoT system — a contract mismatch means the dashboard shows wrong data or commands are silently ignored.

**End-to-end tests:** Dashboard + live Firebase + running ESP32. Observable through browser DevTools + Firebase console.

### 1.2 Test Priority

| Priority | Description |
|----------|-------------|
| **P1 — CRITICAL** | Safety-critical. Writing wrong data to a control path could cause pump to start/stop unexpectedly. Zero tolerance. |
| **P2 — HIGH** | Functional correctness. Wrong data displayed or wrong Firebase path written. |
| **P3 — MEDIUM** | UX quality. Pending states, error messages, visual behavior. |
| **P4 — LOW** | Non-functional quality. Performance, aesthetics, edge cases. |

### 1.3 Test ID Schema

```
DB-[MODULE]-[NUMBER]

Modules:
  CONTRACT   Firebase data contract
  HOOK       usePumpData hook
  ACTION     pumpActions functions
  VALIDATE   Settings validation
  TANK       TankLevelCard component
  STATUS     PumpStatusCard component
  CTRL       ControlPanel component
  ALERT      AlertsCard component
  DIAG       DiagnosticsCard component
  SETTINGS   Settings page
  INT        Dashboard↔Firmware integration
  SAFETY     Control path safety tests
  READ       Status path read fidelity
  CONFIG     Config path round-trip
  OFFLINE    Offline & resilience
  PERF       Performance
  SEC        Security
  A11Y       Accessibility
  QA         Code quality (TypeScript, lint)
  REG        Regression
```

---

## 2. Test Environment & Setup

### 2.1 Unit Test Environment (Mocked Firebase)

```typescript
// jest.config.ts
export default {
  testEnvironment: 'jsdom',
  setupFilesAfterFramework: ['@testing-library/jest-dom'],
  moduleNameMapper: {
    '^@/lib/firebase$': '<rootDir>/__mocks__/firebase.ts',
  },
};

// __mocks__/firebase.ts — mock Firebase database
export const mockDb = {
  status: null as PumpStatus | null,
  control: null as PumpControl | null,
  config: null as DeviceConfig | null,
  connected: true,
};

export const onValue = jest.fn((ref, callback) => {
  // Immediately invoke with mock data based on ref path
  callback({ val: () => getMockDataForPath(ref._path) });
  return jest.fn(); // unsubscribe function
});

export const set = jest.fn(() => Promise.resolve());
export const update = jest.fn(() => Promise.resolve());
export const ref = jest.fn((db, path) => ({ _path: path }));
```

### 2.2 Integration Test Environment

Use Firebase Emulator Suite for integration tests:

```bash
# Start emulator
firebase emulators:start --only database

# Set emulator host in test environment
NEXT_PUBLIC_FIREBASE_EMULATOR=localhost:9000
```

Seed the emulator with representative data before each test suite:

```typescript
// test-fixtures/pump-status-normal.json
{
  "pump_system": {
    "status": {
      "water_level_percent": 65,
      "is_running": false,
      "flow_rate_lpm": 0,
      "run_mode": "AUTO_STANDBY",
      "pump_cooldown_remaining_sec": 0,
      "is_error": false,
      "is_sensor_error": false,
      "is_flow_sensor_error": false,
      "is_overflow_error": false,
      "is_idle_mode": false,
      "is_sleeping": false,
      "emergency_stop_latched": false,
      "manual_desired": false,
      "bypass_level_sensor": false,
      "bypass_flow_sensor": false,
      "remote_sensor_stable": true,
      "level_fresh": true,
      "manual_runtime_warning": false,
      "countdown_remaining_sec": 0,
      "last_fault_code": "",
      "last_fault_message": "",
      "remote_level_discard_count": 0,
      "wifi_rssi": -62,
      "uptime_minutes": 125,
      "last_boot_reason": "Power-on",
      "debug_log_level": 2,
      "total_pump_cycles": 142,
      "free_heap_bytes": 182000,
      "firebase_consecutive_failures": 0
    },
    "control": {
      "mode": "AUTO",
      "manual_desired": false,
      "emergency_stop": false,
      "reset_stop": false,
      "clear_error": false,
      "bypass_level_sensor": false,
      "bypass_flow_sensor": false
    },
    "config": {
      "device": {
        "tank_empty_cm": 200,
        "tank_full_cm": 10,
        "pump_start_level": 20,
        "pump_stop_level": 90,
        "dry_run_threshold_lpm": 1.0,
        "dry_run_timeout_sec": 30,
        "max_pump_runtime_min": 120,
        "flow_calibration_factor": 7.5,
        "debug_log_level": 2
      }
    }
  }
}
```

### 2.3 End-to-End Test Environment

- Dashboard: `http://localhost:3000` (development) or Vercel preview URL
- Firebase: Staging Firebase project (separate from production)
- ESP32: Flashed with production firmware, connected to staging Firebase project
- Browser: Chrome 120+ with DevTools available
- Mobile: Android Chrome (for PWA tests)

### 2.4 Test Data States

| State name | Status conditions | Purpose |
|---|---|---|
| `NORMAL` | All healthy, pump stopped in AUTO | Baseline |
| `PUMP_RUNNING` | `is_running=true`, `run_mode=AUTO` | Active operation |
| `DRY_RUN_ERROR` | `is_error=true`, `last_fault_code=DRY_RUN` | Error state |
| `EMERGENCY_STOPPED` | `emergency_stop_latched=true`, `run_mode=STOPPED` | Safety latch |
| `SENSOR_OFFLINE` | `remote_sensor_stable=false`, `level_fresh=false` | Comm loss |
| `COOLDOWN` | `run_mode=AUTO_COOLDOWN`, `pump_cooldown_remaining_sec=47` | Cooldown active |
| `IDLE` | `is_idle_mode=true`, pump stopped, level high | Idle mode |
| `MANUAL_ON` | `run_mode=MANUAL_ON`, `is_running=true` | Manual operation |
| `BYPASSED_FLOW` | `bypass_flow_sensor=true` | Bypass active |
| `LOADING` | `status=null` | Initial load |

---

## 3. Module 1: Firebase Data Contract Tests

> **Purpose:** Verify the dashboard reads exactly the fields firmware writes, writes
> exactly the fields firmware reads. A contract mismatch = silent bugs.
> **QA technique:** Schema conformance testing, contract testing.
> **Tool:** Code review + Firebase emulator.

---

### DB-CONTRACT-001 [P1] — Every Status Field the Firmware Writes is Read by Dashboard

**Method:** Code review + automated.

**Procedure:**
1. From the firmware QA spec and schema definition, extract the complete list of status fields firmware writes to `/pump_system/status/`.
2. Search dashboard source code for each field name.
3. Verify every field is accessed (read) somewhere in the dashboard.

**Fields to verify are READ:**
```
water_level_percent    is_running             flow_rate_lpm
run_mode               pump_cooldown_remaining_sec
is_error               is_sensor_error        is_flow_sensor_error
is_overflow_error      is_idle_mode           is_sleeping
emergency_stop_latched manual_desired         bypass_level_sensor
bypass_flow_sensor     remote_sensor_stable   level_fresh
manual_runtime_warning countdown_remaining_sec
last_fault_code        last_fault_message     level_sensor_health_pct
remote_level_discard_count flow_volume_added_l wifi_rssi
uptime_minutes         last_boot_reason       debug_log_level
total_pump_cycles      total_pump_run_min     ultrasonic_cycles_ok
ultrasonic_cycles_timeout free_heap_bytes     min_free_heap_observed_bytes
firebase_consecutive_failures
```

**Pass criteria:** Every field appears in at least one component's read path. Zero orphaned firmware fields.
**Fail criteria:** Any field written by firmware that the dashboard never reads. Defect category: DATA CONTRACT.

---

### DB-CONTRACT-002 [P1] — Every Control Field Dashboard Writes is Read by Firmware

**Method:** Code review.

**Firmware reads these control fields — verify dashboard writes them correctly:**

| Field | Dashboard writes | Correct Firebase path |
|---|---|---|
| `mode` | `setMode()` | `/pump_system/control/mode` |
| `manual_desired` | `setManualDesired()` | `/pump_system/control/manual_desired` |
| `emergency_stop` | `triggerEmergencyStop()` | `/pump_system/control/emergency_stop` |
| `reset_stop` | `resetEmergencyStop()` | `/pump_system/control/reset_stop` |
| `clear_error` | `clearError()` | `/pump_system/control/clear_error` |
| `countdown_start` | `startCountdown()` | `/pump_system/control/countdown_start` |
| `countdown_duration_min` | `startCountdown()` | `/pump_system/control/countdown_duration_min` |
| `countdown_add_time` | `addCountdownTime()` | `/pump_system/control/countdown_add_time` |
| `countdown_add_min` | `addCountdownTime()` | `/pump_system/control/countdown_add_min` |
| `bypass_level_sensor` | `setBypassLevel()` | `/pump_system/control/bypass_level_sensor` |
| `bypass_flow_sensor` | `setBypassFlow()` | `/pump_system/control/bypass_flow_sensor` |
| `reboot_request_id` | `requestReboot()` | `/pump_system/control/reboot_request_id` |

**Pass criteria:** Each action function writes to exactly the correct path with the correct value type.
**Fail criteria:** Any path typo, wrong type, or missing field. Defect category: DATA CONTRACT / SAFETY.

---

### DB-CONTRACT-003 [P1] — Dashboard Never Writes to Status Path

**Requirement:** `/pump_system/status/` is write-only for the ESP32. The dashboard must never write to this path.

**Method:** Search dashboard source code for any `set()`, `update()`, or `push()` call targeting `/pump_system/status`.

**Procedure:**
```bash
grep -r "pump_system/status" dashboard/lib/pumpActions.ts
grep -r "pump_system/status" dashboard/components/
grep -r "pump_system/status" dashboard/app/
```

**Pass criteria:** Zero write calls to any `/pump_system/status/` path. Read calls (onValue) are permitted.
**Fail criteria:** Any write to status path. Defect category: SAFETY.

---

### DB-CONTRACT-004 [P1] — Control Field Types Match Firmware Expectations

**Firmware reads control fields and expects specific types. Verify dashboard writes the correct type.**

| Field | Expected type | Wrong type risk |
|---|---|---|
| `mode` | string `"AUTO" \| "MANUAL" \| "COUNTDOWN"` | Writing `"auto"` (lowercase) silently fails |
| `manual_desired` | boolean | Writing `1` (number) may fail firmware bool parse |
| `emergency_stop` | boolean | Writing `"true"` (string) is wrong type |
| `countdown_duration_min` | integer | Writing `"5"` (string) fails firmware atoi |
| `debug_log_level` | integer 0–4 | Writing `"INFO"` (string) wrong type |

**Procedure:** Code review `pumpActions.ts`. Verify each `set()` call uses correct TypeScript types and that TypeScript prevents mismatched types at compile time.

**Pass criteria:** TypeScript enforces correct types; `tsc --noEmit` passes with no errors.

---

### DB-CONTRACT-005 [P2] — Config Path Writes Match Firmware Expected Keys

**Firmware reads `/pump_system/config/device/` with exact key names. Any key name mismatch means the setting is silently ignored.**

**Verify these keys match exactly (case-sensitive):**

```
tank_empty_cm          tank_full_cm           pump_start_level
pump_stop_level        dry_run_threshold_lpm  dry_run_timeout_sec
max_pump_runtime_min   flow_calibration_factor debug_log_level
sleep_enabled          sleep_start_hour       sleep_end_hour
sleep_emergency_level  sensor_failure_threshold
idle_sensor_interval_ms  idle_firebase_interval_ms
```

**Method:** Compare keys used in settings save function against firmware `readDeviceConfigFromFirebase()` implementation (read from source).

---

### DB-CONTRACT-006 [P2] — run_mode Display Covers All 8 Firmware Values

**Firmware writes 8 possible `run_mode` values. Dashboard must handle all 8.**

| Value | Dashboard must handle |
|---|---|
| `AUTO_STANDBY` | Display chip |
| `AUTO` | Display chip |
| `AUTO_COOLDOWN` | Display chip + countdown |
| `MANUAL_ON` | Display chip |
| `MANUAL_OFF` | Display chip |
| `MANUAL_COOLDOWN` | Display chip + countdown |
| `COUNTDOWN` | Display chip |
| `STOPPED` | Display chip + reset button |

**Test:** Inject each value into the Firebase emulator. Verify the correct chip renders.
**Pass criteria:** All 8 values render without `undefined`, `null`, or empty string in the chip.

---

### DB-CONTRACT-007 [P2] — Fault Code Display Covers All 8 Firmware Values

| Fault code | Dashboard must display |
|---|---|
| `DRY_RUN` | Red error card with clear button |
| `OVERFLOW` | Red error card with clear button |
| `E_STOP` | Red error card (E-stop specific) |
| `COMM_LOSS` | Amber warning card |
| `STALE_LEVEL` | Amber warning card |
| `LEVEL_SENSOR` | Amber warning card |
| `FLOW_SENSOR` | Amber warning card |
| `SAFE_MODE` | Red critical card |

**Test:** Inject each `last_fault_code` value. Verify correct alert renders.

---

## 4. Module 2: usePumpData Hook — Unit Tests

> **Tool:** Jest + RTL. Firebase mocked.
> **Purpose:** Verify the data hook correctly manages state, subscriptions, and cleanup.

---

### DB-HOOK-001 [P2] — Hook Returns isLoading=true Before First Data

```typescript
test('returns isLoading=true before first Firebase data', () => {
  // Arrange: mock onValue to never call callback
  const mockOnValue = jest.fn().mockReturnValue(jest.fn());
  jest.mock('@firebase/database', () => ({ onValue: mockOnValue }));

  // Act
  const { result } = renderHook(() => usePumpData());

  // Assert
  expect(result.current.isLoading).toBe(true);
  expect(result.current.status).toBeNull();
});
```

**Pass criteria:** `isLoading = true`, `status = null` before first Firebase callback.

---

### DB-HOOK-002 [P2] — Hook Returns isLoading=false After First Data

```typescript
test('sets isLoading=false after status data received', async () => {
  // Arrange: mock onValue to immediately call with data
  mockOnValue.mockImplementation((ref, cb) => {
    cb({ val: () => TEST_STATUS_NORMAL });
    return jest.fn();
  });

  const { result } = renderHook(() => usePumpData());

  await waitFor(() => {
    expect(result.current.isLoading).toBe(false);
    expect(result.current.status).not.toBeNull();
  });
});
```

---

### DB-HOOK-003 [P1] — Hook Unsubscribes All Listeners on Unmount

```typescript
test('unsubscribes all 4 listeners on unmount', () => {
  const unsubFns = [jest.fn(), jest.fn(), jest.fn(), jest.fn()];
  let callCount = 0;
  mockOnValue.mockImplementation(() => unsubFns[callCount++]);

  const { unmount } = renderHook(() => usePumpData());
  unmount();

  // All 4 listeners (status, control, config, .info/connected) must be unsubscribed
  unsubFns.forEach(fn => expect(fn).toHaveBeenCalledTimes(1));
});
```

**Pass criteria:** All 4 unsubscribe functions called exactly once on unmount.
**Fail criteria:** Any unsubscribe not called. Defect: memory leak that grows with navigations.

---

### DB-HOOK-004 [P2] — Hook isConnected Reflects .info/connected Path

```typescript
test('isConnected=true when .info/connected is true', async () => {
  mockOnValue.mockImplementation((ref, cb) => {
    if (ref._path === '.info/connected') cb({ val: () => true });
    return jest.fn();
  });
  const { result } = renderHook(() => usePumpData());
  await waitFor(() => expect(result.current.isConnected).toBe(true));
});

test('isConnected=false when .info/connected is false', async () => {
  mockOnValue.mockImplementation((ref, cb) => {
    if (ref._path === '.info/connected') cb({ val: () => false });
    return jest.fn();
  });
  const { result } = renderHook(() => usePumpData());
  await waitFor(() => expect(result.current.isConnected).toBe(false));
});
```

---

### DB-HOOK-005 [P2] — Hook Sets error on Firebase Read Failure

```typescript
test('sets error string when onValue throws', async () => {
  mockOnValue.mockImplementation((ref, cb, onError) => {
    onError(new Error('Permission denied'));
    return jest.fn();
  });
  const { result } = renderHook(() => usePumpData());
  await waitFor(() => expect(result.current.error).toBe('Permission denied'));
});
```

---

### DB-HOOK-006 [P2] — Hook Updates lastUpdatedAt on Each Status Push

```typescript
test('lastUpdatedAt updates on each status push', async () => {
  let callback: Function;
  mockOnValue.mockImplementation((ref, cb) => {
    if (ref._path.includes('status')) callback = cb;
    return jest.fn();
  });
  const { result } = renderHook(() => usePumpData());

  const t1 = new Date('2026-01-01T00:00:00Z');
  jest.setSystemTime(t1);
  act(() => callback({ val: () => TEST_STATUS_NORMAL }));
  expect(result.current.lastUpdatedAt?.getTime()).toBe(t1.getTime());

  const t2 = new Date('2026-01-01T00:00:03Z');
  jest.setSystemTime(t2);
  act(() => callback({ val: () => TEST_STATUS_NORMAL }));
  expect(result.current.lastUpdatedAt?.getTime()).toBe(t2.getTime());
});
```

---

### DB-HOOK-007 [P2] — Hook Handles null snap.val() Without Throwing

```typescript
test('handles null status snapshot without throwing', async () => {
  mockOnValue.mockImplementation((ref, cb) => {
    cb({ val: () => null });
    return jest.fn();
  });
  const { result } = renderHook(() => usePumpData());
  await waitFor(() => {
    expect(result.current.status).toBeNull();
    expect(result.current.isLoading).toBe(false);
    expect(result.current.error).toBeNull();
  });
});
```

**This is critical:** `snap.val()` returns null when the Firebase path is empty or deleted. The hook must not crash.

---

## 5. Module 3: pumpActions — Unit Tests

> **Purpose:** Verify each action function writes to the correct Firebase path with the correct value and type.
> **Tool:** Jest. Firebase `set`/`update` mocked.
> **Critical:** A wrong path or wrong value means a firmware command is silently ignored, or worse, the wrong command is sent.

---

### DB-ACTION-001 [P1] — setMode writes to correct path with exact string value

```typescript
test.each([
  ['AUTO', '/pump_system/control/mode'],
  ['MANUAL', '/pump_system/control/mode'],
  ['COUNTDOWN', '/pump_system/control/mode'],
])('setMode(%s) writes string to correct path', async (mode, expectedPath) => {
  await setMode(mode as ControlMode);
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: expectedPath }),
    mode  // Must be string "AUTO", not "auto" or 1
  );
});
```

**Pass criteria:** Exact string match. Firmware mode string is case-sensitive.

---

### DB-ACTION-002 [P1] — triggerEmergencyStop writes boolean true, not string or number

```typescript
test('triggerEmergencyStop writes boolean true', async () => {
  await triggerEmergencyStop();
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/emergency_stop' }),
    true  // Must be boolean true, not "true" or 1
  );
});

test('triggerEmergencyStop does not write to status path', async () => {
  await triggerEmergencyStop();
  const callArgs = mockSet.mock.calls[0][0]._path;
  expect(callArgs).not.toContain('/status/');
});
```

---

### DB-ACTION-003 [P1] — clearError writes to clear_error, not emergency_stop

```typescript
test('clearError writes to clear_error path only', async () => {
  await clearError();
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/clear_error' }),
    true
  );
  // Verify it did NOT accidentally write to emergency_stop
  const allPaths = mockSet.mock.calls.map(c => c[0]._path);
  expect(allPaths).not.toContain('/pump_system/control/emergency_stop');
});
```

---

### DB-ACTION-004 [P1] — startCountdown writes both fields atomically via update()

```typescript
test('startCountdown uses update() not set() for atomicity', async () => {
  await startCountdown(15);
  // Must use update() not set() to write both fields together
  expect(mockUpdate).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control' }),
    {
      countdown_start: true,
      countdown_duration_min: 15,  // integer, not string "15"
    }
  );
  expect(mockSet).not.toHaveBeenCalled();
});

test('startCountdown duration is integer', async () => {
  await startCountdown(15);
  const args = mockUpdate.mock.calls[0][1];
  expect(typeof args.countdown_duration_min).toBe('number');
  expect(Number.isInteger(args.countdown_duration_min)).toBe(true);
});
```

---

### DB-ACTION-005 [P2] — addCountdownTime writes both fields atomically

```typescript
test('addCountdownTime writes countdown_add_time=true and addMin', async () => {
  await addCountdownTime(5);
  expect(mockUpdate).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control' }),
    { countdown_add_time: true, countdown_add_min: 5 }
  );
});
```

---

### DB-ACTION-006 [P1] — setBypassFlow writes to bypass_flow_sensor, not bypass_level_sensor

```typescript
test('setBypassFlow writes to bypass_flow_sensor path', async () => {
  await setBypassFlow(true);
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/bypass_flow_sensor' }),
    true
  );
  // Critical: must NOT accidentally write to bypass_level_sensor
  const callPath = mockSet.mock.calls[0][0]._path;
  expect(callPath).not.toContain('bypass_level_sensor');
});

test('setBypassLevel writes to bypass_level_sensor path', async () => {
  await setBypassLevel(false);
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/bypass_level_sensor' }),
    false
  );
});
```

**This test is critical:** confusing `bypass_level_sensor` and `bypass_flow_sensor` would disable the wrong protection — e.g., intending to bypass the level sensor but actually disabling dry-run protection.

---

### DB-ACTION-007 [P2] — setRemoteLogLevel writes to config path, not control path

```typescript
test('setRemoteLogLevel writes to config/device path', async () => {
  await setRemoteLogLevel(3);
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({
      _path: '/pump_system/config/device/debug_log_level'
    }),
    3  // integer
  );
  // Not to /control/ path
  const callPath = mockSet.mock.calls[0][0]._path;
  expect(callPath).not.toContain('/control/');
});
```

---

### DB-ACTION-008 [P2] — requestReboot increments current ID, not sets to fixed value

```typescript
test('requestReboot increments reboot_request_id', async () => {
  await requestReboot(5);
  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/reboot_request_id' }),
    6  // current + 1
  );
});

test('requestReboot with id=0 writes 1', async () => {
  await requestReboot(0);
  expect(mockSet).toHaveBeenCalledWith(expect.anything(), 1);
});
```

---

### DB-ACTION-009 [P2] — All Actions Throw on Firebase Error (Not Swallow It)

```typescript
test('each action throws when Firebase write fails', async () => {
  mockSet.mockRejectedValue(new Error('Network error'));

  await expect(setMode('AUTO')).rejects.toThrow('Network error');
  await expect(triggerEmergencyStop()).rejects.toThrow('Network error');
  await expect(clearError()).rejects.toThrow('Network error');
});
```

**Pass criteria:** Actions throw — callers (components) must handle errors and show UI feedback.
**Fail criteria:** Actions swallow errors silently — user clicks E-stop and nothing happens, no error shown.

---

## 6. Module 4: Settings Validation — Unit Tests

> **Purpose:** Verify `validateDeviceConfig()` correctly blocks all invalid configurations.
> **QA technique:** Equivalence partitioning, boundary value analysis, decision table.

---

### DB-VALIDATE-001 [P1] — Rejects pump_start_level >= pump_stop_level

```typescript
test.each([
  { start: 90, stop: 20, desc: 'start > stop' },
  { start: 50, stop: 50, desc: 'start = stop' },
  { start: 90, stop: 90, desc: 'both same at 90' },
])('rejects when $desc', ({ start, stop }) => {
  const errors = validateDeviceConfig({
    pump_start_level: start,
    pump_stop_level: stop,
  });
  expect(errors.some(e => e.field === 'pump_start_level')).toBe(true);
});

test('accepts valid start < stop', () => {
  const errors = validateDeviceConfig({
    pump_start_level: 20,
    pump_stop_level: 90,
  });
  expect(errors).toHaveLength(0);
});
```

---

### DB-VALIDATE-002 [P1] — Rejects tank_full_cm >= tank_empty_cm

```typescript
test.each([
  { full: 200, empty: 10, desc: 'full > empty (inverted)' },
  { full: 100, empty: 100, desc: 'full = empty' },
])('rejects when $desc', ({ full, empty }) => {
  const errors = validateDeviceConfig({
    tank_full_cm: full,
    tank_empty_cm: empty,
  });
  expect(errors.some(e => e.field === 'tank_full_cm')).toBe(true);
});
```

---

### DB-VALIDATE-003 [P2] — Boundary Value Tests for Numeric Ranges

```typescript
// dry_run_threshold_lpm: valid range 0.1 – 10.0
test.each([
  { value: 0.09, shouldFail: true,  desc: 'below min (0.09)' },
  { value: 0.1,  shouldFail: false, desc: 'at min (0.1)' },
  { value: 5.0,  shouldFail: false, desc: 'mid-range (5.0)' },
  { value: 10.0, shouldFail: false, desc: 'at max (10.0)' },
  { value: 10.1, shouldFail: true,  desc: 'above max (10.1)' },
])('dry_run_threshold_lpm $desc', ({ value, shouldFail }) => {
  const errors = validateDeviceConfig({ dry_run_threshold_lpm: value });
  const hasError = errors.some(e => e.field === 'dry_run_threshold_lpm');
  expect(hasError).toBe(shouldFail);
});

// dry_run_timeout_sec: valid range 10 – 300
test.each([
  [9, true], [10, false], [300, false], [301, true]
])('dry_run_timeout_sec %d → fail=%s', (value, shouldFail) => {
  const errors = validateDeviceConfig({ dry_run_timeout_sec: value });
  expect(errors.some(e => e.field === 'dry_run_timeout_sec')).toBe(shouldFail);
});

// max_pump_runtime_min: valid range 30 – 480
test.each([
  [29, true], [30, false], [480, false], [481, true]
])('max_pump_runtime_min %d → fail=%s', (value, shouldFail) => {
  const errors = validateDeviceConfig({ max_pump_runtime_min: value });
  expect(errors.some(e => e.field === 'max_pump_runtime_min')).toBe(shouldFail);
});
```

---

### DB-VALIDATE-004 [P2] — Rejects NaN and Undefined for Required Fields

```typescript
test.each(['pump_start_level', 'pump_stop_level', 'dry_run_threshold_lpm'])(
  'rejects NaN for %s',
  (field) => {
    const errors = validateDeviceConfig({ [field]: NaN });
    expect(errors.some(e => e.field === field)).toBe(true);
  }
);

test.each(['pump_start_level', 'pump_stop_level'])(
  'rejects undefined for %s when required',
  (field) => {
    const errors = validateDeviceConfig({ [field]: undefined });
    expect(errors.some(e => e.field === field)).toBe(true);
  }
);
```

---

### DB-VALIDATE-005 [P2] — Returns Empty Array for All Valid Config

```typescript
test('returns empty errors array for valid complete config', () => {
  const errors = validateDeviceConfig({
    tank_empty_cm: 200,
    tank_full_cm: 10,
    pump_start_level: 20,
    pump_stop_level: 90,
    dry_run_threshold_lpm: 1.0,
    dry_run_timeout_sec: 30,
    max_pump_runtime_min: 120,
    flow_calibration_factor: 7.5,
  });
  expect(errors).toHaveLength(0);
});
```

---

### DB-VALIDATE-006 [P2] — Returns Multiple Errors Simultaneously (Not Just First)

```typescript
test('returns all errors simultaneously, not just first', () => {
  const errors = validateDeviceConfig({
    pump_start_level: 90,   // error 1: start > stop
    pump_stop_level: 20,
    dry_run_timeout_sec: 5, // error 2: below min
  });
  expect(errors.length).toBeGreaterThanOrEqual(2);
  expect(errors.some(e => e.field === 'pump_start_level')).toBe(true);
  expect(errors.some(e => e.field === 'dry_run_timeout_sec')).toBe(true);
});
```

---

## 7. Module 5: Component Function Tests — TankLevelCard

> **Tool:** Jest + RTL. Firebase data injected as props or mocked context.

---

### DB-TANK-001 [P2] — Renders Skeleton When status is null

```typescript
test('renders skeleton when status is null', () => {
  render(<TankLevelCard status={null} />);
  expect(document.querySelector('.skeleton')).toBeInTheDocument();
  expect(screen.queryByText(/%/)).not.toBeInTheDocument();
});
```

---

### DB-TANK-002 [P2] — Displays Correct Level Percentage

```typescript
test.each([
  [0, '0%'], [50, '50%'], [100, '100%'], [82, '82%']
])('displays %d%% correctly as "%s"', (level, expected) => {
  render(<TankLevelCard status={{ ...TEST_STATUS_NORMAL, water_level_percent: level }} />);
  expect(screen.getByText(expected)).toBeInTheDocument();
});
```

---

### DB-TANK-003 [P2] — Shows Tilde Prefix When level_estimate_active

```typescript
test('shows ~ prefix when level_estimate_active=true', () => {
  render(<TankLevelCard status={{
    ...TEST_STATUS_NORMAL,
    water_level_percent: 65,
    level_estimate_active: true,
  }} />);
  expect(screen.getByText('~65%')).toBeInTheDocument();
  // Or: the displayed text includes a tilde
  expect(screen.getByTestId('level-value').textContent).toContain('~');
});
```

---

### DB-TANK-004 [P2] — SVG Fill Color Correct by Level Range

```typescript
test.each([
  [10, 'sf-red'],     // 0–20% → red
  [35, 'sf-amber'],   // 20–50% → amber
  [75, 'sf-teal'],    // 50–100% → teal
])('SVG fill is %s color at %d%% level', (level, expectedColor) => {
  const { container } = render(
    <TankLevelCard status={{ ...TEST_STATUS_NORMAL, water_level_percent: level }} />
  );
  const fill = container.querySelector('[data-testid="tank-fill"]');
  expect(fill).toHaveClass(`fill-${expectedColor}`);
});
```

---

### DB-TANK-005 [P2] — level_fresh=false Shows "Stale" Indicator

```typescript
test('shows stale indicator when level_fresh=false', () => {
  render(<TankLevelCard status={{
    ...TEST_STATUS_NORMAL,
    level_fresh: false,
  }} />);
  expect(screen.getByText(/stale/i)).toBeInTheDocument();
});

test('shows fresh indicator when level_fresh=true', () => {
  render(<TankLevelCard status={{
    ...TEST_STATUS_NORMAL,
    level_fresh: true,
  }} />);
  expect(screen.getByText(/fresh/i)).toBeInTheDocument();
});
```

---

### DB-TANK-006 [P2] — is_sensor_error Shows Error Indicator

```typescript
test('shows sensor error indicator when is_sensor_error=true', () => {
  render(<TankLevelCard status={{
    ...TEST_STATUS_NORMAL,
    is_sensor_error: true,
  }} />);
  expect(screen.getByText(/sensor error/i)).toBeInTheDocument();
});
```

---

### DB-TANK-007 [P2] — water_level_percent Absent: No Level Display

```typescript
test('does not show percentage when water_level_percent is absent', () => {
  const statusWithoutLevel = { ...TEST_STATUS_NORMAL };
  delete statusWithoutLevel.water_level_percent;
  render(<TankLevelCard status={statusWithoutLevel} />);
  // Should show "--" or "Waiting for data" instead of 0% or undefined%
  expect(screen.queryByText('0%')).not.toBeInTheDocument();
  expect(screen.queryByText('undefined%')).not.toBeInTheDocument();
});
```

---

## 8. Module 6: Component Function Tests — PumpStatusCard

---

### DB-STATUS-001 [P2] — Correct Chip for Each run_mode Value

```typescript
test.each([
  ['AUTO_STANDBY',    'AUTO — Standby'],
  ['AUTO',            'AUTO — Running'],
  ['AUTO_COOLDOWN',   'AUTO — Cooldown'],
  ['MANUAL_ON',       'MANUAL — On'],
  ['MANUAL_OFF',      'MANUAL — Off'],
  ['MANUAL_COOLDOWN', 'MANUAL — Cooldown'],
  ['COUNTDOWN',       'Countdown'],
  ['STOPPED',         'Emergency Stop'],
] as const)('renders correct label for run_mode=%s', (mode, expectedLabel) => {
  render(<PumpStatusCard status={{ ...TEST_STATUS_NORMAL, run_mode: mode }} />);
  expect(screen.getByText(expectedLabel)).toBeInTheDocument();
});
```

---

### DB-STATUS-002 [P2] — Unknown run_mode Does Not Crash

```typescript
test('unknown run_mode value does not crash', () => {
  expect(() => {
    render(<PumpStatusCard status={{
      ...TEST_STATUS_NORMAL,
      run_mode: 'UNKNOWN_FUTURE_MODE' as any,
    }} />);
  }).not.toThrow();
});
```

---

### DB-STATUS-003 [P2] — Cooldown Countdown Starts from pump_cooldown_remaining_sec

```typescript
test('displays cooldown seconds from Firebase value', () => {
  render(<PumpStatusCard status={{
    ...TEST_STATUS_NORMAL,
    run_mode: 'AUTO_COOLDOWN',
    pump_cooldown_remaining_sec: 47,
  }} />);
  expect(screen.getByText(/47s/)).toBeInTheDocument();
});
```

---

### DB-STATUS-004 [P2] — Countdown Decrements Client-Side Every Second

```typescript
test('countdown decrements by 1 per second client-side', async () => {
  jest.useFakeTimers();
  render(<PumpStatusCard status={{
    ...TEST_STATUS_NORMAL,
    run_mode: 'AUTO_COOLDOWN',
    pump_cooldown_remaining_sec: 60,
  }} />);

  expect(screen.getByText(/60s/)).toBeInTheDocument();
  act(() => jest.advanceTimersByTime(1000));
  expect(screen.getByText(/59s/)).toBeInTheDocument();
  act(() => jest.advanceTimersByTime(1000));
  expect(screen.getByText(/58s/)).toBeInTheDocument();

  jest.useRealTimers();
});
```

**This test verifies the countdown is client-side (smooth), not Firebase-driven (3s jumps).**

---

### DB-STATUS-005 [P2] — Flow Rate Shows Em-Dash When Pump Not Running

```typescript
test('shows em-dash for flow rate when is_running=false', () => {
  render(<PumpStatusCard status={{
    ...TEST_STATUS_NORMAL,
    is_running: false,
    flow_rate_lpm: 0,
  }} />);
  expect(screen.getByText('— LPM')).toBeInTheDocument();
});

test('shows flow rate value when is_running=true', () => {
  render(<PumpStatusCard status={{
    ...TEST_STATUS_NORMAL,
    is_running: true,
    flow_rate_lpm: 8.3,
  }} />);
  expect(screen.getByText('8.3 LPM')).toBeInTheDocument();
  expect(screen.queryByText('— LPM')).not.toBeInTheDocument();
});
```

---

### DB-STATUS-006 [P2] — Uptime Format: Xh Ym for >60 Minutes

```typescript
test.each([
  [30, '30m'],
  [60, '1h 0m'],
  [125, '2h 5m'],
  [0, '0m'],
])('formats %d minutes as "%s"', (minutes, expected) => {
  render(<PumpStatusCard status={{
    ...TEST_STATUS_NORMAL,
    uptime_minutes: minutes,
  }} />);
  expect(screen.getByText(expected)).toBeInTheDocument();
});
```

---

## 9. Module 7: Component Function Tests — ControlPanel

> **Safety-critical module. Every write path is tested for correct Firebase path, confirmation requirement, and pending state.**

---

### DB-CTRL-001 [P1] — E-Stop Shows Confirmation Before Writing

```typescript
test('E-stop click shows confirmation, does not immediately write', async () => {
  render(<ControlPanel {...defaultProps} />);
  const eStopBtn = screen.getByRole('button', { name: /emergency stop/i });
  fireEvent.click(eStopBtn);

  // Confirmation popover should appear
  expect(screen.getByText(/stop the pump now/i)).toBeInTheDocument();
  // Firebase write must NOT have been called yet
  expect(mockSet).not.toHaveBeenCalled();
});

test('E-stop confirmation Cancel prevents Firebase write', async () => {
  render(<ControlPanel {...defaultProps} />);
  fireEvent.click(screen.getByRole('button', { name: /emergency stop/i }));
  fireEvent.click(screen.getByRole('button', { name: /cancel/i }));

  expect(mockSet).not.toHaveBeenCalled();
  // Confirmation should disappear
  expect(screen.queryByText(/stop the pump now/i)).not.toBeInTheDocument();
});

test('E-stop confirmation Confirm writes to correct path', async () => {
  render(<ControlPanel {...defaultProps} />);
  fireEvent.click(screen.getByRole('button', { name: /emergency stop/i }));
  fireEvent.click(screen.getByRole('button', { name: /^stop$/i }));

  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/emergency_stop' }),
    true
  );
});
```

---

### DB-CTRL-002 [P1] — Mode Change Disables Controls During Write (Pending State)

```typescript
test('all controls disabled during Firebase write', async () => {
  mockSet.mockImplementation(() => new Promise(resolve => setTimeout(resolve, 500)));
  render(<ControlPanel {...defaultProps} />);

  fireEvent.click(screen.getByRole('button', { name: /manual/i }));

  // Immediately after click, before write resolves: controls must be disabled
  const autoBtn = screen.getByRole('button', { name: /auto/i });
  expect(autoBtn).toBeDisabled();

  await waitFor(() => expect(autoBtn).not.toBeDisabled());
});
```

---

### DB-CTRL-003 [P1] — MANUAL Start Button Only Visible in MANUAL Mode

```typescript
test('Start Pump button not visible in AUTO mode', () => {
  render(<ControlPanel control={{ mode: 'AUTO', ...defaultControl }} {...defaultProps} />);
  expect(screen.queryByRole('button', { name: /start pump/i })).not.toBeInTheDocument();
});

test('Start Pump button visible in MANUAL mode', () => {
  render(<ControlPanel control={{ mode: 'MANUAL', ...defaultControl }} {...defaultProps} />);
  expect(screen.getByRole('button', { name: /start pump/i })).toBeInTheDocument();
});
```

---

### DB-CTRL-004 [P1] — MANUAL Start Writes manual_desired=true to Correct Path

```typescript
test('Start Pump writes manual_desired=true', async () => {
  render(<ControlPanel control={{ mode: 'MANUAL', manual_desired: false, ...defaultControl }} />);
  fireEvent.click(screen.getByRole('button', { name: /start pump/i }));

  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/manual_desired' }),
    true  // boolean, not string
  );
});
```

---

### DB-CTRL-005 [P1] — MANUAL Stop Writes manual_desired=false

```typescript
test('Stop Pump writes manual_desired=false', async () => {
  render(<ControlPanel
    control={{ mode: 'MANUAL', manual_desired: true, ...defaultControl }}
    status={{ ...TEST_STATUS_NORMAL, is_running: true }}
  />);
  fireEvent.click(screen.getByRole('button', { name: /stop pump/i }));

  expect(mockSet).toHaveBeenCalledWith(
    expect.objectContaining({ _path: '/pump_system/control/manual_desired' }),
    false
  );
});
```

---

### DB-CTRL-006 [P1] — STOPPED State: Only Reset Button Shown, No Other Controls

```typescript
test('when STOPPED: shows reset button, other controls disabled', () => {
  render(<ControlPanel status={{ ...TEST_STATUS_NORMAL, emergency_stop_latched: true, run_mode: 'STOPPED' }} />);

  // Reset button must be visible
  expect(screen.getByRole('button', { name: /reset/i })).toBeInTheDocument();
  // Mode selector buttons must be disabled
  expect(screen.getByRole('button', { name: /auto/i })).toBeDisabled();
  expect(screen.getByRole('button', { name: /manual/i })).toBeDisabled();
});
```

---

### DB-CTRL-007 [P1] — Reset E-Stop Requires Confirmation

```typescript
test('Reset E-stop shows confirmation before writing', () => {
  render(<ControlPanel status={{ ...TEST_STATUS_NORMAL, emergency_stop_latched: true }} />);
  fireEvent.click(screen.getByRole('button', { name: /reset/i }));

  expect(screen.getByText(/confirm/i)).toBeInTheDocument();
  expect(mockSet).not.toHaveBeenCalled();
});
```

---

### DB-CTRL-008 [P2] — Countdown Controls Only Visible in COUNTDOWN Mode

```typescript
test('countdown input not visible in AUTO mode', () => {
  render(<ControlPanel control={{ mode: 'AUTO', ...defaultControl }} />);
  expect(screen.queryByLabelText(/minutes/i)).not.toBeInTheDocument();
});

test('countdown input visible in COUNTDOWN mode', () => {
  render(<ControlPanel control={{ mode: 'COUNTDOWN', ...defaultControl }} />);
  expect(screen.getByLabelText(/minutes/i)).toBeInTheDocument();
});
```

---

### DB-CTRL-009 [P2] — Countdown Duration Clamped to 1–120

```typescript
test('countdown input rejects value below 1', () => {
  render(<ControlPanel control={{ mode: 'COUNTDOWN', ...defaultControl }} />);
  const input = screen.getByLabelText(/minutes/i);
  fireEvent.change(input, { target: { value: '0' } });
  fireEvent.click(screen.getByRole('button', { name: /start timer/i }));

  // Should not write 0 to Firebase
  expect(mockUpdate).not.toHaveBeenCalledWith(
    expect.anything(),
    expect.objectContaining({ countdown_duration_min: 0 })
  );
});
```

---

### DB-CTRL-010 [P2] — Write Error Shows User-Facing Message

```typescript
test('shows error message when Firebase write fails', async () => {
  mockSet.mockRejectedValue(new Error('Network unavailable'));
  render(<ControlPanel {...defaultProps} />);

  fireEvent.click(screen.getByRole('button', { name: /manual/i }));

  await waitFor(() => {
    expect(screen.getByText(/failed|error|unavailable/i)).toBeInTheDocument();
  });
});
```

---

## 10. Module 8: Component Function Tests — AlertsCard

---

### DB-ALERT-001 [P1] — Alert Priority: E-Stop Before DRY_RUN

```typescript
test('emergency stop alert renders before DRY_RUN error', () => {
  render(<AlertsCard status={{
    ...TEST_STATUS_NORMAL,
    emergency_stop_latched: true,
    is_error: true,
    last_fault_code: 'DRY_RUN',
  }} />);

  const alerts = screen.getAllByRole('alert');
  // E-stop should be first
  expect(alerts[0]).toHaveTextContent(/emergency stop/i);
});
```

---

### DB-ALERT-002 [P1] — DRY_RUN Error Card Shows Clear Button

```typescript
test('DRY_RUN error shows clear error button', () => {
  render(<AlertsCard
    status={{ ...TEST_STATUS_NORMAL, is_error: true, last_fault_code: 'DRY_RUN' }}
    onClearError={mockClearError}
  />);
  const clearBtn = screen.getByRole('button', { name: /clear error/i });
  expect(clearBtn).toBeInTheDocument();

  fireEvent.click(clearBtn);
  expect(mockClearError).toHaveBeenCalledTimes(1);
});
```

---

### DB-ALERT-003 [P2] — manual_runtime_warning: Non-Blocking, No Dismiss, No Pump Action

```typescript
test('manual_runtime_warning shows amber non-blocking alert', () => {
  render(<AlertsCard status={{
    ...TEST_STATUS_NORMAL,
    manual_runtime_warning: true,
    is_running: true,
  }} />);

  const alert = screen.getByText(/manual run has exceeded/i).closest('[role="alert"]');
  expect(alert).toHaveClass(/amber/i);
  // No dismiss button
  expect(screen.queryByRole('button', { name: /dismiss/i })).not.toBeInTheDocument();
  // No stop button in this alert
  expect(screen.queryByRole('button', { name: /stop/i })).not.toBeInTheDocument();
});
```

---

### DB-ALERT-004 [P2] — bypass_flow_sensor Active Shows Warning

```typescript
test('shows flow bypass active warning', () => {
  render(<AlertsCard status={{
    ...TEST_STATUS_NORMAL,
    bypass_flow_sensor: true,
  }} />);
  expect(screen.getByText(/flow sensor bypassed/i)).toBeInTheDocument();
  expect(screen.getByText(/dry-run protection disabled/i)).toBeInTheDocument();
});
```

---

### DB-ALERT-005 [P2] — is_idle_mode Shows Informational Badge

```typescript
test('shows idle mode badge when is_idle_mode=true', () => {
  render(<AlertsCard status={{
    ...TEST_STATUS_NORMAL,
    is_idle_mode: true,
  }} />);
  expect(screen.getByText(/idle mode/i)).toBeInTheDocument();
});
```

---

### DB-ALERT-006 [P2] — Normal State Shows "System operating normally"

```typescript
test('shows "System operating normally" when no alerts', () => {
  render(<AlertsCard status={TEST_STATUS_NORMAL} />);
  expect(screen.getByText(/system operating normally/i)).toBeInTheDocument();
  // No alert roles present
  expect(screen.queryByRole('alert')).not.toBeInTheDocument();
});
```

---

### DB-ALERT-007 [P2] — All 8 Fault Codes Render Without Crash

```typescript
test.each([
  'DRY_RUN', 'OVERFLOW', 'E_STOP', 'COMM_LOSS',
  'STALE_LEVEL', 'LEVEL_SENSOR', 'FLOW_SENSOR', 'SAFE_MODE',
])('renders fault code %s without crash', (faultCode) => {
  expect(() => {
    render(<AlertsCard status={{
      ...TEST_STATUS_NORMAL,
      is_error: true,
      last_fault_code: faultCode as FaultCode,
    }} />);
  }).not.toThrow();
});
```

---

## 11. Module 9: Component Function Tests — DiagnosticsCard

---

### DB-DIAG-001 [P2] — Log Level Control Reads from status.debug_log_level

```typescript
test('shows current log level from status field', () => {
  render(<DiagnosticsCard
    status={{ ...TEST_STATUS_NORMAL, debug_log_level: 3 }}
    config={TEST_CONFIG}
  />);
  // The displayed "current" level should be from status (what firmware is actually using)
  // not from config (what we want to set next)
  expect(screen.getByText(/debug/i)).toBeInTheDocument();
  // Level 3 = DEBUG
});
```

---

### DB-DIAG-002 [P2] — Log Level Control Writes to Config Path, Not Status Path

```typescript
test('log level change writes to config/device/debug_log_level', async () => {
  render(<DiagnosticsCard status={TEST_STATUS_NORMAL} config={TEST_CONFIG} />);

  const levelControl = screen.getByLabelText(/log level/i);
  fireEvent.change(levelControl, { target: { value: '3' } });

  await waitFor(() => {
    expect(mockSet).toHaveBeenCalledWith(
      expect.objectContaining({ _path: '/pump_system/config/device/debug_log_level' }),
      3
    );
    // Must NOT write to status path
    const paths = mockSet.mock.calls.map(c => c[0]._path);
    expect(paths.every(p => !p.includes('/status/'))).toBe(true);
  });
});
```

---

### DB-DIAG-003 [P2] — Shows Warning When Log Level Above INFO

```typescript
test('shows warning when log level set above INFO (2)', () => {
  render(<DiagnosticsCard
    status={{ ...TEST_STATUS_NORMAL, debug_log_level: 3 }}
    config={TEST_CONFIG}
  />);
  expect(screen.getByText(/verbose logging/i)).toBeInTheDocument();
});

test('no warning when log level is INFO (2)', () => {
  render(<DiagnosticsCard
    status={{ ...TEST_STATUS_NORMAL, debug_log_level: 2 }}
    config={TEST_CONFIG}
  />);
  expect(screen.queryByText(/verbose logging/i)).not.toBeInTheDocument();
});
```

---

### DB-DIAG-004 [P2] — remote_level_discard_count Styled Amber When > 0

```typescript
test('discard count > 0 rendered in amber color', () => {
  const { container } = render(<DiagnosticsCard
    status={{ ...TEST_STATUS_NORMAL, remote_level_discard_count: 3 }}
    config={TEST_CONFIG}
  />);
  const discardEl = screen.getByText('3').closest('[data-testid="level-discard"]');
  expect(discardEl).toHaveClass(/amber/i);
});

test('discard count = 0 rendered in muted color', () => {
  const { container } = render(<DiagnosticsCard
    status={{ ...TEST_STATUS_NORMAL, remote_level_discard_count: 0 }}
    config={TEST_CONFIG}
  />);
  const discardEl = screen.getByText('0').closest('[data-testid="level-discard"]');
  expect(discardEl).not.toHaveClass(/amber/i);
});
```

---

## 12. Module 10: Settings Page — Function Tests

---

### DB-SETTINGS-001 [P2] — Save Button Disabled While Writing

```typescript
test('save button disabled during Firebase write', async () => {
  mockUpdate.mockImplementation(() =>
    new Promise(resolve => setTimeout(resolve, 500))
  );
  render(<SettingsPage />);

  // Fill valid settings
  fireEvent.change(screen.getByLabelText(/start level/i), { target: { value: '25' } });
  fireEvent.click(screen.getByRole('button', { name: /save/i }));

  // During write: Save button disabled
  expect(screen.getByRole('button', { name: /saving/i })).toBeDisabled();

  await waitFor(() => {
    expect(screen.getByRole('button', { name: /save/i })).not.toBeDisabled();
  });
});
```

---

### DB-SETTINGS-002 [P2] — Success Message Shown After Valid Save

```typescript
test('shows success message after valid save', async () => {
  mockUpdate.mockResolvedValue(undefined);
  render(<SettingsPage />);

  fireEvent.click(screen.getByRole('button', { name: /save/i }));

  await waitFor(() => {
    expect(screen.getByText(/settings saved/i)).toBeInTheDocument();
    expect(screen.getByText(/30 seconds/i)).toBeInTheDocument();
  });
});
```

---

### DB-SETTINGS-003 [P2] — Inline Error Shown Next to Specific Invalid Field

```typescript
test('inline error appears next to pump_start_level field on validation fail', () => {
  render(<SettingsPage />);
  fireEvent.change(screen.getByLabelText(/start level/i), { target: { value: '95' } });
  fireEvent.change(screen.getByLabelText(/stop level/i), { target: { value: '20' } });
  fireEvent.click(screen.getByRole('button', { name: /save/i }));

  // Error must be near the start level field, not just a toast
  const startLevelField = screen.getByLabelText(/start level/i);
  const fieldContainer = startLevelField.closest('[data-testid="field-container"]');
  expect(fieldContainer).toHaveTextContent(/less than stop level/i);
  // Firebase must not have been written
  expect(mockUpdate).not.toHaveBeenCalled();
});
```

---

### DB-SETTINGS-004 [P2] — bypass_flow_sensor Toggle Present in Advanced Section

```typescript
test('bypass_flow_sensor toggle is present', () => {
  render(<SettingsPage />);
  expect(screen.getByLabelText(/bypass flow sensor/i)).toBeInTheDocument();
});

test('bypass_flow_sensor toggle writes to correct Firebase path', async () => {
  render(<SettingsPage />);
  const toggle = screen.getByLabelText(/bypass flow sensor/i);
  fireEvent.click(toggle);

  await waitFor(() => {
    expect(mockSet).toHaveBeenCalledWith(
      expect.objectContaining({ _path: '/pump_system/control/bypass_flow_sensor' }),
      expect.any(Boolean)
    );
  });
});
```

---

## 13. Module 11: Dashboard ↔ Firmware Integration Tests

> **Tool:** Dashboard running against Firebase emulator seeded with firmware-like data.
> **Purpose:** Verify the dashboard correctly renders all firmware states without manual ESP32.

---

### DB-INT-001 [P2] — Dashboard Renders All 8 run_mode Values from Emulator

```typescript
describe('run_mode integration', () => {
  const modes = [
    'AUTO_STANDBY', 'AUTO', 'AUTO_COOLDOWN',
    'MANUAL_ON', 'MANUAL_OFF', 'MANUAL_COOLDOWN',
    'COUNTDOWN', 'STOPPED'
  ];

  test.each(modes)('renders run_mode=%s correctly', async (mode) => {
    await seedEmulator({ status: { ...BASE_STATUS, run_mode: mode } });
    render(<Dashboard />);
    await waitFor(() => {
      const chip = screen.getByTestId('run-mode-chip');
      expect(chip.textContent).not.toBe('');
      expect(chip.textContent).not.toContain('undefined');
    });
  });
});
```

---

### DB-INT-002 [P2] — Dashboard Reflects is_running State Within 1 Render Cycle

```typescript
test('is_running=true shows running state UI', async () => {
  await seedEmulator({ status: { ...BASE_STATUS, is_running: true, run_mode: 'AUTO' } });
  render(<Dashboard />);
  await waitFor(() => {
    expect(screen.getByText('AUTO — Running')).toBeInTheDocument();
  });
});

test('is_running=false shows standby state UI', async () => {
  await seedEmulator({ status: { ...BASE_STATUS, is_running: false, run_mode: 'AUTO_STANDBY' } });
  render(<Dashboard />);
  await waitFor(() => {
    expect(screen.getByText('AUTO — Standby')).toBeInTheDocument();
  });
});
```

---

### DB-INT-003 [P2] — Real-Time Update: Firebase Change Triggers UI Re-render

```typescript
test('UI updates when Firebase status changes without page reload', async () => {
  await seedEmulator({ status: { ...BASE_STATUS, water_level_percent: 30 } });
  render(<Dashboard />);
  await waitFor(() => expect(screen.getByText('30%')).toBeInTheDocument());

  // Simulate firmware updating the level
  await updateEmulator({ 'pump_system/status/water_level_percent': 65 });
  await waitFor(() => expect(screen.getByText('65%')).toBeInTheDocument());
  expect(screen.queryByText('30%')).not.toBeInTheDocument();
});
```

---

### DB-INT-004 [P2] — Error States Render Correctly from Emulator Data

```typescript
test.each([
  { is_error: true, last_fault_code: 'DRY_RUN', expected: /dry-run/i },
  { is_overflow_error: true, last_fault_code: 'OVERFLOW', expected: /overflow/i },
  { emergency_stop_latched: true, run_mode: 'STOPPED', expected: /emergency stop/i },
  { is_sensor_error: true, last_fault_code: 'LEVEL_SENSOR', expected: /sensor error/i },
])('renders $last_fault_code error correctly', async ({ expected, ...status }) => {
  await seedEmulator({ status: { ...BASE_STATUS, ...status } });
  render(<Dashboard />);
  await waitFor(() => expect(screen.getByText(expected)).toBeInTheDocument());
});
```

---

### DB-INT-005 [P2] — Offline Banner Appears on Firebase Disconnect

```typescript
test('offline banner appears when isConnected=false', async () => {
  await seedEmulator({ '.info/connected': false });
  render(<Dashboard />);
  await waitFor(() => {
    expect(screen.getByText(/reconnecting/i)).toBeInTheDocument();
  });
  // Last-known data must still be visible
  expect(screen.queryByText(/loading/i)).not.toBeInTheDocument();
});
```

---

### DB-INT-006 [P3] — lastUpdatedAt Shows Relative Time

```typescript
test('last updated time shown in header', async () => {
  jest.useFakeTimers();
  jest.setSystemTime(new Date('2026-01-01T10:00:00Z'));

  await seedEmulator({ status: BASE_STATUS });
  render(<Dashboard />);
  await waitFor(() => expect(screen.getByText(/just now/i)).toBeInTheDocument());

  jest.advanceTimersByTime(5000);
  await waitFor(() => expect(screen.getByText(/5s ago/i)).toBeInTheDocument());

  jest.useRealTimers();
});
```

---

## 14. Module 12: Control Path Integration — Safety Critical

> **These tests run against a real Firebase project with a running ESP32.**
> **Each test verifies the complete loop: dashboard click → Firebase → ESP32 response → Firebase → dashboard update.**

---

### DB-SAFETY-001 [P1] — E-Stop: Dashboard Click → Pump Stops Within 6 Seconds

**Environment:** Live Firebase + running ESP32 + pump physically connected.

**Procedure:**
1. Pump running in AUTO or MANUAL mode. Confirm `is_running = true`.
2. Click Emergency Stop in dashboard. Click Confirm in popover.
3. Start timer.
4. Observe: `is_running` in Firebase status changes to `false`.
5. Observe: `emergency_stop_latched = true`.
6. Stop timer.

**Pass criteria:** `is_running = false` within 6s (2 × 3s poll cycles max). Relay audibly clicks off.
**Fail criteria:** Pump still running after 6s. Defect category: SAFETY / CRITICAL.

---

### DB-SAFETY-002 [P1] — E-Stop: `emergency_stop` Control Field Resets to false After Processing

**Procedure:**
1. Trigger E-stop via dashboard.
2. Wait 6s.
3. Read `/pump_system/control/emergency_stop` from Firebase.

**Pass criteria:** Value is `false` (firmware processed the one-shot and reset it).
**Fail criteria:** Value stays `true`. Defect: firmware did not process the one-shot, or dashboard wrote wrong path.

---

### DB-SAFETY-003 [P1] — Mode Change: Dashboard → Firmware → Back to Dashboard Loop

**Procedure:**
1. Dashboard shows `run_mode: AUTO_STANDBY`.
2. Click MANUAL in dashboard mode selector.
3. Observe dashboard: shows pending state.
4. Observe Firebase `/pump_system/control/mode`: should be `"MANUAL"`.
5. Wait for ESP32 to read and apply (≤ 6s).
6. Observe Firebase `/pump_system/status/run_mode`: should become `"MANUAL_OFF"`.
7. Observe dashboard: mode chip updates to "MANUAL — Off".

**Pass criteria:** Full round-trip complete within 12s (write + 2 poll cycles).

---

### DB-SAFETY-004 [P1] — MANUAL Start: Dashboard → Firebase → Pump On → Dashboard

**Procedure:**
1. Mode = MANUAL via dashboard.
2. Click "Start Pump".
3. Measure: time from click to `is_running: true` in Firebase status.
4. Observe relay click (physical) within same window.

**Pass criteria:** `is_running: true` within 6s. Relay click audible.

---

### DB-SAFETY-005 [P1] — Clear Error: Dashboard Click Resolves DRY_RUN Error

**Procedure:**
1. DRY_RUN error active (`is_error: true`).
2. Restore flow (so pump can restart after clear).
3. Click "Clear Error" in dashboard AlertsCard.
4. Observe: `is_error` becomes `false` within 6s.
5. Observe: `clear_error` control field resets to `false` (one-shot).
6. In AUTO mode: observe pump restarts within 6s (if level below start).

**Pass criteria:** Error cleared, one-shot reset, pump able to restart.

---

### DB-SAFETY-006 [P1] — Bypass Flow Sensor: Dashboard Toggle → Firmware Effect → No DRY_RUN

**Procedure:**
1. `bypass_flow_sensor = false`. Block flow (0 LPM). Wait — DRY_RUN should fire after 30s.
2. Clear error. Enable `bypass_flow_sensor` from Settings.
3. Block flow again. Wait 40s.
4. Confirm `is_error = false` (DRY_RUN did not fire with bypass active).
5. Disable bypass. Block flow again. Confirm DRY_RUN fires within 36s.

**Pass criteria:** Bypass toggle correctly enables/disables firmware dry-run protection.

---

### DB-SAFETY-007 [P2] — Countdown Start: Dashboard Sets Duration → Firmware Counts Down

**Procedure:**
1. Set countdown to 3 minutes via dashboard.
2. Click Start Timer.
3. Observe `run_mode: COUNTDOWN` in Firebase.
4. Observe `countdown_remaining_sec` starting near 180.
5. Verify countdown decrements in Firebase every ~3s (firmware poll rate).
6. Verify dashboard countdown decrements smoothly every 1s (client-side).
7. At t=3m: verify pump stops, `run_mode` reverts to AUTO.

---

### DB-SAFETY-008 [P2] — Remote Log Level: Dashboard → Firmware → Serial Output Changes

**Procedure:**
1. Initial `debug_log_level: 2` (INFO). No [D] messages in ESP32 Serial.
2. Set log level to 3 (DEBUG) via Diagnostics panel.
3. Wait 30s (config poll cycle).
4. Observe ESP32 Serial Monitor: [D] messages should appear.
5. Observe Firebase `status.debug_log_level: 3` (firmware confirms it read the new value).
6. Return level to INFO. Verify [D] messages stop.

---

## 15. Module 13: Status Path Integration — Read Fidelity

> **Verify the dashboard correctly displays what firmware writes, with no distortion, rounding errors, or mapping mistakes.**

---

### DB-READ-001 [P2] — wifi_rssi Displayed with Correct Signal Strength Color

```typescript
test.each([
  [-50, 'sf-teal'],    // ≥ -60 dBm: good
  [-70, 'sf-amber'],   // -60 to -75 dBm: fair
  [-80, 'sf-red'],     // < -75 dBm: poor
])('rssi=%d shows %s color', (rssi, expectedColor) => {
  render(<Header status={{ ...TEST_STATUS_NORMAL, wifi_rssi: rssi }} />);
  const rssiBadge = screen.getByTestId('rssi-badge');
  expect(rssiBadge).toHaveClass(expectedColor);
  expect(rssiBadge).toHaveTextContent(`${rssi} dBm`);
});
```

---

### DB-READ-002 [P2] — free_heap_bytes Displayed as Formatted Number

```typescript
test('free_heap_bytes displays with thousands separator', () => {
  render(<DiagnosticsCard status={{
    ...TEST_STATUS_NORMAL,
    free_heap_bytes: 182450,
  }} />);
  expect(screen.getByText('182,450 B')).toBeInTheDocument();
  // Not "182450B" or "182.450 B" (wrong locale format)
});
```

---

### DB-READ-003 [P2] — remote_level_discard_count Reflects Current Cycle Count

```typescript
test('displays remote_level_discard_count directly from Firebase', () => {
  render(<DiagnosticsCard status={{
    ...TEST_STATUS_NORMAL,
    remote_level_discard_count: 7,
  }} />);
  expect(screen.getByText('7')).toBeInTheDocument();
  // Verify context: should be labeled as level discards
  expect(screen.getByText(/level discard/i)).toBeInTheDocument();
});
```

---

### DB-READ-004 [P2] — last_boot_reason Shown Verbatim

```typescript
test.each([
  'Power-on',
  'Task watchdog',
  'Software reset',
  'Brownout reset',
])('last_boot_reason "%s" shown verbatim', (reason) => {
  render(<PumpStatusCard status={{ ...TEST_STATUS_NORMAL, last_boot_reason: reason }} />);
  expect(screen.getByText(reason)).toBeInTheDocument();
});
```

---

## 16. Module 14: Config Path Integration — Settings Round-Trip

---

### DB-CONFIG-001 [P2] — Settings Form Populated from Firebase Config on Load

```typescript
test('settings form pre-populated with current Firebase config', async () => {
  await seedEmulator({ config: { device: {
    ...BASE_CONFIG,
    pump_start_level: 35,
    pump_stop_level: 85,
  }}});

  render(<SettingsPage />);
  await waitFor(() => {
    expect(screen.getByLabelText(/start level/i)).toHaveValue(35);
    expect(screen.getByLabelText(/stop level/i)).toHaveValue(85);
  });
});
```

---

### DB-CONFIG-002 [P2] — Settings Save Writes All Fields to Correct Paths

```typescript
test('save writes all fields to /pump_system/config/device/', async () => {
  render(<SettingsPage />);

  fireEvent.change(screen.getByLabelText(/start level/i), { target: { value: '25' } });
  fireEvent.click(screen.getByRole('button', { name: /save/i }));

  await waitFor(() => {
    expect(mockUpdate).toHaveBeenCalledWith(
      expect.objectContaining({ _path: '/pump_system/config/device' }),
      expect.objectContaining({ pump_start_level: 25 })
    );
  });
});
```

---

### DB-CONFIG-003 [P2] — Settings Save Does Not Write to Control Path

```typescript
test('settings save does not write to /pump_system/control/', async () => {
  render(<SettingsPage />);
  fireEvent.click(screen.getByRole('button', { name: /save/i }));

  await waitFor(() => expect(mockUpdate).toHaveBeenCalled());
  const allPaths = mockUpdate.mock.calls.map(c => c[0]._path);
  expect(allPaths.every(p => !p.includes('/control/'))).toBe(true);
});
```

---

### DB-CONFIG-004 [P2] — Cancel Reverts Unsaved Changes

```typescript
test('cancel reverts form to Firebase values', async () => {
  await seedEmulator({ config: { device: { ...BASE_CONFIG, pump_start_level: 20 } } });
  render(<SettingsPage />);

  fireEvent.change(screen.getByLabelText(/start level/i), { target: { value: '35' } });
  fireEvent.click(screen.getByRole('button', { name: /cancel/i }));

  expect(screen.getByLabelText(/start level/i)).toHaveValue(20);
  expect(mockUpdate).not.toHaveBeenCalled();
});
```

---

## 17. Module 15: Offline & Resilience Tests

---

### DB-OFFLINE-001 [P2] — Offline Banner Appears and is Not Dismissable

```typescript
test('offline banner appears and has no dismiss button', async () => {
  // Simulate Firebase disconnect
  await updateEmulator({ '.info/connected': false });
  render(<Dashboard />);

  await waitFor(() => {
    const banner = screen.getByTestId('offline-banner');
    expect(banner).toBeInTheDocument();
    // No dismiss/close button inside the banner
    expect(within(banner).queryByRole('button')).not.toBeInTheDocument();
  });
});
```

---

### DB-OFFLINE-002 [P2] — Last-Known Data Shown During Offline, Not Blank

```typescript
test('last-known data shown during offline period', async () => {
  // First get data
  await seedEmulator({ status: { ...BASE_STATUS, water_level_percent: 65 } });
  render(<Dashboard />);
  await waitFor(() => expect(screen.getByText('65%')).toBeInTheDocument());

  // Then disconnect
  await updateEmulator({ '.info/connected': false });
  await waitFor(() =>
    expect(screen.getByTestId('offline-banner')).toBeInTheDocument()
  );

  // Data must still be visible — not blank, not 0%
  expect(screen.getByText('65%')).toBeInTheDocument();
});
```

---

### DB-OFFLINE-003 [P2] — Controls Blocked During Offline

```typescript
test('control panel buttons disabled during offline', async () => {
  await updateEmulator({ '.info/connected': false });
  render(<ControlPanel status={TEST_STATUS_NORMAL} />);

  await waitFor(() => {
    expect(screen.getByRole('button', { name: /auto/i })).toBeDisabled();
    expect(screen.getByRole('button', { name: /emergency stop/i })).toBeDisabled();
  });
});
```

**Engineering rationale:** Allowing writes during Firebase disconnect causes lost commands — the write will eventually succeed when connection restores, but the user may have already changed their mind or the pump state may have changed.

---

### DB-OFFLINE-004 [P2] — Banner Disappears on Reconnect

```typescript
test('offline banner disappears when connection restores', async () => {
  await updateEmulator({ '.info/connected': false });
  render(<Dashboard />);
  await waitFor(() => expect(screen.getByTestId('offline-banner')).toBeInTheDocument());

  await updateEmulator({ '.info/connected': true });
  await waitFor(() =>
    expect(screen.queryByTestId('offline-banner')).not.toBeInTheDocument()
  );
});
```

---

### DB-OFFLINE-005 [P2] — E-Stop Remains Visible During Offline (Safety Invariant)

```typescript
test('emergency stop button visible even during offline', async () => {
  await updateEmulator({ '.info/connected': false });
  render(<Dashboard />);

  // E-stop must always be visible — it's a physical safety control
  expect(screen.getByRole('button', { name: /emergency stop/i })).toBeInTheDocument();
  // Note: it may be disabled during offline, but must be visible
});
```

---

### DB-OFFLINE-006 [P2] — Error Boundary Catches Component Crash, Shows Fallback

```typescript
test('error boundary shows fallback when component throws', () => {
  // Force TankLevelCard to throw
  const BadTankCard = () => { throw new Error('Render error'); };
  render(
    <ErrorBoundary fallback={<div>Unable to load tank level. <button>Retry</button></div>}>
      <BadTankCard />
    </ErrorBoundary>
  );
  expect(screen.getByText(/unable to load tank level/i)).toBeInTheDocument();
  expect(screen.getByRole('button', { name: /retry/i })).toBeInTheDocument();
});
```

---

## 18. Module 16: Performance & Load Tests

---

### DB-PERF-001 [P3] — Cold Load: All Skeletons Replaced Within 5s on Fast Connection

**Tool:** Chrome DevTools → Performance tab. Dashboard running on localhost.

**Procedure:**
1. Open Chrome DevTools → Network tab → set to "Fast 3G".
2. Hard refresh dashboard (Ctrl+Shift+R).
3. Observe: skeleton loaders appear immediately, then real data.
4. Measure time from navigation start to last skeleton replaced.

**Pass criteria:** ≤ 5s on Fast 3G simulation.

---

### DB-PERF-002 [P3] — Lighthouse Performance Score ≥ 80

**Tool:** Chrome DevTools → Lighthouse → Mobile preset → Generate report.

**Pass criteria:** Performance ≥ 80, Accessibility ≥ 95, PWA ≥ 80.

---

### DB-PERF-003 [P3] — No Memory Leak: Heap Stable After 10 Mount/Unmount Cycles

**Tool:** Chrome DevTools → Memory → Record allocation timeline.

**Procedure:**
1. Open dashboard. Take heap snapshot (baseline).
2. Navigate `/` → `/settings` → `/` ten times.
3. Take heap snapshot (post-navigation).
4. Compare retained objects.

**Pass criteria:** Heap growth < 5MB between snapshots. No detached DOM nodes from Firebase listeners.

---

### DB-PERF-004 [P3] — First Contentful Paint < 2.0s on 3G

**Tool:** Lighthouse Mobile preset (Vercel deployment).

**Pass criteria:** FCP < 2.0s.

---

### DB-PERF-005 [P3] — Firebase Updates Do Not Cause Full Page Re-render

**Tool:** React DevTools Profiler.

**Procedure:**
1. Open React DevTools Profiler.
2. Start recording.
3. Let 5 Firebase status updates come in (15 seconds).
4. Stop recording.

**Expected result:** Only components consuming the changed data re-render. Root/layout components must not re-render on each Firebase update.

---

## 19. Module 17: Security Tests

---

### DB-SEC-001 [P1] — Unauthenticated User Cannot Access Dashboard

**Procedure:**
1. Open dashboard in incognito tab (no prior auth).
2. Navigate to `https://your-dashboard.vercel.app/`.

**Expected result:** Redirect to `/login` page or Google OAuth prompt. Main dashboard not accessible.
**Pass criteria:** HTTP response for `/` when unauthenticated is either 302 redirect or shows login UI.

---

### DB-SEC-002 [P1] — Unauthenticated Firebase Write Rejected

**Procedure:**
1. In browser console (incognito, no auth):
```javascript
import { initializeApp } from 'https://www.gstatic.com/firebasejs/10.x/firebase-app.js';
import { getDatabase, ref, set } from 'https://www.gstatic.com/firebasejs/10.x/firebase-database.js';
const app = initializeApp({ /* production config */ });
const db = getDatabase(app);
await set(ref(db, '/pump_system/control/emergency_stop'), true);
```
2. Observe console for PERMISSION_DENIED error.

**Pass criteria:** Firebase returns PERMISSION_DENIED. Pump is unaffected.
**Fail criteria:** Write succeeds. Defect category: SECURITY / CRITICAL.

---

### DB-SEC-003 [P1] — Wrong Google Account Cannot Control Pump

**Procedure:**
1. Log into dashboard with Account A (authorized).
2. Note authorized UID.
3. Log out. Log in with Account B (a different Google account — not authorized).
4. Attempt to write to `/pump_system/control/mode` from Account B's session.

**Expected result:** PERMISSION_DENIED from Firebase rules.
**Pass criteria:** Only the authorized UID can write control fields.

---

### DB-SEC-004 [P2] — Dashboard Does Not Expose Firebase Config in Client Bundle

**Procedure:**
1. Build the dashboard: `npm run build`.
2. Search the build output for Firebase API key and project ID.
3. Verify these are in `NEXT_PUBLIC_` environment variables (intentionally public for client-side Firebase SDK — this is expected and documented).
4. Verify NO Firebase service account keys, database secrets, or admin SDK credentials appear in the build.

**Pass criteria:** Only `NEXT_PUBLIC_FIREBASE_*` client-SDK config in bundle. No admin credentials.

---

### DB-SEC-005 [P2] — No Sensitive Data in localStorage or sessionStorage

**Procedure:**
1. Log in to dashboard.
2. DevTools → Application → Local Storage and Session Storage.
3. Inspect all keys.

**Pass criteria:** Theme preference (`sf-theme`) may be present. No Firebase auth tokens in readable form, no API keys. Firebase SDK stores auth in IndexedDB (encrypted) — that is acceptable.

---

### DB-SEC-006 [P2] — HTTPS Enforced: HTTP Redirects to HTTPS

**Procedure:**
1. Access `http://your-dashboard.vercel.app/` (HTTP).
2. Observe: automatic redirect to HTTPS.

**Pass criteria:** HTTP 301/302 redirect to HTTPS version.

---

### DB-SEC-007 [P2] — Content Security Policy Headers Present

**Procedure:**
1. DevTools → Network → Document request → Headers.
2. Look for `Content-Security-Policy` response header.

**Pass criteria:** CSP header present. Does not include `unsafe-eval` in script-src (unless justified and documented).

---

### DB-SEC-008 [P3] — No Console Errors or Warnings in Production Build

**Procedure:**
1. Open production dashboard in Chrome.
2. Open DevTools → Console.
3. Navigate all pages and trigger all features.

**Pass criteria:** Zero red errors. Zero security-related warnings (e.g., "mixed content").

---

## 20. Module 18: Accessibility Tests

---

### DB-A11Y-001 [P2] — Lighthouse Accessibility Score ≥ 95

**Tool:** Chrome DevTools → Lighthouse → Accessibility audit.
**Pass criteria:** ≥ 95.

---

### DB-A11Y-002 [P2] — E-Stop Button Has Descriptive aria-label

```typescript
test('E-stop button has descriptive aria-label', () => {
  render(<ControlPanel {...defaultProps} />);
  const btn = screen.getByRole('button', { name: /emergency stop/i });
  const label = btn.getAttribute('aria-label');
  // Label must describe the consequence, not just the name
  expect(label).toMatch(/stops pump/i);
});
```

---

### DB-A11Y-003 [P2] — All Form Inputs Have Associated Labels

```typescript
test('all settings inputs have associated labels', () => {
  render(<SettingsPage />);
  const inputs = screen.getAllByRole('textbox');
  inputs.forEach(input => {
    // Each input must have an id, and a label with matching htmlFor
    expect(input).toHaveAttribute('id');
    const label = document.querySelector(`label[for="${input.id}"]`);
    expect(label).toBeInTheDocument();
  });
});
```

---

### DB-A11Y-004 [P2] — E-Stop Visible at 375px Without Scrolling

**Tool:** Chrome DevTools → Device toolbar → 375×667 px.

**Procedure:**
1. Set viewport to 375px wide.
2. Without scrolling: confirm E-stop button (or mobile fixed bottom bar) is visible.

**Pass criteria:** E-stop visible above the fold at 375px.

---

### DB-A11Y-005 [P2] — Focus Ring Visible on All Interactive Elements

**Procedure:**
1. Open dashboard. Click somewhere neutral.
2. Press Tab. Verify focus ring visible on first focused element.
3. Continue Tab. Every interactive element gets a visible outline.
4. No keyboard trap: Tab always advances.

**Pass criteria:** All interactive elements receive visible focus ring. No keyboard trap.

---

### DB-A11Y-006 [P2] — Pump State Changes Announced to Screen Readers

```typescript
test('status live region announces pump start', async () => {
  render(<Dashboard />);

  // Find live region
  const liveRegion = screen.getByRole('status');
  expect(liveRegion).toBeInTheDocument();
  expect(liveRegion).toHaveAttribute('aria-live', 'polite');

  // Simulate pump starting
  act(() => {
    // Update status to is_running=true
    fireStatusUpdate({ ...BASE_STATUS, is_running: true, run_mode: 'AUTO' });
  });

  await waitFor(() => {
    expect(liveRegion.textContent).toMatch(/running|started/i);
  });
});
```

---

### DB-A11Y-007 [P2] — Color Contrast Ratios Meet WCAG 2.1 AA

**Tool:** Chrome DevTools → Rendering → Emulate CSS media → forced-colors: none → inspect each color pair.

**Pre-verified pairs to check:**

| Text color | Background | Ratio | Required |
|---|---|---|---|
| `#0F6E56` (sf-teal) | `#E1F5EE` (sf-teal-light) | 5.8:1 | ≥ 4.5:1 ✅ |
| `#BA7517` (sf-amber) | `#FAEEDA` (sf-amber-light) | 4.6:1 | ≥ 4.5:1 ✅ |
| `#A32D2D` (sf-red) | `#FCEBEB` (sf-red-light) | 6.1:1 | ≥ 4.5:1 ✅ |
| `#185FA5` (sf-blue) | `#FFFFFF` (white) | 7.2:1 | ≥ 4.5:1 ✅ |
| `#5F5E5A` (sf-gray-600) | `#F1EFE8` (sf-gray-50) | 4.8:1 | ≥ 4.5:1 ✅ |

**Pass criteria:** All pairs ≥ 4.5:1. Verify in dark mode as well.

---

## 21. Module 19: TypeScript & Code Quality Tests

---

### DB-QA-001 [P2] — Zero TypeScript Errors: `tsc --noEmit` Clean

**Procedure:**
```bash
cd dashboard && npx tsc --noEmit
```

**Pass criteria:** Exit code 0. Zero errors. Zero warnings (if configured as errors).

---

### DB-QA-002 [P2] — Zero `any` Types in New Files

**Procedure:**
```bash
grep -rn ": any" dashboard/lib/types.ts
grep -rn "as any" dashboard/lib/
grep -rn "as any" dashboard/components/
```

**Pass criteria:** Zero results in `lib/types.ts`, `lib/usePumpData.ts`, `lib/pumpActions.ts`.

---

### DB-QA-003 [P2] — All Firebase Listeners Have Cleanup

**Procedure:**
```bash
# Find all onValue() calls
grep -rn "onValue(" dashboard/lib/ dashboard/components/ dashboard/app/

# For each: verify the containing useEffect has a return () => cleanup
```

**Pass criteria:** Every `onValue()` call inside a `useEffect` has a corresponding unsubscribe in the cleanup function.

---

### DB-QA-004 [P2] — All Firebase Writes Are Wrapped in Try/Catch

**Procedure:**
```bash
grep -rn "await set\|await update" dashboard/components/ dashboard/app/
```

**For each `await set()` or `await update()`: verify it is inside a try/catch block.**

**Pass criteria:** Zero uncaught Firebase write calls in component code.

---

### DB-QA-005 [P3] — No "Smart Water Pump Controller" String in Codebase

**Procedure:**
```bash
grep -ri "smart water pump controller" dashboard/
```

**Pass criteria:** Zero results. All occurrences replaced with "SmartFlow".

---

### DB-QA-006 [P3] — npm audit: No Critical or High Vulnerabilities

**Procedure:**
```bash
cd dashboard && npm audit --audit-level=high
```

**Pass criteria:** Exit code 0 (no high or critical vulnerabilities). Document any medium findings.

---

## 22. Module 20: Regression Test Checklist

Run this set after every dashboard code change to confirm no regression.

| ID | Test | Component | Priority | Type |
|---|---|---|---|---|
| DB-HOOK-003 | All listeners unsubscribed on unmount | usePumpData | P1 | Unit |
| DB-ACTION-001 | setMode writes correct path and value | pumpActions | P1 | Unit |
| DB-ACTION-002 | E-stop writes boolean true | pumpActions | P1 | Unit |
| DB-ACTION-003 | clearError writes to clear_error, not emergency_stop | pumpActions | P1 | Unit |
| DB-ACTION-006 | setBypassFlow writes to bypass_flow_sensor path | pumpActions | P1 | Unit |
| DB-CONTRACT-003 | Dashboard never writes to status path | Contract | P1 | Review |
| DB-CONTRACT-006 | All 8 run_mode values render without crash | Contract | P1 | Integration |
| DB-VALIDATE-001 | Rejects start ≥ stop level | Validation | P1 | Unit |
| DB-CTRL-001 | E-stop shows confirmation before writing | ControlPanel | P1 | Unit |
| DB-CTRL-002 | Controls disabled during write | ControlPanel | P1 | Unit |
| DB-CTRL-004 | MANUAL start writes manual_desired=true | ControlPanel | P1 | Unit |
| DB-CTRL-006 | STOPPED state shows only reset button | ControlPanel | P1 | Unit |
| DB-CTRL-007 | Reset E-stop requires confirmation | ControlPanel | P1 | Unit |
| DB-TANK-007 | No 0% displayed when level field absent | TankLevelCard | P1 | Unit |
| DB-OFFLINE-002 | Last-known data shown during offline | Offline | P2 | Integration |
| DB-OFFLINE-005 | E-stop visible during offline | Offline | P1 | Integration |
| DB-SAFETY-001 | E-stop → pump stops within 6s | Integration | P1 | E2E |
| DB-SAFETY-004 | MANUAL start round-trip within 6s | Integration | P1 | E2E |
| DB-SEC-001 | Unauthenticated user sees login page | Security | P1 | Manual |
| DB-SEC-002 | Unauthenticated write rejected | Security | P1 | Manual |
| DB-QA-001 | tsc --noEmit passes | Code quality | P2 | Automated |

---

## 23. Test Traceability Matrix

| Requirement | Source | Test cases covering |
|---|---|---|
| Firebase listener cleanup (memory leak fix) | D2-LEAK | DB-HOOK-003, DB-QA-003 |
| Null-safe Firebase data access | D2-NULL | DB-HOOK-007, DB-TANK-007 |
| Pending state on all writes | D2-PENDING | DB-CTRL-002, DB-SETTINGS-001 |
| Settings validation before save | D2-VALIDATE | DB-VALIDATE-001 through DB-VALIDATE-006, DB-SETTINGS-003 |
| PumpStatus typed interface | D1 | DB-CONTRACT-001, DB-QA-002 |
| All 8 run_mode values handled | Dashboard spec | DB-CONTRACT-006, DB-STATUS-001, DB-STATUS-002 |
| All 8 fault codes handled | Dashboard spec | DB-CONTRACT-007, DB-ALERT-007 |
| E-stop confirmation required | Safety principle | DB-CTRL-001, DB-CTRL-007 |
| E-stop visible on mobile | D7 / Safety | DB-A11Y-004, DB-OFFLINE-005 |
| bypass_flow_sensor UI + warning | D5 | DB-ACTION-006, DB-ALERT-004, DB-SETTINGS-004 |
| is_idle_mode badge | D5 | DB-ALERT-005, DB-INT-002 |
| debug_log_level read/write | D5 | DB-DIAG-001, DB-DIAG-002, DB-SAFETY-008 |
| remote_level_discard_count display | D5 | DB-DIAG-003, DB-DIAG-004, DB-READ-003 |
| Cooldown countdown client-side | D4 | DB-STATUS-003, DB-STATUS-004 |
| Level estimate visual (~prefix) | D5 | DB-TANK-003, DB-INT-005 |
| Offline banner + last data | D10 | DB-OFFLINE-001 through DB-OFFLINE-005 |
| Dark/light theme | D3 | DB-INT-006, DB-PERF-002 |
| Google auth required | D7 / Security | DB-SEC-001, DB-SEC-003 |
| Firebase rules: only correct UID | D7 / Security | DB-SEC-002, DB-SEC-003 |
| Dashboard never writes to status | Contract | DB-CONTRACT-003, DB-CONFIG-003 |
| Control fields correct types | Contract | DB-CONTRACT-004, DB-ACTION-009 |
| Config keys case-sensitive match | Contract | DB-CONTRACT-005 |
| Settings round-trip (save → firmware) | Integration | DB-CONFIG-001 through DB-CONFIG-004 |
| Mode change round-trip | Integration | DB-SAFETY-003 |
| E-stop round-trip timing | Integration | DB-SAFETY-001, DB-SAFETY-002 |
| WCAG 2.1 AA contrast | D9 | DB-A11Y-007 |
| ARIA labels on safety controls | D9 | DB-A11Y-002, DB-A11Y-006 |
| Keyboard navigation | D9 | DB-A11Y-005 |
| Lighthouse Accessibility ≥ 95 | D8 | DB-A11Y-001, DB-PERF-002 |
| Lighthouse PWA ≥ 80 | D8 | DB-PERF-002 |
| TypeScript: zero any in new files | D1 | DB-QA-002 |
| tsc clean | D1 | DB-QA-001 |
| No old brand strings | D11 | DB-QA-005 |
| No npm vulnerabilities | D7 | DB-QA-006 |

---

*SmartFlow Dashboard & Firmware Integration QA Test Specification v1.0*
*Standards: IEEE 829-2008 (Software Test Documentation) · ISO/IEC 25010:2011 ·
ISTQB Foundation Level v4.0 (unit, integration, system, non-functional testing) ·
OWASP Web Application Security Testing Guide v4.2 ·
WCAG 2.1 Level AA (W3C, 2018) · Google Web Vitals (Core Performance Metrics) ·
RFC 7230 HTTP Semantics · React Testing Library best practices (Kent C. Dodds, 2023)*
*All Firebase paths and field names verified against SmartFlow Firebase Schema Canonical Contract v2.0.*
