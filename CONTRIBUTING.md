# SmartFlow Development Guide

SmartFlow is a safety-sensitive personal IoT project maintained by its owner. This guide documents the project's development and validation practices. External code contributions are not currently accepted, and public visibility does not grant permission to modify or redistribute the project.

## Start with the current specifications

- [Specification index](docs/specs/README.md)
- [Safety constitution](.specify/memory/constitution.md)
- [Firmware operational rules](docs/specs/firmware_operational_rules.md)
- [Android app behavior](docs/specs/app.md)
- [RS-485 protocol](docs/specs/rs485_protocol.md)

Non-trivial changes must follow the Spec Kit lifecycle and use an active feature directory under `specs/`.

## Components

| Component | Path | Stack |
|-----------|------|-------|
| Android app | `app/` | Kotlin, Jetpack Compose, Firebase SDK |
| Master firmware | `firmware/master_node/` | ESP32, C++, Arduino, PlatformIO |
| Sensor firmware | `firmware/sensor_node/` | ESP8266, C++, Arduino, PlatformIO |
| Cloud Functions | `functions/` | Node.js 22, TypeScript, Firebase Functions |

The earlier web-dashboard experiment is retired and is not an active development target.

## Local validation

### Android

Use JDK 21 and configure the Android SDK and `app/google-services.json` locally.

```powershell
./gradlew.bat test
./gradlew.bat assembleDebug
```

### Cloud Functions

```bash
cd functions
npm ci
npm run build
npm test
```

### Firmware

```bash
pio run -d firmware/master_node
pio run -d firmware/sensor_node
```

Hardware flashing and end-to-end control validation require the physical test setup. Do not energize mains equipment simply to satisfy a software test.

## Safety requirements

- Every fault or ambiguous state must bias the pump OFF.
- Relay changes must continue through the authorized pump-control boundary.
- Dry-run and overflow lockouts must persist until explicitly cleared.
- Sensor freshness and RS-485 validity must gate pump starts.
- The emergency-stop and recovery paths must remain reachable.
- Protocol and database changes must be additive and backward compatible.
- Software protection must never replace the independent thermal overload relay.

## Owner change reviews

Changes should remain focused and include:

- the problem and chosen approach;
- the related feature specification or issue;
- validation commands and results;
- hardware validation limitations;
- screenshots for visible Android changes;
- updates to the owning file under `docs/specs/` when behavior changes.

Use clear Conventional Commit messages, for example:

```text
fix(firmware): preserve e-stop polling during lockout
feat(app): show authoritative pending control state
docs(readme): update field deployment case study
```

Never commit credentials, `app/google-services.json`, firmware secret headers, service-account files, local database exports, or environment files. See the repository [proprietary notice](LICENSE) for permitted use.
