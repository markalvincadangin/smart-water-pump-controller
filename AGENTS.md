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
