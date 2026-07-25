# SmartFlow QA Day 1 — Complete Execution Blueprint
**Date:** 2026-03-31  
**Status:** ✅ All software validation complete | ⏳ Hardware tests pending board connection

---

## 📋 **START HERE: Document Directory**

### 🚀 **For Immediate Hardware Execution**

1. **[QUICK_REFERENCE_CHECKLIST.md](QUICK_REFERENCE_CHECKLIST.md)** ← *One-page quick start*
   - 5-minute overview of board identification, flashing, and test execution
   - Use this if you want to get started NOW

2. **[HARDWARE_FLASH_AND_TEST_GUIDE.md](HARDWARE_FLASH_AND_TEST_GUIDE.md)** ← *Comprehensive step-by-step*
   - Detailed firmware upload procedures for both boards
   - Complete Layer 2 test execution walkthrough (SF-SN-001 to SF-SN-006)
   - Complete Layer 3 test execution walkthrough (SF-RS-001 to SF-RS-006)
   - Troubleshooting section for common issues

3. **[QA_READINESS_REPORT.md](QA_READINESS_REPORT.md)** ← *Executive summary*
   - What's been completed, what's pending, what's required next
   - Compilation & build history; known issues resolved
   - Pass/fail criteria summary

---

### 📖 **Layer-Specific Test Runbooks**

4. **[day-1/layer2_sensor_node_execution.md](day-1/layer2_sensor_node_execution.md)**
   - Detailed instructions for SF-SN-001 through SF-SN-006 tests
   - Per-test prerequisites, setup, evidence requirements
   - Pass criteria and logging rules

5. **[day-1/layer3_rs485_execution.md](day-1/layer3_rs485_execution.md)**
   - Detailed instructions for SF-RS-001 through SF-RS-006 tests
   - RS-485 protocol-specific evidence capture (serial logs, logic analyzer waveforms)
   - CRITICAL test identification (SF-RS-001, SF-RS-002, SF-RS-006 must PASS)
   - Exit gate validation checklist

---

### 📊 **Result Tracking & Evidence**

6. **[day-1/results.csv](day-1/results.csv)**
   - Master test result ledger
   - Current state: Layer 1 hardwmare (SF-HW-001 to SF-HW-006) marked PASS
   - Add Layer 2 & 3 results here as tests complete
   - Columns: Test_ID | Status (PASS/FAIL/SKIP) | Timestamp | Evidence_Path | Notes

7. **[day-1/defects.csv](day-1/defects.csv)**
   - Defect/issue tracking log
   - Add entries only when tests FAIL
   - Columns: Defect_ID | Test_ID | Severity (CRITICAL/MAJOR/MINOR) | Description | Root_Cause | Status

8. **[day-1/rs485_test_capture_template.csv](day-1/rs485_test_capture_template.csv)**
   - Per-test protocol evidence binding for Layer 3
   - Record serial logs, frame captures, timing measurements

---

### 📁 **Evidence Directories**

```
day-1/
├── logs/                           # Serial monitor captures, timing logs, measurements
├── screenshots/                    # Photos of serial output, Firebase state, meters
└── videos/                         # (Optional) Screen recordings of test execution
```

---

### 🔍 **Reference & Infrastructure**

9. **[day-1/baseline_snapshot.md](day-1/baseline_snapshot.md)**
   - Baseline firmware hash and configuration snapshot
   - Layer 1 test results recorded with timestamps
   - Reference point for Layer 2 & 3 validation

10. **[day-1/README.md](day-1/README.md)**
    - QA Day 1 overview
    - Layer progression (1 → 2 → 3)
    - Gate validation checkpoints

11. **[day-1/automated_validation_run.md](day-1/automated_validation_run.md)**
    - Complete log of all autonomous software/firmware tests executed in this session
    - Defect discovery & resolution history (ISR lambda fix documented)
    - Build times, compilation outputs, test pass/fail records

12. **[day-1/wave0_signoff.md](wave0_signoff.md)**
    - Phase 1 Wave 0 readiness gate
    - Sign-off record for initial infrastructure setup

---

## ⚡ **Quick Start Path**

### **If you're ready to test NOW:**
```
1. Read: QUICK_REFERENCE_CHECKLIST.md (5 min)
2. Connect: NodeMCU + ESP32 boards over USB
3. Flash: Follow Phase 2 in HARDWARE_FLASH_AND_TEST_GUIDE.md
4. Execute: Follow Phase 3 & 4 for Layer 2 & Layer 3 tests
5. Record: Update results.csv and defects.csv as tests complete
```

### **If you need detailed background:**
```
1. Read: QA_READINESS_REPORT.md (10 min) — understand session status
2. Read: HARDWARE_FLASH_AND_TEST_GUIDE.md (20 min) — full procedures
3. Read: day-1/layer2_sensor_node_execution.md & layer3_rs485_execution.md for specifics
4. Connect hardware and execute per guides
```

