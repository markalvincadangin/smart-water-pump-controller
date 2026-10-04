# SmartFlow Contributing & Development Guide

Thank you for your interest in SmartFlow! SmartFlow is an automated deep-well pump controller and overhead water tank monitoring system designed for real-world residential reliability and safety.

The project is licensed under the [Apache License 2.0](LICENSE). Because SmartFlow directly switches high-voltage inductive mains electricity (220V AC, 1.5 HP motor), all contributions must prioritize electrical and mechanical safety above all else.

---

## 1. Specifications & Architecture Documentation

Before proposing architectural changes or adding features, familiarize yourself with the formal specifications:

- **[Specification Index](docs/specs/README.md)**: Master architectural map.
- **[Safety Constitution](.specify/memory/constitution.md)**: Core non-negotiable safety principles.
- **[Firmware Operational Rules](docs/specs/firmware_operational_rules.md)**: State machine logic, dry-run cutoffs, and fail-safe transitions.
- **[Android Application Behavior](docs/specs/app.md)**: Reactive state synchronization, BLE ownership claim, and UI rules.
- **[RS-485 Framing Protocol](docs/specs/rs485_protocol.md)**: Half-duplex packet framing, CRC16 error detection, and register maps.
- **[Pre-Energization Verification](DEPLOYMENT_SAFETY.md)**: Physical safety checklist and emergency procedures.
- **[Security Policy](SECURITY.md)**: Vulnerability disclosure and responsible research boundaries.

---

## 2. Components & Tech Stack

| Component | Path | Technology Stack |
|-----------|------|------------------|
| **Android App** | `app/` | Kotlin, Jetpack Compose (Material 3), RxAndroidBle3, Firebase SDK |
| **Master Controller** | `firmware/master_node/` | ESP32 DevKit V1, C++, PlatformIO, FreeRTOS, NimBLE |
| **Tank Sensor Node** | `firmware/sensor_node/` | NodeMCU V2 (ESP8266), C++, PlatformIO, UART/RS-485 |
| **Cloud Functions** | `functions/` | Node.js 22, TypeScript, Firebase Admin & Functions SDK |

---

## 3. Branching Strategy & Git Workflow

We follow a standardized Git workflow:

```text
feature/xyz  ──┐
fix/abc      ──┼──► develop (Active Integration) ──► PR / Release ──► main (Stable Production)
docs/update  ──┘
```

- **`main`**: Protected, production-ready branch. Receives pull requests exclusively from `develop` when an integration milestone has been validated.
- **`develop`**: Active integration branch. All feature branches, bug fixes, and maintenance branches branch off and target `develop`.
- **Branch Naming**:
  - `feature/<short-description>`: New capabilities or enhancements.
  - `fix/<issue-description>`: Bug fixes or error resolution.
  - `docs/<topic>`: Documentation updates and specifications.
  - `refactor/<subsystem>`: Code cleanups with no behavior change.

---

## 4. Local Build & Test Validation

All automated tests must pass before submitting a pull request.

### Android Application
Requires JDK 17 or 21 and Android SDK 34:
```powershell
./gradlew.bat test
./gradlew.bat assembleDebug
```

### Cloud Functions
Requires Node.js 22:
```bash
cd functions
npm ci
npm run build
npm test
```

### Microcontroller Firmware
Requires PlatformIO Core:
```bash
# Sensor Node (ESP8266)
pio run -d firmware/sensor_node

# Master Node (ESP32)
pio run -d firmware/master_node
```

> **SAFETY WARNING:** Firmware can be compiled and unit-tested without physical hardware. **Never energize 220V mains wiring solely to validate software changes.**

---

## 5. Non-Negotiable Safety Rules

Every contribution touching firmware, Android control intent, or database rules must strictly uphold these safety invariants:

1. **Bias Towards OFF**: Any hardware fault, watchdog timeout, invalid sensor packet, or ambiguous state must immediately de-energize the pump relay.
2. **Independent Hardware Protection**: Software logic must never be treated as a replacement for the mechanical miniature circuit breaker (MCB) and thermal overload relay (TOR).
3. **Persistent Lockouts**: Dry-run and overflow fault states must be written to non-volatile storage (NVS) and require explicit user intervention to reset.
4. **Sensor Freshness Gating**: The pump cannot start or remain running without fresh, CRC16-validated telemetry from the tank sensor node within the timeout window.
5. **Emergency Stop Reachability**: Physical emergency-stop inputs and digital application e-stops must be processed with highest priority across all operating modes (AUTO, MANUAL, COUNTDOWN).
6. **No Committed Credentials**: Never commit `secrets.h`, `google-services.json`, Firebase service account keys, or local environment files (`.env`).

---

## 6. Commit Message Guidelines

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```text
<type>(<scope>): <short description>
```

- **`feat`**: A new feature (e.g., `feat(app): add countdown timer presets`).
- **`fix`**: A bug fix (e.g., `fix(firmware): preserve e-stop polling during dry-run lockout`).
- **`docs`**: Documentation only (e.g., `docs(readme): add RS-485 wiring notes`).
- **`test`**: Adding or updating tests (e.g., `test(functions): add RTDB authorization test`).
- **`refactor`**: Code change that neither fixes a bug nor adds a feature.

---

## 7. Submitting a Pull Request

1. Fork the repository and create your branch from `develop`:
   ```bash
   git checkout -b feature/my-new-feature origin/develop
   ```
2. Implement your changes following existing code formatting and documentation standards.
3. Verify that all test suites pass (`./gradlew.bat test`, `npm test`, `pio run`).
4. If modifying control logic, complete the verification checklist in [DEPLOYMENT_SAFETY.md](DEPLOYMENT_SAFETY.md).
5. Push to your fork and submit a Pull Request targeting the **`develop`** branch.
6. Provide a concise summary of the problem, proposed solution, and validation commands executed.

---

## 8. Code of Conduct

We are committed to providing a welcoming, inclusive, and professional environment. Treat all contributors and reviewers with respect, critique code rather than individuals, and maintain constructive, empathetic discussions.
