# SmartFlow Dashboard Implementation Plan

## 1) Objective

Build and maintain a modern, minimalist, professional SmartFlow dashboard using `React + Next.js` that is fully compatible with the current firmware behavior and Firebase data contracts.

This plan defines:
- The exact UI behavior for control modes and safety flows
- The data fields that must be read and written
- The visual/design system and icon asset standards
- The implementation and validation checklist to keep `dashboard` and `firmware` aligned

---

## 2) Product and Technical Scope

### In Scope
- Next.js dashboard (`dashboard/`) as the primary web UI
- Firebase Realtime Database integration for real-time status, control, config, and notifications
- Firmware-compatible control flows:
  - mode switching (`AUTO`, `MANUAL`, `COUNTDOWN`)
  - manual pump intent
  - countdown start/extend/stop
  - emergency stop and reset
  - reboot request
  - bypass toggles
- Clean professional visual refresh based on a minimalist design language
- Professional SVG icon assets downloaded from web and served locally

### Out of Scope
- Legacy single-file HTML dashboard concepts
- Chart.js-specific references
- Placeholder/generic SVGs and decorative icon sets without product purpose
- Simulation-mode-only behavior not present in production code paths

---

## 3) Source of Truth and Compatibility Rules

Compatibility must always prioritize:
1. Firmware behavior (`firmware/platformio_smart_water_pump_controller`)
2. Dashboard runtime contract (`dashboard/lib/types.ts`, hooks, and component usage)
3. Firebase RTDB paths currently used by the app

If any documentation statement conflicts with actual code behavior, update the plan to match code and firmware.

---

## 4) Data Contract (Firmware <-> Dashboard)

### 4.1 RTDB Paths
- `pump_system/status` (read)
- `pump_system/control` (read/write)
- `pump_system/config/device` (read/write)
- `pump_system/config/notifications_by_user/{uid}` (read/write)

### 4.2 Status (Read)
Dashboard consumes controller state and telemetry from `status`, including operational and safety fields used by UI rendering and alerts.

Required compatibility highlights:
- `run_mode` is derived by firmware and displayed by UI
- lockouts/fault states determine user-facing alert priority
- online/stale detection is based on latest timestamp and stale threshold

### 4.3 Control (Read/Write)
Dashboard writes only firmware-supported control values:
- `mode`: `AUTO | MANUAL | COUNTDOWN`
- `manual_desired`
- `countdown_start`
- `countdown_add_time`
- `countdown_stop`
- `emergency_stop`
- `reset_stop`
- `clear_error`
- `bypass_level_sensor`
- `bypass_flow_sensor`
- `reboot_request_id`

Write policy:
- Use explicit one-shot writes for commands
- Keep command semantics consistent with firmware-side command consumption/reset behavior

### 4.4 Device Config (Read/Write)
Dashboard exposes firmware-backed editable settings under `config/device`, including thresholds, timings, calibration, and resilience settings used in current code paths.

### 4.5 Notification Config (Read/Write)
Per-user notification preferences are saved under:
- `config/notifications_by_user/{uid}`

UI must only present settings that are wired to actual notification behavior supported in app/infra.

### 4.6 Implementation Status Acknowledgment (Firmware vs Dashboard)

This section records real code status so the plan stays honest and compatible.

#### Option B (COUNTDOWN stays in COUNTDOWN when stopped/expired) status
- **Firmware:** implemented in `platformio_smart_water_pump_controller`:
  - supports `countdown_stop` one-shot
  - includes boot guard for `countdownConsumed` to avoid unintended auto-rearm after reboot
  - supports COUNTDOWN idle semantics instead of forced AUTO in Option B paths
- **Dashboard:** implemented for countdown stop flow:
  - `usePumpData.stopCountdown()` writes `control/countdown_stop`
  - activity/audit already recognize `control.countdown_stop`
- **Plan policy:** treat Option B as the standard behavior.

#### Flow sensor bypass status
- **Firmware:** implemented:
  - control input `bypass_flow_sensor`
  - persisted via NVS
  - published in status (`bypass_flow_sensor`)
  - integrated in safety logic (dry-run/flow-stuck bypass behavior)
