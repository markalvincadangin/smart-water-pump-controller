# Publication Review

## Current decision

**Keep this development repository private.** A Gitleaks full-history scan found no provider-specific secret rule, but it found historical hardcoded Firebase authentication identifiers in old rules and documentation. Publish a separate sanitized snapshot repository without this private history. The project is proprietary and made available for portfolio review only; it is not open source.

## Proposed GitHub metadata

**Description**

> Field-deployed ESP32/ESP8266 water-pump controller with RS-485 sensing, layered safety, Firebase services, and a native Android app.

**Topics**

`iot`, `esp32`, `esp8266`, `android`, `kotlin`, `jetpack-compose`, `firebase`, `platformio`, `rs485`, `embedded-systems`, `home-automation`, `water-pump`

**Homepage**

Clear the obsolete Vercel URL. Leave the field empty until a current portfolio page or demo video exists.

**Visibility**

Remain private until the owner explicitly approves publication after the history and image review.

## Validation evidence

- Android unit tests: passed.
- Android debug assembly: passed.
- Cloud Functions TypeScript build: passed.
- Cloud Functions tests: 26 passed across 3 suites.
- README local links: passed.
- Selected public images: no GPS metadata; no visible account, network, location, or device identifier.
- Current tracked-text scan: one expected placeholder Firebase configuration match; no confirmed active credential.
- Git history: credential-bearing paths existed in earlier revisions; automated history scan still required.
- Copyright: all-rights-reserved proprietary notice applied; third-party dependencies retain their own licenses.
- Dependency audit after remediation: 0 critical, 0 high, 9 moderate, 0 low. Remaining findings are in the Firebase Functions / Firebase Admin / Google Cloud chain; npm's forced proposal would introduce a breaking downgrade and was rejected.
- History scan: 116 commits scanned with redaction. Generic matches were classified as historical Firebase authentication identifiers rather than provider-specific secrets. Keep them out of the public snapshot.

## Resume bullets

- Designed and deployed an end-to-end IoT controller for a residential 1.5 HP water pump, integrating ESP32/ESP8266 firmware, a native Kotlin/Jetpack Compose app, and Firebase services.
- Implemented a CRC-protected RS-485 sensor network and fail-toward-OFF control logic with stale-data gating, emergency stop, dry-run lockout, overflow cutoff, and independent thermal overload protection.
- Built secure BLE-to-cloud provisioning, device ownership workflows, real-time telemetry, remote operating modes, notifications, diagnostics, and automated Android/Cloud Functions validation.

## Before publication

1. Create a sanitized public snapshot repository without the private development history.
2. Exclude ignored secrets, original sensitive screenshots, internal identifiers, local exports, and unrelated in-progress files.
3. Review GitHub secret-scanning, dependency, and code-scanning settings on the public repository.
4. Confirm the exact snapshot and metadata with the owner.
5. Make only the sanitized repository public after explicit approval; keep this development repository private.
