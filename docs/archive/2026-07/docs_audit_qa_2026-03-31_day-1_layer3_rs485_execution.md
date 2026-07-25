# Layer 3 Execution Sheet - RS-485 Protocol (SF-RS)

Date: 2026-03-31
Scope: SF-RS-001 to SF-RS-006
Dependency: Execute after Layer 2 SF-SN tests are complete and stable

## Goal
Validate RS-485 protocol integrity, timeout/retry behavior, parser compatibility, and bus-direction timing with strict evidence capture.

## Required Setup Before Start
- ESP32 master and NodeMCU sensor node on baseline firmware hash 881195f
- CAT6 and RS-485 transceivers connected with shared ground
- USB serial capture active for both nodes
- Logic analyzer ready on RS-485 A/B for waveform/timing tests
- Tracker and defects log open

## Execution Order
1. SF-RS-001 Frame CRC integrity
2. SF-RS-002 Timeout and retry behavior
3. SF-RS-003 Partial frame stall recovery
4. SF-RS-004 Sequence monotonicity and wrap
5. SF-RS-005 LDSC backward compatibility
6. SF-RS-006 Bus direction control timing

## Evidence Requirements by Test
- SF-RS-001: serial excerpt proving valid vs rejected CRC frames
- SF-RS-002: retry count evidence + offline/online transition timestamps
- SF-RS-003: stall reset log plus next valid frame response evidence
- SF-RS-004: sequence trace across 260 frames including 255 -> 0 wrap
- SF-RS-005: evidence with LDSC present and absent frame variants
- SF-RS-006: logic analyzer capture proving DE/RE hold and no contention

## Pass Criteria Summary
- CRITICAL tests SF-RS-001, SF-RS-002, SF-RS-006 must pass before advancing
- No parser crashes or corrupt state updates from malformed/legacy frames
- Retry and offline detection timings align with plan limits

## Logging Rules
- Each completed test row in results.csv must include evidence path and short note
- Any FAIL immediately gets a defect_id and defects.csv entry
- Do not mark PASS without direct artifact evidence

## Exit Gate
Proceed beyond Layer 3 only if:
- All SF-RS tests are PASS/FAIL/SKIP with evidence
- No open CRITICAL protocol defects
- Parser compatibility behavior documented for LDSC optional mode
