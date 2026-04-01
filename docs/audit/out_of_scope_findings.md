# SmartFlow — Out-of-Scope Findings

**Document version:** 1.1  
**Date:** 2026-03-31  
**Rule:** Per Universal Rule §7, these items are documented here and NOT acted on.
Any changes require explicit inclusion in a future refactor plan phase.

---

## OOS-01 [Medium] — Structural Indentation Issue in `03_safety_pump.ino` (MANUAL mode block)

**File:** `firmware/arduino_smart_water_pump_controller/03_safety_pump.ino`  
**Lines:** 292–328 (`executePumpLogic()` — MANUAL mode section)  
**Discovered:** Phase 3 source audit (2026-03-31)

**Observation:**  
The MANUAL mode policy block (`if (pumpMode == "MANUAL") { ... }`) closes at approximately
line 306. Lines 307–327 — which contain the level-sensor error guard, tank-full stop,
minimum off-time check, and the `setPump(true)` call — appear to fall **outside** the
MANUAL block in the compiled brace structure, despite being visually indented as if inside.

If the analysis is correct:
- `setPump(true)` (line 326) is dead code for the MANUAL path (a `return;` at line 305
  exits before it is reached).
- Lines 307–327 would execute for all modes after skipping the MANUAL block, preceding
  the COUNTDOWN block at line 330 — potentially applying MANUAL-specific guards to
  AUTO/COUNTDOWN flows.

**Mitigating factors:**
- The phase completion summary reports the system as "production-ready" and system tests
  have been run; the reported behavior is that MANUAL mode works (pump starts).
- The Arduino multi-tab build concatenates files, and compilation warnings/errors would
  have surfaced a brace mismatch. No compile error was reported.
- Indentation in the `view_file` output uses a `N: content` prefix format that makes
  exact space-counting ambiguous.

**Risk level:** Medium — requires hardware bench verification to confirm actual behavior.

**Action required before next refactor phase:**  
On bench: enter MANUAL mode (`manual_desired=true`), verify relay activates.
If it does, the brace analysis was incorrect (indentation is misleading).
If it does not, this is a Critical bug requiring priority fix.

**Do NOT touch this code block until bench verified.**

---

## OOS-02 [Low] — TC-M-03/M-04/M-05 are Stubs in Test Master Sketch

**File:** `firmware/test_master_node/test_master_node.ino`  
**Discovered:** Phase 5 source audit (2026-03-31)

**Observation:**  
- `test_wifi()` (TC-M-03): Does not actually connect to WiFi. Returns `true` unconditionally
  after printing MAC address. Credential integration via `secrets.h` is not implemented.
- `test_firebase()` (TC-M-04): Returns `true` unconditionally. No actual Firebase
  read/write/delete is attempted.
- `test_integration()` (TC-M-05): Returns `true` unconditionally.

**Impact:** TC-M-03, TC-M-04, TC-M-05 produce false PASS results without exercising
the subsystems they are supposed to test.

**Recommendation for future phase:** Integrate `secrets.h` into the test sketch and
implement the WiFi connect loop, Firebase test node write/read/delete, and RS-485→Firebase
round-trip end-to-end test.

---

## OOS-03 [Low] — `levelLastValidMs` Referenced in Dashboard Status Push Despite M-01 Fix

**File:** `firmware/arduino_smart_water_pump_controller/05_connectivity_cloud.ino`  
**Lines:** 539–544  
**Discovered:** Phase 3 code review (2026-03-31)

**Observation:**  
M-01 consolidated freshness gating onto `levelLastUpdateMs`. However, `levelLastValidMs`
is still referenced for two **dashboard display metrics only** (not safety gates):
- `level_last_valid_age_sec` (line 539)
- `level_sensor_health_pct` computation (line 543)

The shared header comment confirms this: `// dashboard health metric ONLY — NOT a freshness gate; see M-01`.

**Assessment:** This is intentional and documented — not a bug. The metric is cosmetic.
However `levelLastValidMs` is never updated after M-01 changes so `level_last_valid_age_sec`
and `level_sensor_health_pct` may show stale/incorrect values on the dashboard.

**Action:** If dashboard accuracy is required, migrate these two metrics to use
`levelLastUpdateMs` in a future firmware release.

---

## OOS-04 [Low] — `__tests__/` Pre-existing TypeScript Errors

**File:** `dashboard/__tests__/lib/usePendingControl.test.tsx`, `usePumpData.test.tsx`  
**Discovered:** Phase 6 TypeScript check (2026-03-31)

**Observation:**  
`npx tsc --noEmit` reports 2 TS2493 errors in the `__tests__/` directory:
- `usePendingControl.test.tsx(42,16)`: empty array index access
- `usePumpData.test.tsx(112,27)`: tuple type length mismatch

These errors pre-date the Phase 6 type changes. The production source (`app/`, `components/`,
`lib/`) compiles cleanly (verified by Next.js build).

**Action:** Fix test file TypeScript errors in a dedicated test-maintenance sprint.

---

## OOS-05 [Info] — `shimmerSweep` Keyframe Referenced in `tailwind.config.ts` but Not Defined

**File:** `dashboard/tailwind.config.ts` line 68  
**Discovered:** Phase 6 CSS audit  

**Observation:**  
`animation: { shimmer: "shimmerSweep 1.5s linear infinite" }` references a keyframe named
`shimmerSweep`, but the `keyframes` block only defines `fadeIn`, `slideUp`, `glowPulse`,
and `tankWave`. `shimmerSweep` is absent.

**Impact:** `animate-shimmer` utility class will not produce visible animation at runtime.
No build error — Tailwind silently drops undefined keyframe animations.

**Action:** Add `shimmerSweep` keyframe definition to `tailwind.config.ts` in a future
CSS polish sprint.

---

*SmartFlow Out-of-Scope Findings — maintained per Universal Rule §7 — do not act without plan inclusion.*
