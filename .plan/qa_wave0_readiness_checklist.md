# SmartFlow QA Wave 0 Readiness Checklist

Date: 2026-03-31  
Phase: Wave 0 (Readiness and Dry Run)  
Reference: .plan/smartflow_test_implementation_plan.md

## A. Team and Ownership
- [ ] QA Lead assigned
- [ ] Firmware QA Engineer assigned
- [ ] Dashboard QA Engineer assigned
- [ ] Cloud/Backend QA Engineer assigned
- [ ] Safety Observer (qualified electrical) assigned for [SAFETY] tests
- [ ] Release Manager assigned

## B. Safety Preconditions (Mandatory)
- [ ] MCB is accessible during all energized tests
- [ ] TOR is present and not bypassed
- [ ] Manual bypass switch warning acknowledged by team
- [ ] Emergency shutdown path and roles briefed
- [ ] Work area cleared and dry; no exposed unsafe wiring

## C. Hardware and Lab Readiness
- [ ] ESP32 master device available and powered
- [ ] NodeMCU sensor node available and powered
- [ ] RS-485 transceivers and CAT6 link verified
- [ ] Relay/contactor/TOR wiring verified by qualified person
- [ ] Pump and water supply available for energized tests
- [ ] Multimeter and logic analyzer functional
- [ ] USB-TTL adapter connected for NodeMCU debug

## D. Software and Environment Readiness
- [ ] ESP32 firmware build prepared and flashed
- [ ] NodeMCU firmware build prepared and flashed
- [ ] Dashboard build deployed or running locally
- [ ] Firebase RTDB reachable
- [ ] Firebase rules deployed and validated
- [ ] `secrets.h` present on test devices (not in git)

## E. Traceability and Evidence Readiness
- [ ] Day 1 evidence directory created under `docs/audit/qa/2026-03-31/day-1/`
- [ ] `results.csv` ready with Wave 1 test IDs
- [ ] Naming convention understood by all testers
- [ ] Defect tracker project/board ready
- [ ] Build hash capture procedure prepared

## F. Dry Run of Procedure
- [ ] Confirm serial logs can be captured from both nodes
- [ ] Confirm Firebase snapshots can be captured
- [ ] Confirm dashboard screenshots can be captured
- [ ] Execute one rehearsal test record (mock) end to end

## Readiness Gate Decision
- [ ] READY to start Wave 1
- [ ] NOT READY (blockers documented)

## Blockers (if any)
- 1.
- 2.
- 3.

## Sign-Off
- QA Lead: ____________________  Date/Time: ____________________
- Safety Observer: _____________ Date/Time: ____________________
- Release Manager: _____________ Date/Time: ____________________
