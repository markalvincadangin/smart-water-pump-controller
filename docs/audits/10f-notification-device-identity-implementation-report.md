# WP-10 N-FIX-08 — Device Identity / Display Name in Push Notifications

**Status:** VERIFIED  
**Scope:** Cloud Functions notification copy, canonical device display identity, RTDB protection

## 1. Finding

The existing SmartFlow contract did not contain a user-friendly device-name field. Device metadata currently contains firmware/hardware/protocol/serial identity, while the Android device list displayed the internal device ID directly.

Therefore, N-FIX-08 could not safely "read the existing device name" without inventing an authority.

## 2. Decision

Establish the optional canonical field:

`/devices/{deviceId}/metadata/displayName`

Rules:

- Optional user-facing name.
- Maximum 64 characters.
- Backend notification code treats it as untrusted input and normalizes it.
- Blank, invalid, or exact internal device-ID values fall back to **SmartFlow Pump**.
- Notification title/body copy must not use the raw device ID.
- Current firmware does not author `displayName`; device-name management remains a trusted backend/app concern.

The RTDB device metadata rule preserves `displayName` during firmware writes, preventing the device principal from changing the user-facing identity.

## 3. Implementation

### Cloud Functions

Added `functions/src/notificationIdentity.ts`:

- `resolveDeviceDisplayName(metadata, deviceId)`
- strips control characters
- normalizes whitespace
- trims to 64 characters
- rejects blank values
- rejects a value identical to the internal device ID
- falls back to `SmartFlow Pump`

Updated notification titles:

- Low Tank — {device name}
- Pump Started — {device name}
- Dry-Run Lockout — {device name}
- Maximum Runtime Protection — {device name}

Routing data continues to carry the internal `deviceId` separately for app navigation; it is not placed in user-facing notification text.

### Firebase contract/rules

Updated `docs/specifications/smartflow-system-contract.md` to define `metadata.displayName`.

Updated `database.rules.json` so device-authenticated firmware writes must preserve the existing display name.

## 4. Tests Added

`functions/src/__tests__/notificationIdentity.test.ts` covers:

1. valid custom display name
2. missing metadata
3. blank display name
4. internal device-ID rejection
5. control-character/whitespace normalization
6. 64-character length bound
7. non-string metadata rejection

## 5. Verification Results

### Cloud Functions
- `npm test -- --runInBand` passed (43 / 43 tests passed across all 5 test suites).
  - `notificationIdentity.test.ts`: all 5 test scenarios passed (valid display name, missing/blank fallback, internal device ID rejection, whitespace/control-character normalization and length bounds, non-string metadata rejection).
- `npm run build` (`tsc`) compiled cleanly with 0 errors.

### Android
- `.\gradlew.bat compileDebugSources` passed (BUILD SUCCESSFUL).
- `.\gradlew.bat testDebugUnitTest --rerun-tasks` passed (39 / 39 tests passed across all 6 test suites with 0 failures, 0 errors, 0 skipped).

### Acceptance criteria satisfied
- all existing Cloud Functions tests pass
- new notification identity tests pass
- TypeScript build passes
- Android compilation/tests remain green
- no notification title/body contains a raw internal device ID
- missing/blank display name produces `SmartFlow Pump`


## 6. Scope Boundary

N-FIX-08 does **not** add a device-name editing screen. That is intentionally left as a separate app-management concern. Until such a UI/backend management path exists, production notifications safely use the fallback name.

## 7. Commits

Implementation commits on `docs/smartflow-system-contract`:

- `20182206` — notification identity resolver
- `096d124` — notification title enrichment
- `a809d6d` — identity tests
- `4b46b7e` — RTDB protection
- `7ad8148` — system-contract update
- `b84e369` — reject embedded device IDs in resolver
- `20d83ee` — test embedded device-ID rejection
