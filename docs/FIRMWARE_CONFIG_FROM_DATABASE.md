# Firmware Config from Database

**Status: Implemented.** Calibration and thresholds are stored in `/pump_system/config/device`; the ESP32 reads them every **30 s** when online and persists to NVS. The dashboard has a **Device config** (gear icon) modal to edit values; only authorized UIDs can write (same as control/mode).

This document also covers: (1) a quick firmware correctness check, and (2) the design for DB-backed config and offline behaviour.

---

## 1. Firmware check — current state

| Item | Status | Notes |
|------|--------|--------|
| **Pin mapping** | OK | GPIO 4 (relay), 5/18 (ultrasonic), 34 (flow) match docs and comments |
| **Firebase paths** | OK | `/pump_system/status` (write), `/pump_system/control/mode` and `clear_error` (read) align with `database.rules.json` |
| **Auth** | OK | Email/Password in code and README; `secrets.h.example` has the right defines |
| **Safety** | OK | Dry-run: 0.5 LPM / 30 s; pump OFF on boot; lockout until `clear_error` |
| **AUTO logic** | OK | Hysteresis 30% start / 100% stop; FORCE_ON/FORCE_OFF respected |
| **Docs** | OK | `libraries.txt` comment updated from "Anonymous Auth" to "Email/Password Auth" |

**Summary:** Firmware is consistent and correct. No code bugs found; only the libraries.txt comment was updated.

---

## 2. Proposed improvement: config from database

**Idea:** Store calibration and thresholds in Firebase (e.g. `/pump_system/config/device`) so you can change them from the dashboard or Firebase Console without reflashing the ESP32.

**Question:** If the ESP32 goes offline, will it still work?

**Answer:** Yes, if we design it like this:

- **When online:** ESP32 reads config from Firebase and uses it (and can optionally store it in NVS so it survives reboot).
- **When offline:** ESP32 uses **last-known config** from RAM, or from **NVS** if we persist it, or **compiled-in defaults** if nothing has been read yet.
- Pump logic (AUTO, dry-run, sensors) keeps running with whatever config is in memory; only Firebase sync is skipped when offline.

So operation continues when the ESP32 is offline; it just uses the last successfully applied config (or defaults on first boot before any read).

---

## 3. Feasibility: yes

Technically straightforward:

- Add a single config path in the database (e.g. `pump_system/config/device`).
- ESP32: on startup and/or periodically (e.g. every sync cycle) read that path; if successful, update in-RAM variables; optionally write to NVS after a successful read.
- Use **compiled-in defaults** for every field so that (a) first boot before any Firebase read, and (b) missing/corrupt DB values still give safe behavior.
- Validate ranges on the ESP32 (e.g. `PUMP_START_LEVEL` < `PUMP_STOP_LEVEL`); if invalid, ignore the update and keep previous config (or defaults).
- Dashboard (optional): add a “Device config” or “Calibration” screen that writes to the same path so you can change values without reflashing.

---

## 4. Pros and cons

### Pros

- **No reflash for calibration:** Change tank empty/full cm, flow factor, AUTO thresholds, dry-run threshold/timeout from dashboard or Firebase Console.
- **Single source of truth:** Same config can be shared across devices or edited in one place.
- **Offline-safe:** ESP32 runs on last-known (or default) config when WiFi/Firebase is down.
- **Easier tuning:** Adjust hysteresis or dry-run sensitivity without opening the codebase.

### Cons

- **More logic on ESP32:** Defaults, validation, optional NVS read/write, and “last good” config handling.
- **Security and rules:** Config path must be writable only by trusted clients (e.g. dashboard with auth); ESP32 needs read access (already has auth).
- **First boot / empty DB:** Must rely on firmware defaults until the first successful read (or you seed the database with default values).
- **Invalid data:** If someone writes bad values (e.g. start level > stop level), firmware must validate and reject (keep previous or defaults).

---

## 5. Implementation plan

### 5.1 Database schema

Add a single object at **`/pump_system/config/device`** (or `pump_system/config/calibration`). Suggested shape:

```json
{
  "tank_empty_cm": 122,
  "tank_full_cm": 8,
  "pump_start_level": 30,
  "pump_stop_level": 100,
  "dry_run_threshold_lpm": 0.5,
  "dry_run_timeout_sec": 30,
  "flow_calibration_factor": 1.0
}
```