### **If troubleshooting issues:**
```
1. Check: HARDWARE_FLASH_AND_TEST_GUIDE.md § Part 8: Troubleshooting
2. Check: day-1/automated_validation_run.md for prior errors/resolutions
3. Check: results.csv and defects.csv for patterns
```

---

## ✅ **Session Progress Summary**

### Completed
- ✅ Phase 1 Wave 0 readiness scaffolding (checklist, sign-off, templates)
- ✅ Layer 1 hardware tests recorded as PASS (user-confirmed, 2026-03-31)
- ✅ All firmware builds (PlatformIO + Arduino): **5/5 PASS**
- ✅ All dashboard automation (Jest, lint, build): **PASS**
- ✅ Test harness defect (ISR lambda) identified and **FIXED**
- ✅ Layer 2 execution runbook prepared (SF-SN-001 to SF-SN-006)
- ✅ Layer 3 execution runbook prepared (SF-RS-001 to SF-RS-006)
- ✅ Hardware flash & test guides written with step-by-step instructions
- ✅ Results tracking infrastructure initialized (results.csv, defects.csv)
- ✅ Evidence capture directories created (logs/, screenshots/, videos/)

### Pending (Blocked by Missing Hardware)
- ⏳ Layer 2 runtime execution (requires NodeMCU connected)
- ⏳ Layer 3 runtime execution (requires ESP32 + RS-485 setup)
- ⏳ Hardware-dependent test automation (sensor measurements, protocol validation)

### Not Started (Dependent on Layer 2 & 3 Results)
- Phase 7 integration validation
- System-level pump rig testing
- Production readiness sign-off

---

## 🔑 **Key Files at a Glance**

| File | Purpose | Time to Read |
|------|---------|--------------|
| [QUICK_REFERENCE_CHECKLIST.md](QUICK_REFERENCE_CHECKLIST.md) | One-page flash & test checklist | 5 min |
| [HARDWARE_FLASH_AND_TEST_GUIDE.md](HARDWARE_FLASH_AND_TEST_GUIDE.md) | Complete procedure manual | 30 min |
| [QA_READINESS_REPORT.md](QA_READINESS_REPORT.md) | Session status & compilations | 10 min |
| [day-1/layer2_sensor_node_execution.md](day-1/layer2_sensor_node_execution.md) | SF-SN test details (6 cases) | 15 min |
| [day-1/layer3_rs485_execution.md](day-1/layer3_rs485_execution.md) | SF-RS test details (6 cases) | 15 min |
| [day-1/results.csv](day-1/results.csv) | Master result ledger | (populate during testing) |
| [day-1/defects.csv](day-1/defects.csv) | Defect log (fill on FAIL) | (populate during testing) |

---

## 🎯 **Pass/Fail Gate Summary**

### Layer 2 (Sensor Node) Exit Gate
All 6 tests (SF-SN-001 to SF-SN-006) must be executed → PASS/FAIL/SKIP.
- No CRITICAL blockers defined for Layer 2
- All failures must have defect entries in `defects.csv`
- Proceed to Layer 3 only when Layer 2 is fully documented

### Layer 3 (RS-485 Protocol) Exit Gate  
**CRITICAL tests that MUST PASS:**
1. ✅ SF-RS-001: CRC Integrity (valid frames pass, corrupt rejected)
2. ✅ SF-RS-002: Timeout/Retry (offline detected, retry within limits, recovery verified)
3. ✅ SF-RS-006: Bus Direction Timing (no collisions, clean transitions)

**Plus** all other tests (SF-RS-003, SF-RS-004, SF-RS-005) must be PASS/FAIL/SKIP with evidence.

---

## 📞 **Support**

**For questions during hardware execution:**
1. Check the **Troubleshooting** section in HARDWARE_FLASH_AND_TEST_GUIDE.md
2. Review prior defect resolutions in day-1/automated_validation_run.md
3. Cross-reference with appropriate layer runbook (layer2 or layer3)

**For defects found during testing:**
1. Record in day-1/results.csv with FAIL status
2. Create entry in day-1/defects.csv with Severity (CRITICAL/MAJOR/MINOR)
3. If CRITICAL and blocking, stop further testing and escalate per project procedures

---

## 🚀 **Next Command**

When hardware is ready:

```bash
# 1. Check board detection
arduino-cli board list

# 2. Note COM ports and follow QUICK_REFERENCE_CHECKLIST.md Phase 2: Flash Test Firmware

# 3. Start with Layer 2: SF-SN-001 first test
```

---

**Generated:** 2026-03-31  
**Status:** Day 1 Software Validation Complete — awaiting hardware connection for Layer 2 & 3 runtime tests

---

*For detailed execution procedures, refer to the specific guides above. For background on session decisions and resolved issues, see [QA_READINESS_REPORT.md](QA_READINESS_REPORT.md).*
