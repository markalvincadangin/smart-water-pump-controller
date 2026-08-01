# Enclosure Layout

Project: SmartFlow  
Main enclosure: IP65 ABS, 300 x 400 x 200 mm  
Orientation: Portrait (300 mm wide x 400 mm tall)

## 1) Safety Zoning

Use strict physical zoning to reduce electrical risk and EMI coupling.

- Zone A (top): High voltage (220V AC only)
- Zone B (middle): Transition/distribution (limited interface between HV and LV)
- Zone C (bottom): Low voltage logic (5V/3.3V and RS-485)

Rule:
- Never run 220V conductors through Zone C.
- Maintain minimum 25 mm separation between HV and LV runs when parallel.

## 2) Recommended Physical Arrangement

```text
Top (Zone A, HV)
  [MCB 2P 20A] [Contactor CJX2 + TOR LR2-D13] [PE lug]

Middle (Zone B, transition)
  [Neutral terminal block / bus] [5V adapter]

Bottom (Zone C, LV)
  [ESP32 DevKit] [Relay module] [MAX485 main transceiver] [Smart Reset Button]

Bottom wall glands
  [PG16] Grid in   [PG16] Pump out   [PG9] CAT6 to tank box
```

## 3) Main Box Wiring Boundaries

Zone A:
- MCB, contactor, TOR, and all pump-path conductors.
- Coil trigger path components may terminate near this zone but LV control wires must stay isolated.

Zone B:
- Neutral distribution and AC input to the 5V adapter.
- Keep branch terminations labeled; no double-landing two conductors under one terminal unless terminal is rated for it.

Zone C:
- ESP32 + relay module + MAX485.
- Short signal routes only.
- CAT6 terminations for A/B and distributed DC reference/power.

## 4) Earth and Bonding

- PE from grid input bonds to the enclosure PE point.
- Pump cable PE bonds directly from PE point to pump casing conductor.
- PE is not routed through contactor/TOR switching poles.

## 5) Tank-Side Mini Enclosure Layout

Recommended order inside tank box:

```text
[CAT6 terminal block] -> [LM2596 buck] -> [NodeMCU + MAX485] -> [sensor terminations]
```

- Keep JSN and flow signal wires short.
- Install divider networks in the tank box near NodeMCU input pins.
- Add 100nF local decoupling near NodeMCU and MAX485; add bulk capacitor on 5V rail.

## 6) Assembly Sequence

1. Install rail, glands, and mechanical hardware.
2. Mount Zone A devices (MCB/contact/TOR/PE).
3. Mount Zone B adapter and terminals.
4. Mount Zone C logic devices.
5. Complete HV terminations and torque-check.
6. Complete LV and RS-485 wiring.
7. Verify PE continuity and insulation checks.
8. Power-up LV first, then commission HV after checklist sign-off.

## 7) Inspection Checklist (Mechanical + Routing)

- HV and LV physically separated and tie-managed.
- All gland seals tightened and strain relief effective.
- No exposed conductor at terminals.
- Terminal labels present and legible.
- Enclosure gasket clean, seated, and undamaged before closure.

Revision: 2026-04-04