- **Optional later:** `sensor_interval_ms`, `firebase_interval_ms` (keep timing in firmware initially to avoid accidental long intervals).
- **Not in DB:** Pin definitions and safety-critical constants (e.g. relay active LOW) stay in firmware.

### 5.2 Database rules

- **Read:** `auth != null` (ESP32 and dashboard can read).
- **Write:** Restrict to dashboard-only if desired (e.g. same UIDs that can write `control/mode`), so random clients cannot change calibration.

Example addition to `database.rules.json` under `pump_system.config`:

```json
"device": {
  ".read": "auth != null",
  ".write": "auth.uid === 'ZScmJXQg2tZSVDiZ1YxTUxN36xw1' || auth.uid === 'vQ4s0FyCmjMgoeltwcZR7WfckXZ2'"
}
```

(Adjust UIDs to match your authorized dashboard users.)

### 5.3 Firmware changes

1. **Defaults in code**  
   Keep `#define` defaults for every value (same as today). Use them when:
   - No config has been read yet, or
   - Read fails (offline, path missing, error), or
   - Validation fails.

2. **Runtime variables**  
   Replace direct use of macros in logic with variables, e.g.:
   - `int tankEmptyCm`, `int tankFullCm`
   - `int pumpStartLevel`, `int pumpStopLevel`
   - `float dryRunThresholdLpm`, `uint32_t dryRunTimeoutMs`
   - `float flowCalibrationFactor`  
   Initialize these in `setup()` from defaults.

3. **Read config from Firebase**  
   In the same place you currently call `readFirebaseControl()` (every 3 s when `Firebase.ready()`):
   - Call `Firebase.RTDB.getJSON(&fbdo, "/pump_system/config/device")`.
   - If success: parse each field, validate (e.g. `pump_start_level` < `pump_stop_level`, tank cm in a sane range, timeouts > 0).
   - If valid: update the runtime variables; optionally write to NVS for next boot.
   - If fail or invalid: leave variables unchanged (keep last good or defaults).

4. **Use variables everywhere**  
   In `readUltrasonicSensor()`, `executePumpLogic()`, `checkSafetyCutoff()`, `calculateFlowRate()` use the runtime variables instead of the `#define` constants.

5. **Optional: NVS persistence**  
   After a successful config read, write the JSON or each field to NVS. In `setup()`, before WiFi, read from NVS and apply so that after power loss the device starts with last-known config instead of only defaults.

### 5.4 Dashboard (optional)

- Add a “Device config” or “Calibration” page/section (protected by auth).
- Read `/pump_system/config/device` once; show form fields for each key.
- On Save, validate (e.g. start level < stop level) and write to `pump_system/config/device`.
- ESP32 will pick up changes on its next Firebase read (within 3 s when online).

### 5.5 Validation rules (ESP32)

- `tank_full_cm` < `tank_empty_cm` (e.g. 5–200 cm).
- `pump_start_level` < `pump_stop_level`, both in 0–100.
- `dry_run_threshold_lpm` > 0 (e.g. 0.1–10).
- `dry_run_timeout_sec` ≥ 10 (e.g. cap at 300).
- `flow_calibration_factor` > 0 (e.g. 0.1–10).

If any check fails, discard the update and keep previous config (or defaults).

### 5.6 Offline behavior summary

| Scenario | Behavior |
|----------|----------|
| First boot, no WiFi | Use compiled-in defaults for all parameters. |
| First boot, WiFi OK, DB empty | Read fails → use defaults. |
| First boot, WiFi OK, DB has config | Read succeeds → use DB config; optionally save to NVS. |
| Later, WiFi drops | Keep using in-RAM config (last successful read). Pump and safety logic unchanged. |
| WiFi back | Next 3 s cycle reads config again; if DB changed, apply new values. |

---

## 6. Suggested order of work

1. Add `pump_system.config.device` to database rules and seed default values in Firebase (or leave empty and rely on firmware defaults).
2. In firmware: introduce runtime variables and defaults; read `/pump_system/config/device` in the Firebase sync loop; validate and apply; use variables in all logic.
3. Test: change values in Firebase Console, confirm ESP32 behavior updates after a few seconds; test with ESP32 offline (unplug WiFi) and confirm it keeps running with last config.
4. (Optional) Add NVS persistence so last good config survives reboot.
5. (Optional) Add dashboard UI to edit `device` so you never need to touch Firebase Console.

This keeps the firmware correct and makes “recalibrate without reflashing” feasible while preserving correct behavior when the ESP32 is offline.
