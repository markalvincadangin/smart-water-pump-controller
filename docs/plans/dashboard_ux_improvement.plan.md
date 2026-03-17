## Dashboard UI/UX Improvement Plan (Safety-Critical vNext)

This plan improves operator experience **without changing the firmware↔dashboard RTDB contract**.

### Design goals (human factors)

- **Make abnormal states obvious, but keep the normal screen calm**
  - Use a muted baseline palette; reserve high-saturation colors for abnormal conditions (high-performance HMI pattern).
- **Actionability over information**
  - Every red/amber banner must answer: *What happened? What is the risk? What should I do now?*
- **Prevent mode errors**
  - Reduce accidental activation (especially MANUAL ON and Emergency Stop) through clear affordances, disabled-state reasons, and consistent wording.
- **Support situation awareness**
  - Surface the two gating signals that matter for safe starts: `level_fresh` and `remote_sensor_stable`.

### References (standards + evidence)

- **ISA-101**: HMI design lifecycle + consistency and performance-first layouts.
- **ISA-18.2 / IEC 62682**: alarm philosophy and alarm management (alarms should be actionable; reduce nuisance).
- **IEC 60073**: color meaning assignments for man-machine interfaces (reserve red for critical/emergency).
- **High-performance HMI** guidance (industry practice): grayscale-first UI with restrained color use so alarms “pop”.

### Scope (what will change)

- **Fault messaging**
  - Ensure `last_fault_code` values emitted by firmware (e.g., `COMM_LOSS`, `E_STOP`, `STALE_LEVEL`) render a clear title/recovery text.
  - Provide a safe fallback for unknown fault codes.
- **Status visibility**
  - Show small, non-noisy indicators for:
    - `remote_sensor_stable` (RS-485 link stability)
    - `level_fresh` (freshness gate)
  - Keep “controller offline” as the highest priority banner.
- **Clarity & consistency**
  - Remove emoji-based semantics from status/alerts (use icons + text).
  - Replace legacy comments/labels that reference removed modes (FORCE_*).

### Acceptance criteria

- `npm run validate` passes (tests + lint + build) after each change set.
- No dashboard writes to deprecated control keys (`manual_start`, `manual_stop`).
- New/updated firmware fault codes are always visible to the operator with a recovery hint.
- Link-gate indicators match the current firmware fields:
  - `status.remote_sensor_stable`
  - `status.level_fresh`