- **Dashboard:** **not fully implemented yet**:
  - no typed field exposure for `bypass_flow_sensor` in current dashboard type contract
  - no hook action equivalent to level bypass setter
  - no UI toggle in settings panel equivalent to level/ultrasonic bypass control
- **Plan requirement:** add first-class dashboard support for flow sensor bypass (type, hook write, UI control, audit/activity label, and alert handling consistency).

---

## 5) UX and Visual Direction (Modern Minimalist Professional)

### 5.0 HCI Principles (Mandatory)
Apply these principles throughout design and implementation decisions without exception.

| Principle | Application in SmartFlow |
| --- | --- |
| Visibility of system status (Nielsen #1) | Pump state, tank level, and sensor health are always visible at a glance with no hidden critical status. |
| Match between system and real world | Tank visuals fill from bottom-up, flow is represented as movement/rate, and pump ON/OFF states use intuitive semantic colors. |
| Error prevention over error recovery | Show clear warnings before destructive/high-risk actions and disable controls that are illegal in the current mode/state. |
| Recognition over recall | Mode names, status labels, and control states are self-explanatory and readable without requiring legends. |
| Aesthetic and minimalist design (Nielsen #8) | Show only operator-relevant information by default; use progressive disclosure for diagnostics and history. |
| Fitts's Law | Primary controls (pump toggle, emergency stop) are large, reachable, and spatially separated to reduce mis-taps. |
| Gestalt proximity | Related information is grouped into clear clusters: tank status, pump control, sensor health, and history/diagnostics. |

### 5.1 Visual Identity
- Product name: `SmartFlow`
- Tagline: `Water, managed.`
- Timezone display standard: `PHT (UTC+8)`
- Timestamp rule: all user-facing timestamps are rendered in Philippine Standard Time

### 5.2 Design Principles
- Clean hierarchy, ample spacing, restrained color usage
- Information-dense but uncluttered cards and sections
- Strong readability in light and dark themes
- Purpose-driven motion only (no decorative animation)
- Safety and fault states visually prioritized over neutral telemetry

### 5.3 Color System (Tailwind Token Source)
- All colors are defined as design tokens and mapped to Tailwind theme variables.
- Do not hardcode color hex values in component-level styles.
- Semantic status colors must keep consistent meaning across light and dark themes.

#### 5.3.1 Dark Theme (Default) Token Values

| Token | Value |
| --- | --- |
| `bg-base` | `#0A0E14` |
| `bg-surface` | `#111720` |
| `bg-elevated` | `#1A2232` |
| `bg-overlay` | `#232E42` |
| `border-subtle` | `#1E2A3A` |
| `border-default` | `#2A3A50` |
| `border-focus` | `#3B82F6` |
| `brand-400` | `#60A5FA` |
| `brand-500` | `#3B82F6` |
| `brand-600` | `#2563EB` |
| `brand-glow` | `rgba(59,130,246,0.15)` |
| `status-ok` | `#10B981` |
| `status-ok-dim` | `rgba(16,185,129,0.12)` |
| `status-warn` | `#F59E0B` |
| `status-warn-dim` | `rgba(245,158,11,0.12)` |
| `status-error` | `#EF4444` |
| `status-error-dim` | `rgba(239,68,68,0.12)` |
| `status-idle` | `#6B7280` |
| `status-idle-dim` | `rgba(107,114,128,0.10)` |
| `water-low` | `#EF4444` |
| `water-mid` | `#F59E0B` |
| `water-high` | `#3B82F6` |
| `water-full` | `#10B981` |
| `text-primary` | `#F0F4F8` |
| `text-secondary` | `#8899AA` |
| `text-tertiary` | `#556070` |
| `text-inverse` | `#0A0E14` |
| `text-data` | `#E2E8F0` |
| `text-unit` | `#64748B` |

#### 5.3.2 Light Theme (Clean Industrial) Token Values

| Token | Value |
| --- | --- |
| `bg-base` | `#F0F4F8` |
| `bg-surface` | `#FFFFFF` |
| `bg-elevated` | `#E8EEF5` |
| `bg-overlay` | `#D8E2ED` |
| `border-subtle` | `#D1DCE8` |
| `border-default` | `#B8C8D8` |
| `border-focus` | `#3B82F6` |
| `brand-400` | `#2563EB` |
| `brand-500` | `#1D4ED8` |
| `brand-600` | `#1E40AF` |
| `brand-glow` | `rgba(59,130,246,0.10)` |
| `status-ok` | `#10B981` |
| `status-ok-dim` | `rgba(16,185,129,0.10)` |
| `status-warn` | `#D97706` |
| `status-warn-dim` | `rgba(217,119,6,0.10)` |
| `status-error` | `#DC2626` |
| `status-error-dim` | `rgba(220,38,38,0.10)` |
| `status-idle` | `#9CA3AF` |
| `status-idle-dim` | `rgba(156,163,175,0.12)` |
| `water-low` | `#EF4444` |
| `water-mid` | `#F59E0B` |
| `water-high` | `#3B82F6` |
| `water-full` | `#10B981` |
| `text-primary` | `#0F172A` |
| `text-secondary` | `#475569` |
| `text-tertiary` | `#94A3B8` |
| `text-inverse` | `#F0F4F8` |
| `text-data` | `#1E293B` |
| `text-unit` | `#94A3B8` |

#### 5.3.3 Elevation Rule
- Light theme panel cards use subtle elevation and border.
- Dark theme panels keep flatter surfaces (minimal shadow).

### 5.4 Typography
- UI font family: `Inter`
- Data font family: `JetBrains Mono`
- Load fonts through Next.js font pipeline (preferred) or approved web font source.

#### 5.4.1 Typographic Roles

| Role | Spec |
| --- | --- |
| Hero metric (tank %, flow rate) | `3rem`, `600`, data font |
| Section heading | `0.75rem`, `600`, UI font, uppercase, letter spacing `0.12em`, tertiary text color |
| Body label | `0.875rem`, `500`, UI font |
| Micro label / unit | `0.75rem`, data font, unit text color |
| Numeric values / codes / timestamps | always data font |

#### 5.4.2 Typography Rules
- Use data font for every number, code, fault string, and timestamp.
- Use UI font for labels, navigation, and buttons.
- Do not mix UI/data fonts in the same semantic element.

### 5.5 Layout System
- Responsive grid using Tailwind utilities and existing component structure
- Mobile-first behavior with progressive enhancement for desktop
- Clear separation:
  - System status + header actions
  - Live operational controls
  - Telemetry/history/diagnostics
  - Configuration and notification settings

### 5.6 Component Visual Standards
- Unified card primitives (radius, border, elevation, spacing)
- Consistent button hierarchy (primary, secondary, danger)
- Consistent form control sizing and label/help patterns
- No inline style experiments outside design tokens/utilities

---

## 6) Professional SVG Icon System (No Generic Icons)

### 6.1 Requirement
Do not use ad hoc/generic icons or random SVG snippets. Use a curated professional icon set downloaded from reputable sources, then serve icons locally.

### 6.2 Approved Sources
- [Tabler Icons](https://tabler.io/icons) (MIT)
- [Heroicons](https://heroicons.com/) (MIT)
- [Phosphor Icons](https://phosphoricons.com/) (open-source license; verify selected assets)

Use one primary icon family for consistency. Avoid mixing visual styles unless intentional and documented.

### 6.3 Asset Workflow
1. Select exact icons per UI semantic slot.
2. Download SVG files from approved source.
3. Store under `dashboard/public/icons/<set-name>/`.
4. Optimize SVGs (size/paths) while preserving shape quality.
5. Create a typed icon map in dashboard code to prevent arbitrary asset usage.
6. Reference icons through a single `AppIcon` abstraction/component.

### 6.4 Icon Usage Rules
- Every icon must communicate function/state (never decorative filler)
- Keep stroke/weight/style uniform across dashboard
- Use accessible labels where icon-only controls exist
- Avoid duplicated meanings across different icons

---

## 7) Functional UX Flows (Firmware-Compatible)

### 7.1 Mode Control
- Writable modes: `AUTO`, `MANUAL`, `COUNTDOWN`
- UI must not present writable `OFF` mode
- Displayed run state comes from firmware `run_mode` (derived)

### 7.2 Pump Runtime Controls
- `AUTO`: automatic behavior, no manual force toggle
- `MANUAL`: toggle uses `manual_desired`
- `COUNTDOWN`: start/extend/stop via one-shot fields

### 7.3 Emergency and Safety
- Emergency stop writes `emergency_stop: true` immediately
- Reset stop uses `reset_stop: true` (subject to firmware safety checks)
- Alert surface must prioritize E-stop latch, lockouts, and active fault conditions

### 7.4 Sensor Bypass
- Expose `bypass_level_sensor` and `bypass_flow_sensor` with explicit warning UI
- Bypass controls must reflect real firmware state and persistence behavior
- If sensors are unreliable but operator confirms physical water flow/pump operation, bypass controls remain available as explicit manual override with high-visibility caution messaging

### 7.5 Reboot / Restart
- Trigger reboot by writing unique `reboot_request_id`
- UI should show pending feedback and prevent duplicate accidental requests

### 7.6 Configuration and Notification Settings
- Device settings page/section edits firmware-backed config values only
- Notification settings are user-scoped and persist under per-user path

---

## 8) Dashboard Structure (Next.js / React)

### 8.1 Core Composition
Use current componentized architecture (App Router) rather than monolithic page logic.

Primary areas:
- Header/status section
- Main control and telemetry grid
- History/activity/diagnostics sections
- Config + notification settings controls

### 8.2 State and Data Hooks
- Real-time subscription hook(s) for status/control/config
- Pending-action hook(s) for optimistic UX and command feedback
- Alert-ranking utility for deterministic severity display

### 8.3 Theming
- Use `next-themes` and Tailwind tokenized styles
- Ensure parity between light/dark for contrast and semantic colors

### 8.4 Charting
- Keep Recharts as charting standard in current dashboard
- Chart styles must use theme tokens and match card/typography system

---

## 9) Cleanup Rules for Plan and Code

Apply these cleanup rules continuously:
- Remove uncertain language (`maybe`, `probably`, `TBD`) from implementation plan
- Remove legacy architecture notes that no longer represent production dashboard
- Remove unused CSS blocks or pseudo-CSS not mapped to Next.js/Tailwind components
- Remove dead comments and legacy TODO noise
- Keep plan statements testable and implementation-verifiable

---

## 10) Implementation Checklist

### 10.1 UI/Design
- Modern minimalist layout applied consistently across main dashboard
- Spacing, typography, and card primitives standardized
- Danger/warning/info visual semantics consistent and accessible

### 10.2 Icons
- Professional SVG icon set selected and documented
- Icons downloaded from approved source and stored locally in `public/icons`
- No generic placeholder icon usage in production components
- Single icon abstraction used by components

### 10.3 Firmware Compatibility
- Mode, manual, countdown, emergency, reset, clear-error flows verified
- Bypass level + flow behavior verified end to end
- Reboot request flow verified (`reboot_request_id`)
- Device config and notification settings field mappings verified
- Firmware-implemented-but-dashboard-missing items are explicitly tracked and prioritized until closed

### 10.4 Engineering Quality
- Types updated and aligned with actual RTDB payloads
- Component logic avoids duplicated/contradictory state derivation
- Existing tests updated where behavior or contracts changed
- Lint/type checks pass for edited files

---

## 11) Validation Matrix

### 11.1 Mode and Control Validation
- Switch `AUTO` -> `MANUAL` -> `COUNTDOWN`
- Confirm writable fields and resulting firmware `run_mode` transitions

### 11.2 Safety Validation
- Trigger emergency stop and verify latch behavior reflected in UI
- Verify reset-stop behavior with and without active lockout conditions

### 11.3 Sensor and Bypass Validation
- Simulate/observe level and flow sensor faults
- Validate bypass toggles are represented and persisted correctly

### 11.4 Configuration Validation
- Modify representative device config fields and confirm firmware consumption
- Confirm stale/offline handling and status messaging

### 11.5 Notification Validation
- Save per-user notification config and reload session to confirm persistence

### 11.6 Reboot Validation
- Issue reboot request and confirm one-shot behavior + status continuity after boot

---

## 12) Definition of Done

This plan is complete when:
- All statements are consistent with current `dashboard` and `firmware` behavior
- Legacy/uncertain/unused plan content is removed
- UI direction is clearly modern, minimalist, and professional
- Icon policy enforces professional web-sourced SVG assets, locally managed
- Compatibility flows are validated and reproducible using the matrix above

