# Admin Bootstrap and Rule Validation

This runbook validates SmartFlow access control after `database.rules.json` deployment.

## Scope

- Bootstrap first dashboard admin at `pump_system/config/admins/{uid}`
- Validate that non-admin users cannot write privileged control fields
- Confirm ESP32 service-account behavior remains functional

## Current rule model

From `database.rules.json`:

- `pump_system/control/mode`: writable by admin UIDs and ESP32 Email/Password client
- Privileged control fields (`manual_desired`, `bypass_*`, `reboot_request_id`, etc.): admin UIDs only
- ESP32 can write one-shot `false` resets for selected control keys (`emergency_stop`, `reset_stop`, `countdown_start`, `clear_error`, `countdown_add_time`, `countdown_stop`)
- `pump_system/config/device`: admin UIDs only
- `pump_system/status` and `pump_system/logs/errors`: ESP32 Email/Password client write path

## 1. Bootstrap first admin

1. Get the dashboard user UID:
  - Firebase Console -> Authentication -> Users
2. Run the bootstrap script:

```bash
cd scripts
npm install
node bootstrap-admin.js --keyfile /path/to/serviceAccountKey.json YOUR_ADMIN_UID
```

3. Confirm output contains:

```text
Admin bootstrap OK: pump_system/config/admins/YOUR_ADMIN_UID = true
```

4. Verify in RTDB Data view:

```text
pump_system/config/admins/YOUR_ADMIN_UID = true
```

## 2. Validate rules behavior

### Admin path must succeed

1. Sign in to dashboard with bootstrapped admin account.
2. Change `control/mode` between `AUTO`, `MANUAL`, and `COUNTDOWN`.
3. Expected: write succeeds, UI state updates.

### Non-admin path must fail

1. Use a second authenticated user not present in `config/admins`.
2. Attempt write to:

```text
/pump_system/control/manual_desired
```

3. Expected: permission denied.

### ESP32 compatibility must remain intact

1. Confirm ESP32 continues writing to:

```text
/pump_system/status
```

2. Confirm control one-shot self-clears can still be set back to `false` by ESP32 where applicable.

## 3. Rule simulation checklist

| Check | Expected result |
|------|------------------|
| Admin write to `control/mode` | Allowed |
| Non-admin write to `control/mode` | Denied |
| Non-admin write to privileged control field | Denied |
| ESP32 status write | Allowed |
| ESP32 one-shot reset (`false`) on allowed keys | Allowed |

If any expected rule behavior fails, stop release progression and resolve before launch.
