# Mode Architecture v5 — Implementation Plan

## Goal

Refine the v4 mode architecture for clarity: **AUTO** (autonomous), **MANUAL** (operator ON/OFF with full safety), **COUNTDOWN** (semi-auto timed run, action-first), **FORCE_OFF / FORCE_ON** (true overrides). Errors in normal modes trigger a latched lockout. Dashboard UX redesigned for cognitive efficiency.

---

## Design Decisions (Resolved)

| Decision | Resolution | Backing |
|----------|-----------|---------|
| MANUAL sticky (pump off ≠ exit mode) | ✅ Approved by user | IEC 61511 mode/demand separation |
| Quick Start removed | ✅ Approved by user | Hick's Law — fewer choices |
| COUNTDOWN interaction | **Action-first** (not mode-first) | ISA-101: "presents immediate control options relevant to the current situation, rather than requiring an operator to first select a mode which might add an unnecessary step." Reduces cognitive load. Firmware already auto-sets `pumpMode="COUNTDOWN"` on start. |

---

## Research Backing

| Principle | Standard | Application |
|-----------|----------|-------------|
| Safety lockout = explicit ack | **IEC 61511 §11.6**, **NFPA 79 §9.2** | P1 errors latch pump OFF. `clear_error` clears flag only |
| State-based alarming | **IEC 62682**, **ISA-18.2** | Dashboard alerts adapt per mode |
| Action-first for timed ops | **ISA-101** (ANSI/ISA-101.01-2015) | "Action first" reduces steps and cognitive burden for direct operations |
| Binary toggle for ON/OFF | **Hick's Law** (cognitive psych) | Toggles optimal for binary immediate-effect states |
| Progressive disclosure | **ISA-101**, **HFE best practice** | Overrides hidden in collapsible; timer appears contextually |
| Motor restart delay | **IEC 60204-1 §7.2**, **NFPA 79** | R-01 `MIN_PUMP_OFF_TIME_MS = 30s` — already implemented |

---

## Dashboard UX Layout (v5)

### Controls Card — Redesigned

```
┌─────────────────────────────────────────┐
│                                         │
│ Mode ─────────────────────────────────  │
│ ┌──────────────────┬──────────────────┐ │
│ │      AUTO        │     MANUAL       │ │
│ └──────────────────┴──────────────────┘ │
│                                         │
│ Run Controls ─────────────────────────  │
│ (context-sensitive per mode)            │
│                                         │
│  AUTO: "Pump runs automatically ..."    │
│  MANUAL: [ ■ ON ] / [ □ OFF ] toggle   │
│  COUNTDOWN: Timer | [Start] | [Stop]   │
│  Error: [Clear Error] inline card       │
│                                         │
│ Semi-Auto Timer ──────────────────────  │
│ [5] [10] [15] [30] [Custom] → [Start]  │
│ (starts COUNTDOWN mode on press)        │
│                                         │
│ ▸ Emergency Controls ────────────────   │
│   [FORCE OFF]     [FORCE ON ⚡]        │
│   (toggle)        (2-step confirm)      │
└─────────────────────────────────────────┘
```

**Key changes from v4:**
- Mode pill: 2 segments (AUTO | MANUAL) instead of Normal + Emergency sections
- COUNTDOWN: action-first timer in RunControls (not a mode button)
- MANUAL: ON/OFF toggle replaces Quick Start + Stop
- Emergency controls: collapsible accordion (progressive disclosure)
- Error lockout: inline card inside controls (ISA-101 proximity principle)

---

## Proposed Changes

### Firmware (3 changes)

---

#### [MODIFY] [03_safety_pump.ino](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/arduino_smart_water_pump_controller/03_safety_pump.ino)

**F-01:** P3 tank-full — stay in MANUAL (remove revert to AUTO):
```diff
-      pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
+      // v5: stay in MANUAL. Operator exits explicitly.
```

**F-03:** runMode derivation — `MANUAL_OFF` when MANUAL + pump off:
```diff
-    runMode = "OFF";       // safety stopped; pump off while MANUAL mode holds
+    runMode = "MANUAL_OFF"; // v5: MANUAL mode, pump off
```

---

#### [MODIFY] [05_connectivity_cloud.ino](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/arduino_smart_water_pump_controller/05_connectivity_cloud.ino)

**F-02:** `manual_stop` — keep MANUAL mode (don't revert to AUTO):
```diff
-        pumpMode = "AUTO";
-        pendingModeWriteback = true;
-        ...
-        Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
+        // v5: stay in MANUAL (OFF position). Don't revert to AUTO.
```

---

#### [MODIFY] [shared.h](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/arduino_smart_water_pump_controller/smart_water_pump_controller_shared.h)

Update `runMode` comment to include `"MANUAL_OFF"`.

---

### Dashboard (6 component changes)

---

#### [MODIFY] [types.ts](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/lib/types.ts)

Add `"MANUAL_OFF"` to `PumpStatus.run_mode` union.

---

#### [MODIFY] [ModeControls.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/ModeControls.tsx)

Full rewrite: 2-segment pill (AUTO | MANUAL) + collapsible Emergency Controls (FORCE_OFF, FORCE_ON with 2-step confirm). COUNTDOWN removed from mode selector.

---

#### [MODIFY] [RunControls.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/RunControls.tsx)

Full rewrite — context-sensitive controls:
- **AUTO:** Informational "Pump operates automatically"
- **MANUAL:** Large ON/OFF toggle. ON = `startManualRun()`, OFF = `stopRun()`
- **COUNTDOWN (active):** Live timer + Stop + Add time
- **Idle (AUTO/MANUAL_OFF):** Semi-Auto Timer section with presets (5/10/15/30 min) + custom + Start
- **Error lockout:** Inline Clear Error card
- **FORCE_OFF:** "Pump locked out" info
- **FORCE_ON:** Override warning

---

#### [MODIFY] [StatusBar.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/StatusBar.tsx)

Handle `MANUAL_OFF` badge: "MANUAL" with neutral/green styling (mode still active, pump off).

---

#### [MODIFY] [DashboardMainGrid.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/DashboardMainGrid.tsx)

Update [RunMode](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/RunControls.tsx#8-9) cast to include `"MANUAL_OFF"`. Stat card sub-text: show "MANUAL · Off" for MANUAL_OFF.

---

#### [MODIFY] [usePumpData.ts](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/lib/usePumpData.ts)

`stopRun()`: when in MANUAL mode, don't write `mode=AUTO` (align with firmware F-02).

---

#### Sync

Copy modified firmware files to `firmware/platformio_smart_water_pump_controller/src/`.

---

## Verification Plan

### Automated
- `npm run build` — TypeScript compilation
- `npx jest` — existing tests pass
- PlatformIO `pio run` — firmware compilation (if available)

### Manual Hardware Tests
1. **MN-01:** MANUAL → ON → OFF → stays in MANUAL
2. **MN-02:** MANUAL ON → tank full → pump stops → stays in MANUAL
3. **MN-03:** MANUAL ON → dry-run → lockout → Clear Error → stays in MANUAL (pump off)
4. **CD-01:** Start countdown 5 min → runs → timer expires → reverts to AUTO
5. **FN-01:** FORCE_ON via 2-step → pump runs → exit to AUTO
6. **FF-01:** FORCE_OFF → all runs blocked → AUTO resumes
