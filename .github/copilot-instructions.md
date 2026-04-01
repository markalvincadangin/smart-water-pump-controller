# Copilot Workspace Instructions for SmartFlow

Apply the SmartFlow skill and constraints for repository work.

## Always-on behavior
- Prioritize safety and reliability for pump-control behavior.
- Use additive, backward-compatible schema/protocol updates.
- Keep edits tightly scoped and avoid unrelated rewrites.
- Preserve documentation traceability in docs/audit and .plan artifacts.

## Skill routing
Use `.github/skills/smartflow/SKILL.md` for:
- Firmware changes in `firmware/**`
- Dashboard changes in `dashboard/**`
- Protocol/schema/deployment docs in `docs/**` and `.plan/**`
- Requests mentioning SmartFlow, ESP32, NodeMCU, RS-485, Firebase RTDB, dry-run, overflow, run_mode, or known bug IDs.

## Validation preference
- Dashboard: run type/build checks when practical.
- Firmware: keep compile compatibility and avoid breaking pin/protocol contracts.
