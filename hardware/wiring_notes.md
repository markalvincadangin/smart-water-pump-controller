# Wiring Notes

Project: SmartFlow  
Electrical system: single-phase 220V AC (Philippines)  
Document scope: deployed main panel + remote tank node architecture

## 1) Electrical Safety and Scope

This document is for technical reference and commissioning quality control.  
Final installation and energization must be performed by a qualified electrician.

Mandatory safety controls:
- Lockout/tagout before touching any terminal.
- Verify de-energized state with a meter (not switch position only).
- Keep protective earth (PE) continuous from source to pump casing.
- Do not bypass TOR in normal operation.

## 2) Architecture Summary (As Deployed)

Main enclosure (near power/pump):
- ESP32 master
- Relay output path to contactor coil
- Main RS-485 transceiver
- 5V local PSU for control electronics

Tank enclosure (near tank):
- NodeMCU V2 sensor node
- JSN-SR04T level sensor
- YF-G1 flow sensor
- RS-485 transceiver
- LM2596 local 5V from distributed 12V

Communications and distributed power over CAT6 (30-40 m):
- RS-485 differential pair A/B
- 12V and GND power pairs

## 3) Firmware Pin Mapping (Source of Truth)

From deployed firmware config files:

ESP32 master:
- GPIO4  -> Relay input control
- GPIO17 -> RS-485 DI (TX)
- GPIO25 <- RS-485 RO (RX)
- GPIO5  -> RS-485 DE/RE (tied)

NodeMCU sensor node:
- D5 / GPIO14 -> RS-485 DE/RE (tied)
- D6 / GPIO12 -> Flow input
- D1 / GPIO5  -> JSN TRIG
- D0 / GPIO16 <- JSN ECHO (through divider)
- UART0 TX/RX (GPIO1/GPIO3) <-> MAX485 DI/RO

Important:
- Flow input is D6/GPIO12 in current deployed firmware.
- If firmware changes, update this document immediately.

## 4) High-Voltage Power Path (220V)

Single-line intent:

```text
Grid 220V -> 2P MCB -> Contactor poles -> TOR power path -> Pump (L/N)
Grid PE -----------------------------------------------> Pump PE
```

Notes:
- Use one pole for line and one pole for neutral switching path according to panel design.
- PE is never switched through contactor or TOR.
- TOR overload setting: align to motor nameplate FLA (typically 8-9A for installed pump).

## 5) Coil Control Path

Control objective: contactor coil energizes only when relay path is enabled and TOR NC path is healthy.

Typical path:

```text
MCB line -> relay COM -> relay NO -> TOR NC 95-96 -> contactor A1
MCB neutral ----------------------------------------> contactor A2
```

Manual bypass (if installed) must be documented and labeled clearly as service/diagnostic use.

## 6) Low-Voltage Power Distribution

Main enclosure:
- 5V adapter powers ESP32, relay module, and main MAX485 module.

Tank enclosure:
- 12V distributed over CAT6 into LM2596.
- LM2596 output set to 5.00V before connecting loads.
- 5V rail powers NodeMCU, sensors, and tank-side MAX485.

Grounding/reference:
- Maintain common reference between transceivers through CAT6 GND path.

## 7) Signal Conditioning

JSN-SR04T ECHO level shift (required):
- 10k series, 20k to GND, then into NodeMCU D0/GPIO16.

YF-G1 signal level check:
- If pulse level exceeds 3.3V at NodeMCU input, add divider (1k series, 2k or 2.2k to GND).

## 8) RS-485 Bus Practices

- Half-duplex mode with DE/RE tied per node.
- 120 ohm termination at both physical ends only.
- Keep A/B on one twisted pair (green pair recommended).
- Keep untwist length short at terminations.
- Verify A/B polarity consistency end-to-end.

## 9) CAT6 Assignment (Recommended)

Main panel to tank node:

| Pair/Conductor | Assignment |
|---|---|
| Green / Green-White | RS-485 A/B |
| Orange / Orange-White | +12V (paralleled) |
| Blue / Blue-White | GND (paralleled) |
| Brown / Brown-White | Spare |

## 10) Pre-Energization Verification

- No L-N short at panel input.
- Torque and tug-check complete on HV terminations.
- PE continuity to pump casing < 1 ohm.
- TOR setpoint verified and seal/mark applied.
- LV rails measured: main 5V and tank 5V stable.
- RS-485 link validated with live telemetry frames.
- Enclosure glands tightened and strain-relieved.
- Safety checklist completed: see DEPLOYMENT_SAFETY.md.

## 11) Common Fault Checks

No RS-485 data:
- Swap A/B at one end if polarity mismatch suspected.
- Verify DE/RE control pin behavior on both nodes.
- Confirm shared reference GND continuity.
- Confirm termination resistors present only at bus ends.

Unstable tank node:
- Recheck LM2596 output under load.
- Improve local decoupling in tank enclosure.
- Reduce parallel routing with noisy AC conductors.

False sensor readings:
- Confirm divider values and placement near NodeMCU.
- Verify JSN probe mounting angle and clear acoustic path.
- Perform bucket test and calibration validation for flow.

Revision: 2026-04-04 (corrected to deployed firmware pinout and production topology)
