# AGENTS

Repository agent routing for SmartFlow.

## Default Agent
- Name: Copilot
- Model: GPT-5.3-Codex

## Source of truth

**Canonical current-state specs:** [`docs/specs/`](docs/specs/README.md) — one owner file per domain; cross-reference, do not duplicate.

**Supporting documentation:** [`docs/`](docs/README.md) — operations runbooks (`docs/operations/`), audit findings (`docs/audit/`), ADRs (`docs/adr/`), archive (`docs/archive/`). Use what specs point to.

**Safety principles:** [`.specify/memory/constitution.md`](.specify/memory/constitution.md) — non-negotiable; not duplicated into specs.

**Feature work:** [`specs/`](specs/) — Spec Kit outputs (`spec.md`, `plan.md`, `tasks.md`, etc.) per feature directory. Do not add feature specs under `docs/specs/`.

## Spec Kit lifecycle (mandatory)

All non-trivial product or engineering work MUST follow Spec Kit. Do not jump straight to code without an active feature under `specs/[###-name]/`.

| Step | Cursor skill | Purpose |
|------|----------------|---------|
| 1 | `/speckit-specify` | Feature specification |
| 2 | `/speckit-clarify` | Resolve ambiguities (when needed) |
| 3 | `/speckit-plan` | Technical plan |
| 4 | `/speckit-tasks` | Task breakdown |
| 5 | `/speckit-implement` | Implementation against tasks |

Use `/speckit-analyze`, `/speckit-checklist`, and `/speckit-converge` at review gates when appropriate. For brownfield onboarding or migration, use `/speckit-brownfield-scan`, `/speckit-brownfield-bootstrap`, `/speckit-brownfield-migrate`, and `/speckit-brownfield-validate`.

When implementation changes canonical behavior, update the owning file in `docs/specs/` in the same change set.

## Skill Discovery
- Workspace instructions: `.github/copilot-instructions.md`
- Spec Kit skills: `.cursor/skills/speckit-*` (cursor-agent integration)

## Cursor
- Project skills: `.cursor/skills/` (also discovers `.agents/skills/` and `.claude/skills/`)
- Project rules: `.cursor/rules/` (`graphify.mdc`, `smartflow.mdc`)
- Project MCP: `.cursor/mcp.json` (graphify, GitHub)
- Graphify: `graphify install --platform cursor --project` (already applied)

## Auto-apply Scope
Apply SmartFlow conventions (`.cursor/rules/smartflow.mdc`, `docs/specs/`, Spec Kit lifecycle) for work in:
- `firmware/**`
- `app/**`
- `functions/**`
- `docs/**`
- `specs/**`

## Trigger Terms
SmartFlow, ESP32, NodeMCU, RS-485, Firebase RTDB, dry-run, overflow, run_mode, JSN-SR04T, YF-G1, C-01, C-02, H-02, H-03, H-04, H-05, H-06, H-07, M-01, M-02, M-03, M-05, M-06.

## Notes
- Keep safety constraints primary for firmware-affecting work.
- Keep protocol/schema changes backward compatible.
- Do not treat `.claude/` as required routing for this repo configuration.

## Cursor Cloud specific instructions

### Repository layout
This is an IoT project with three software components (firmware requires physical hardware and cannot be tested on the VM):

| Component | Path | Stack | Dev commands |
|-----------|------|-------|-------------|
| **Android App** | `app/` | Kotlin, Jetpack Compose, Firebase SDK | `.\gradlew.bat assembleDebug` (with Java 21) |
| **Cloud Functions** | `functions/` | Node.js 22, TypeScript, firebase-functions v7 | See `functions/package.json` scripts |

### Android App
- Uses Gradle (`build.gradle.kts`). Run `.\gradlew.bat assembleDebug` to build.
- Ensure your `JAVA_HOME` is set to JDK 21 and the Android SDK is configured via `local.properties`.

### Cloud Functions
- `cd functions && npm install && npm run build` (TypeScript compile). Tests: `npm test`.

### Gotchas
- No Docker, no monorepo tooling, no pre-commit hooks.
- Firmware directories (`firmware/`) contain Arduino/PlatformIO code for ESP32/ESP8266 — compilation requires PlatformIO CLI (`pip install platformio`) but flashing requires physical USB boards.
- The RTDB export JSON file in the repo root (named after the Firebase project with `-default-rtdb-export.json` suffix) can be imported into Firebase for realistic test data without hardware.
- The Android app requires `google-services.json` to be configured with the correct Firebase project credentials. Ensure it is placed in the `app/` directory.
