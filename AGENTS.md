# AGENTS

Repository agent routing for SmartFlow.

## Default Agent
- Name: Copilot
- Model: GPT-5.3-Codex

## Skill Discovery
- Primary project skill: `.github/skills/smartflow/SKILL.md`
- Workspace instructions: `.github/copilot-instructions.md`

## Auto-apply Scope
Use SmartFlow skill for work in:
- `firmware/**`
- `dashboard/**`
- `docs/**`
- `.plan/**`

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
| **Dashboard** | `dashboard/` | Next.js 15, TypeScript, Tailwind, Firebase SDK | See `dashboard/package.json` scripts |
| **Cloud Functions** | `functions/` | Node.js 22, TypeScript, firebase-functions v7 | See `functions/package.json` scripts |
| **Admin Scripts** | `scripts/` | Node.js, firebase-admin | One-off utilities, not routinely run |

### Dashboard
- Uses `npm` (has `package-lock.json`). Run `npm install` then `npm run dev` (port 3000).
- Requires `dashboard/.env.local` copied from `.env.local.example` with Firebase credentials. Without real credentials the app renders the login page but cannot sign in.
- Lint: `npm run lint`, Tests: `npm test`, Build: `npm run build`, Full validation: `npm run validate`.
- 4 tests in `__tests__/lib/usePumpData.test.tsx` have pre-existing failures (mock uses `set` but source uses `update`). The other test suites pass.

### Cloud Functions
- `cd functions && npm install && npm run build` (TypeScript compile). Tests: `npm test`.

### Gotchas
- No Docker, no monorepo tooling, no pre-commit hooks.
- Firmware directories (`firmware/`) contain Arduino/PlatformIO code for ESP32/ESP8266 — compilation requires PlatformIO CLI (`pip install platformio`) but flashing requires physical USB boards.
- The RTDB export JSON file in the repo root (named after the Firebase project with `-default-rtdb-export.json` suffix) can be imported into Firebase for realistic test data without hardware.
- When killing the Next.js dev server, orphaned `next-server` child processes may remain bound to the port. Use `netstat -tlnp | grep 300` to find and kill them by PID before restarting.
- The dashboard has a health endpoint at `/api/health` that returns `{"status":"ok","firebase":"initialized"}` when Firebase is properly configured — useful for quick verification.
- `.env.local` is gitignored. Firebase secrets are injected as environment variables; write them to `dashboard/.env.local` before running the dev server.
