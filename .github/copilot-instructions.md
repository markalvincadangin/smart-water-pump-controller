# Copilot Workspace Instructions for SmartFlow

Apply SmartFlow safety constraints, **docs/specs as source of truth**, and the **mandatory Spec Kit lifecycle** for all substantive work.

## Source of truth

1. **Current implementation:** [`docs/specs/`](docs/specs/README.md) — normative specs per domain.
2. **Supporting docs:** [`docs/`](docs/README.md) — operations, audit, ADR, archive (as referenced by specs).
3. **Safety:** [`.specify/memory/constitution.md`](.specify/memory/constitution.md).
4. **Features in flight:** [`specs/[###-name]/`](specs/) — Spec Kit artifacts only; not under `docs/specs/`.

## Spec Kit lifecycle (mandatory)

Do not implement non-trivial changes without an active feature directory under `specs/`.

1. `/speckit-specify` → 2. `/speckit-clarify` (if needed) → 3. `/speckit-plan` → 4. `/speckit-tasks` → 5. `/speckit-implement`

Use analyze, checklist, and converge skills at review gates. Use brownfield skills when adopting or migrating SDD on existing code.

Update the owning `docs/specs/` file when canonical behavior changes.

## Always-on behavior

- Prioritize safety and reliability for pump-control behavior.
- Use additive, backward-compatible schema/protocol updates.
- Keep edits tightly scoped and avoid unrelated rewrites.
- Preserve documentation traceability in `docs/audit/` and feature specs under `specs/`.

## Scope

Apply the above for firmware (`firmware/**`), app (`app/**`), functions (`functions/**`), docs, and feature specs — including requests mentioning SmartFlow, ESP32, NodeMCU, RS-485, Firebase RTDB, dry-run, overflow, run_mode, or known bug IDs.

## Validation preference

- App: `.\gradlew.bat assembleDebug` when practical.
- Functions: `npm run build` and `npm test` when practical.
- Firmware: compile compatibility; do not break pin/protocol contracts.
