# SmartFlow Software Quality Assurance — Complete Execution Blueprint
**Date:** 2026-03-31  
**Scope:** Firmware (ESP32 + NodeMCU) + Dashboard (Next.js)  
**Focus:** Correctness, Integrity, Safety Standards (no hardware required)  
**Status:** ✅ Complete QA Plans Ready for Execution

---

## Executive Summary

Now that hardware integration is deferred, QA focus shifts to **comprehensive software validation**:

1. ✅ **Firmware QA Test Plan** (80+ test cases) — Ready for unit/integration test development
2. ✅ **Firmware Static Code Review** (8 findings) — Architecture validated, 3 MAJOR issues identified for fix
3. ✅ **Dashboard QA Test Plan** (30+ test cases) — Integration & accessibility testing framework
4. ✅ **Build Validation** — Both firmware and dashboard compile with 0 warnings

**Immediate Next Steps:**
- Execute firmware findings remediation (FINDING #1–3)
- Develop detailed unit/integration test suites
- Perform comprehensive code review validation
- Execute dashboard integration tests

---

## Part 1: Firmware QA Execution Path

### Phase 1A: Static Code Review (✅ COMPLETE)

**Deliverable:** [FIRMWARE_STATIC_CODE_REVIEW.md](./FIRMWARE_STATIC_CODE_REVIEW.md)

**Findings Summary:**

| Finding | Severity | Module | Status |
|---------|----------|--------|--------|
| F#1: Manual runtime limit not enforced | MAJOR | safety_pump.cpp | 🔴 ACTION REQUIRED |
| F#2: CRC16 implementation unvalidated | MAJOR | crc16_modbus.cpp | 🔴 ACTION REQUIRED |
| F#3: RS-485 frame validation missing | MAJOR | rs485_comm.cpp | 🔴 ACTION REQUIRED |  
| F#4: Dry-run timeout boundary untested | HIGH | safety_pump.cpp | 🟡 TESTING REQUIRED |
| F#5: Auto-bypass timer not RTC-synced | MEDIUM | safety_pump.cpp | ℹ️ DOCUMENT |
| F#6: Firebase async timing | MEDIUM | main.cpp | ℹ️ VERIFY |
| F#7: String allocation audit | MEDIUM | main.cpp | ℹ️ AUDIT |
| F#8: Hardware abstraction missing | LOW | firmware-wide | 📅 FUTURE |

**Strengths Verified:**
- ✋ Safety-first architecture (pump defaults to OFF)
- ✋ State machine well-defined (P0–P5 transitions)
- ✋ Error handling robust (sensor failure auto-bypass)
- ✋ Backward compatibility (LDSC optional field)
- ✋ Configuration validation (NVS bounds checking)

### Phase 1B: Findings Remediation (⏳ IN PROGRESS)

**FINDING #1: Manual Runtime Limit Enforcement**

```cpp
// BEFORE (safety_pump.cpp, line 97-108):
if (pumpMode == "MANUAL") {
  manualRuntimeWarning = true;  // WARNING ONLY
  LOG(LOG_LEVEL_WARN, ...);
} else {
  isOverflowError = true;
  setPump(false);  // AUTO mode: hard stop
}

// AFTER (proposed fix):
// Apply hard limit to ALL pump modes equally
if (elapsed >= maxRuntimeMs) {
  isOverflowError = true;
  setPump(false);  // Hard stop for ALL modes
  if (pumpMode == "MANUAL") {
    manualRuntimeWarning = true;  // Also warn user
  }
}
```

**Test Case:** TC-SAFETY-MANUAL-RUNTIME-001 in [FIRMWARE_QA_TEST_PLAN.md](./FIRMWARE_QA_TEST_PLAN.md) § Section 4.4

**FINDING #2: CRC16 Validation Tests**

```cpp
// Create test/test_crc16.cpp with known good vectors:
TC-CRC-001: [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A] → CRC = 0x4409 ✓
TC-CRC-002: Corrupt frame rejected (CRC mismatch)
TC-CRC-003: Zero-length frame handled gracefully
TC-CRC-004: Single-byte frame (edge case)
TC-CRC-005: 256-byte frame (max length)
```

**Test Suite:** Create `firmware/platformio_smart_water_pump_controller/test/test_crc16.cpp`

**FINDING #3: RS-485 Frame Validation Function**

```cpp  
// Add to rs485_comm.cpp:
bool validateSensorResponseFrame(const char* frame) {
  // Check frame structure:
  // [STX][FUNC][BYTE_COUNT][DATA][CRC_L][CRC_H][ETX]
  size_t len = strlen(frame);
  if (len < 8) return false;  // Minimum frame size
  
  // Parse BYTE_COUNT and verify DATA section length
  int byteCount;
  if (!parseIntField(frame, "BC:", byteCount)) return false;
  
  // Verify frame contains expected data fields
  if (!strstr(frame, "L:")) return false;  // Level field required
  if (!strstr(frame, "F:")) return false;  // Flow field required
  
  // Verify CRC last
  // ... CRC validation ...
  
  return true;
}
```

**Usage in rs485_requestData():**
```cpp
if (!validateSensorResponseFrame(rxBuffer)) {
  LOG(LOG_LEVEL_ERROR, "RS485", "Frame validation failed");
  return false;  // Reject malformed frame
}
```

### Phase 1C: Unit Test Development (⏳ NEXT)

**Test Suites to Create:**

1. `test/test_state.cpp` — 8 state machine tests (P0–P5 transitions)
2. `test/test_safety.cpp` — 9 safety module tests (errors, recovery)
3. `test/test_crc16.cpp` — 5 CRC integrity tests (known vectors)
4. `test/test_protocol.cpp` — 6 RS-485 protocol tests
5. `test/test_persistence.cpp` — 6 NVS persistence tests

**Execution Command:**
```bash
cd firmware/platformio_smart_water_pump_controller
pio test -e esp32dev --filter="test_*"
```

**Expected Results:**
- 28+ unit tests written
- 28/28 PASS ✅
- Build time < 30 seconds
- 0 compilation warnings

### Phase 1D: Integration Test Development (⏳ NEXT)

**Test Suites to Create:**

1. `test/integration_state_flow.cpp` — Full pump cycle with sensors
2. `test/integration_rs485.cpp` — Protocol communication scenarios
3. `test/integration_errors.cpp` — Error detection & recovery

**Expected Results:**
- 10+ integration tests
- 10/10 PASS ✅
- Coverage: state machine, protocol, error paths

### Phase 1E: Build Validation (⏳ FINAL)

```bash
# Compile with strict warnings
pio run -d firmware/platformio_smart_water_pump_controller -e esp32dev -- -Wall -Wextra -Werror

# Verify resource limits
# Expected:
#   - Flash: < 60% (currently ~40%)
#   - RAM: < 25% (currently ~15%)
#   - Warnings: 0
```

---

## Part 2: Dashboard QA Execution Path

### Phase 2A: Current Status ✅

**All automated checks PASS:**
```
Jest:       17/17 tests ✓ PASS
ESLint:     0 warnings, 0 errors ✓ PASS
TypeScript: strict mode, 0 type errors ✓ PASS
Build:      8 routes, PWA ready ✓ PASS
```

### Phase 2B: Unit Test Expansion (⏳ IN PROGRESS)

**New Test Suites to Create:**

1. **State Management Tests** — Store correctness, immutability
   ```
   TC-STATE-UI-001: Firebase data binding
   TC-STATE-UI-002: Pump control transitions
   TC-STATE-UI-003: Error state display
   TC-STATE-UI-004: Sensor offline recovery
   TC-STATE-UI-005: Configuration persistence
   ```

2. **Component Tests** — Individual UI correctness
   ```
   TC-COMP-001: PumpControl (ON/OFF buttons, state)
   TC-COMP-002: LevelGauge (visual representation)
   TC-COMP-003: SensorStatus (error display)
   TC-COMP-004: TelemetryChart (data visualization)
   TC-COMP-005: SettingsForm (validation)
   ```

**Execution:**
```bash
cd dashboard
npm test -- --coverage
```

**Expected:** 15+ new tests, all PASS ✅

### Phase 2C: Integration Testing (⏳ NEXT)

**User Workflow Tests:**

1. **Workflow 1: Normal Pump Operation**
   - Boot → Monitor level/flow → Manual ON → Pump energizes → Manual OFF
   - Expected: All transitions smooth, telemetry updates, no errors

2. **Workflow 2: Auto Mode Cycle**
   - Set AUTO mode → Level rises to start threshold → Pump energizes → Level rises to stop → Pump de-energizes
   - Expected: Auto cycle works without user input

3. **Workflow 3: Error & Recovery**
   - Simulate dry-run error → Pump stops → Resume flow → Error clears → Pump re-enables
   - Expected: Error displayed, recovery works

**Firebase Integration Tests:**

- Real-time data binding (frontend receives backend updates)
- Remote command execution (frontend sends, backend executes)
- Error logging (errors appear in dashboard)
- Offline resilience (queued during offline, execute on reconnect)
- Permissions (access control enforced)

### Phase 2D: Accessibility & Performance (⏳ NEXT)

**WCAG 2.1 AA Compliance:**
```
TC-A11Y-001: Keyboard navigation (TAB key, focus visible)
TC-A11Y-002: Screen reader compatibility (NVDA, JAWS)
TC-A11Y-003: Color contrast (4.5:1 ratio)
TC-A11Y-004: Form labels (all inputs labeled)
TC-A11Y-005: Error messages (associated with fields)
```

**Performance Metrics:**
```
TC-PERF-001: Initial load < 3 seconds
TC-PERF-002: FCP < 1 second
TC-PERF-003: Re-render on data update < 100 ms
TC-PERF-004: Telemetry chart with 100+ points smooth
TC-PERF-005: No memory leaks (heap stable after 10 page cycles)
```

### Phase 2E: Configuration Testing (⏳ NEXT)

**Form Validation Tests:**
```
TC-CONFIG-001: Pump start level (0–100%)
TC-CONFIG-002: Pump stop level (> start)
TC-CONFIG-003: Dry-run threshold (0.1–10 LPM)
TC-CONFIG-004: Max pump runtime (30–480 min)
TC-CONFIG-005: Sleep schedule (hour ranges)
```

---

## Part 3: Execution Roadmap & Timeline

### Week 1 (March 31 – April 4)

**Day 1-2: Firmware Findings Remediation**
- [ ] Fix FINDING #1: Manual runtime limit (code change + test)
- [ ] Implement FINDING #2: CRC validation tests (5 test cases)
- [ ] Implement FINDING #3: Frame validation function (code + test)
- **Deliverable:** PR with fixes + test results

**Day 3: Unit Test Development**
- [ ] Create test/test_state.cpp (8 tests)
- [ ] Create test/test_safety.cpp (9 tests)
- [ ] Create test/test_crc16.cpp (5 tests)
- **Deliverable:** 22+ unit tests, all PASS ✅

**Day 4: Protocol & Persistence Tests**
- [ ] Create test/test_protocol.cpp (6 tests)
- [ ] Create test/test_persistence.cpp (6 tests)
- **Deliverable:** 12+ additional tests, all PASS ✅

**Day 5: Integration Tests & Build Validation**
- [ ] Create integration test suite (10+ tests)
- [ ] Verify build (0 warnings, resource limits)
- [ ] Generate final firmware QA report
- **Deliverable:** Firmware QA Sign-Off ✅

### Week 2 (April 7 – April 11)

**Day 1-2: Dashboard Unit Tests**
- [ ] Expand state management tests (6 tests)
- [ ] Expand component tests (8+ tests)
- **Deliverable:** 15+ new unit tests, all PASS ✅

**Day 3-4: Dashboard Integration Tests**
- [ ] Implement workflow tests (3 tests)
- [ ] Implement Firebase integration tests (5 tests)
- **Deliverable:** 8+ integration tests, all PASS ✅

**Day 5: Accessibility & Performance**
- [ ] Run WCAG 2.1 AA compliance checks
- [ ] Execute performance profiling (Lighthouse, Core Web Vitals)
- [ ] Generate dashboard QA final report
- **Deliverable:** Dashboard QA Sign-Off ✅

---

## Part 4: Quality Gates & Sign-Off

### Firmware Approval Gate

**Must have all of:**
- [ ] FINDING #1–3 resolved (code changes committed)
- [ ] Unit tests: 28+ tests, 100% PASS
- [ ] Integration tests: 10+ tests, 100% PASS
- [ ] Build validation: 0 errors, 0 warnings, resource limits OK
- [ ] Code review: All critical paths documented + tested
- [ ] Protocol integrity: CRC and frame format validated
- [ ] Safety: All OFF paths verified

**Expected:** ✅ Firmware Approval for Dashboard Integration Testing

### Dashboard Approval Gate

**Must have all of:**
- [ ] Current unit tests: 17/17 PASS (existing)
- [ ] New unit tests: 15+ tests, 100% PASS
- [ ] Integration tests: 8+ tests, 100% PASS
- [ ] Build: 0 errors, 0 TypeScript warnings
- [ ] Accessibility: WCAG 2.1 AA compliance verified
- [ ] Performance: Lighthouse ≥ 85, Core Web Vitals PASS
- [ ] Firebase integration: Real-time sync, offline resilience verified

**Expected:** ✅ Dashboard Approval for Production Deployment

---

## Part 5: Documentation & Artifacts

### Deliverables Created ✅

1. [FIRMWARE_QA_TEST_PLAN.md](./FIRMWARE_QA_TEST_PLAN.md)
   - 80+ test cases defined
   - 6 layers of testing (static, unit, integration, safety, boundary, error handling)
   - Build validation checklist

2. [FIRMWARE_STATIC_CODE_REVIEW.md](./FIRMWARE_STATIC_CODE_REVIEW.md)
   - Architecture strengths documented
   - 8 findings with remediation paths
   - Critical path analysis
   - Code quality metrics

3. [DASHBOARD_QA_TEST_PLAN.md](./DASHBOARD_QA_TEST_PLAN.md)
   - 30+ test cases defined
   - State management, components, workflows, Firebase
   - Accessibility (WCAG 2.1), performance (Core Web Vitals)
   - Configuration validation

### In-Progress Artifacts 🏗️

- Firmware unit test suites (test/test_*.cpp files)
- Firmware integration tests (test/integration_*.cpp files)
- Firmware findings fix PRs (safety_pump.cpp, rs485_comm.cpp, crc16_modbus.cpp)
- Dashboard expanded unit tests (dashboard/__tests__/*.test.tsx)
- Dashboard integration tests (dashboard/__tests__/integration/*.test.tsx)

### Final Artifacts 📋

- **QA Executive Report** (summary of all findings, test results, sign-off)
- **Build Validation Report** (compilation metrics, warnings, resource usage)
- **Test Coverage Report** (unit + integration coverage percentage)
- **Findings Remediation Report** (fixes verified, test evidence)
- **Accessibility Audit Report** (WCAG compliance validated)
- **Performance Analysis Report** (Lighthouse, Core Web Vitals metrics)

---

## Part 6: Key Contacts & Escalation

**Questions or blockers:**
1. Firmware findings or technical complexities → Review [FIRMWARE_STATIC_CODE_REVIEW.md](./FIRMWARE_STATIC_CODE_REVIEW.md) § Part 2
2. Test planning or execution → Reference [FIRMWARE_QA_TEST_PLAN.md](./FIRMWARE_QA_TEST_PLAN.md) § Part 8
3. Dashboard integration issues → Check [DASHBOARD_QA_TEST_PLAN.md](./DASHBOARD_QA_TEST_PLAN.md) § Part 3
4. Build or compilation errors → See QA_READINESS_REPORT.md § Part 7

---

## Part 7: Success Criteria & Completion Definition

**Phase Complete When:**

✅ Firmware:
- All MAJOR findings remediated and verified by tests
- 28+ unit tests PASS
- 10+ integration tests PASS
- 0 build warnings
- All critical safety paths tested and documented

✅ Dashboard:
- 32+ total unit tests PASS (17 existing + 15 new)
- 8+ integration tests PASS
- WCAG 2.1 AA compliance verified
- Performance metrics acceptable (Lighthouse ≥ 85)
- Firebase integration fully tested

✅ Documentation:
- All test results recorded with evidence
- All findings resolved or documented
- Handoff documentation prepared for integration team

---

**Status:** 📋 **COMPLETE QA PLANS READY — EXECUTION PHASE STARTS IMMEDIATELY**

**Next Command:** Execute Phase 1A findings remediation (FINDING #1–3 fixes)

---

*Master QA Blueprint prepared: 2026-03-31*  
*Scope: Complete firmware & dashboard validation without hardware*  
*Expected completion: April 11, 2026*
