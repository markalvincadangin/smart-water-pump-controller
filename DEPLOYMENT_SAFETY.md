# SmartFlow Deployment Safety Checklist & Operational Runbook

SmartFlow controls a 220V AC mains-powered industrial inductive load (1.5 HP deep-well pump motor). This checklist summarizes mandatory hardware verification, pre-energization electrical checks, and operational commissioning protocols.

> **CRITICAL WARNING:** This checklist is not a substitute for national electrical codes, qualified professional inspection, or component manufacturer specifications. Never energize high-voltage mains wiring solely to validate software logic.

---

## 1. High-Voltage Electrical Isolation & Safety

Before opening the electrical enclosure or connecting 220V AC power:

- [ ] **Mains Lockout / Tagout**: Switch off the upstream branch circuit breaker and verify complete absence of voltage across all terminals using a calibrated multimeter or voltage tester.
- [ ] **Continuous Protective Earth**: Verify that the DIN-rail ground terminal, electrical enclosure chassis, and pump motor casing have an unbroken, low-impedance ground path (< 1.0 Ω).
- [ ] **Segregation of Circuits**: Confirm high-voltage AC mains lines (220V power, contactor coil, and motor leads) are physically segregated and routed separately from low-voltage DC signals (3.3V/5V logic, sensors, and RS-485 bus).
- [ ] **Overload Relay Calibration**: Confirm the **LR2-D13 Thermal Overload Relay** dial is manually dialed to match the pump motor nameplate Full Load Amperage (FLA, typically 8.0–9.0 A). Do not rely on software to protect against motor thermal overload.
- [ ] **Physical Enclosure Protection**: Confirm IP65 enclosure seals, PG cable glands, and transparent terminal covers are correctly installed to prevent dust, moisture, and insect ingress.

---

## 2. Low-Voltage DC & Communications Pre-Flight

Before seating microcontrollers into their terminal sockets:

- [ ] **Power Supply Regulation**: Measure DC buck converter output under no-load; verify it provides a clean, stable 5.0V ± 0.1V rail to both controllers.
- [ ] **Logic Level Shifting**: Verify that the ultrasonic sensor ECHO pin and any 5V sensor outputs pass through voltage dividers or level shifters before reaching the 3.3V ESP32/ESP8266 GPIO pins.
- [ ] **RS-485 Bus Integrity**:
  - Verify A (+) and B (-) polarity between the master node and tank node.
  - Verify 120 Ω bus termination resistors are fitted at both ends of the 40-meter CAT6 run.
  - Verify common signal ground reference across nodes to prevent ground-loop common-mode voltage breakdown.
- [ ] **Contactor Coil Snubber**: Verify that an RC snubber or varistor is installed across the CJX2 contactor coil to suppress inductive flyback voltage spikes when the relay switches off.

---

## 3. Isolated Software & Logic Validation

With the 220V motor branch circuit isolated and unenergized:

1. **Android Application Test Suite**:
   ```powershell
   ./gradlew.bat test
   ```
2. **Cloud Functions Test Suite**:
   ```bash
   cd functions && npm test
   ```
3. **Firmware Targets Compilation**:
   ```bash
   pio run -d firmware/master_node
   pio run -d firmware/sensor_node
   ```
4. **Relay Startup State**: Power up the ESP32 master node; confirm the pump relay remains **normally OPEN (de-energized)** during bootloader execution and Wi-Fi connection.
5. **Sensor Watchdog Validation**: Disconnect the RS-485 link; confirm that stale or missing telemetry blocks all automatic pump starts and causes a communication fault warning.

---

## 4. Controlled Wet Commissioning

Perform these checks only when water piping and electrical protections are installed:

- [ ] **Emergency Stop Response**: Press the physical enclosure e-stop button or trigger the digital e-stop in the Android app; verify the contactor immediately drops out within < 100 ms in all modes (AUTO, MANUAL, COUNTDOWN).
- [ ] **Dry-Run Detection Test**: With the pump active, simulate zero or restricted flow (< 0.5 L/min); verify that firmware cuts relay power within 15 seconds and engages a persistent `DRY_RUN` lockout.
- [ ] **Dry-Run Fault Persistence**: Power-cycle the master controller during a dry-run fault; verify that the lockout persists across reboot until explicitly cleared via the user app.
- [ ] **Maximum Continuous Runtime Ceiling**: Verify that continuous pump operation stops at the configured ceiling timeout (default 45 min) to prevent catastrophic flooding if a sensor hangs.
- [ ] **Minimum Off-Time Protection**: Verify that the controller enforces a resting interval between cycles to prevent rapid motor cycling and thermal stress.
- [ ] **Offline Autonomy**: Disconnect the local Wi-Fi router; confirm that local firmware continues executing autonomous tank fill and safety protections without cloud connectivity.

---

## 5. Emergency Operational Runbooks

### Incident A: Pump Fails to Shut Down
1. **Immediate Action**: Flip the main 20A miniature circuit breaker (MCB) on the enclosure supply line to mechanically cut all power.
2. **Diagnosis**: Check if the relay module contacts welded or if the manual rotary bypass switch is engaged.
3. **Verification**: Inspect contactor coil voltage with a multimeter before re-energizing.

### Incident B: Tank Overflows
1. **Immediate Action**: Engage the physical emergency-stop button or flip the manual supply breaker.
2. **Diagnosis**: Inspect the JSN-SR04T ultrasonic sensor lens for condensation, foam, or debris. Verify flow meter pulse integrity.
3. **Recovery**: Clear the fault code via the Android app only after verifying actual physical tank levels.

### Incident C: Wi-Fi or Cloud Disconnection
- **Controller Behavior**: The ESP32 enters local autonomous mode. Scheduled cycles and safety cutoffs continue operating locally using non-volatile storage (NVS) thresholds.
- **Recovery**: Re-provision Wi-Fi credentials via Bluetooth Low Energy (BLE) using the Android setup screen if network credentials have changed.

---

## 6. Preventative Maintenance Schedule

| Interval | Inspection Item | Verification Procedure |
|:---|:---|:---|
| **Monthly (30 Days)** | Thermal Overload Relay | Press the manual TOR test lever to verify mechanical tripping under power. |
| **Monthly (30 Days)** | Enclosure Weatherproofing | Inspect IP65 seals and cable gland nuts for moisture or condensation. |
| **Quarterly (90 Days)** | Emergency Stop | Test response time of physical and mobile e-stop controls. |
| **Quarterly (90 Days)** | Sensor Calibration | Measure physical tank water level with a dip tape; verify telemetry matches within ± 2 cm. |
| **Annually (1 Year)** | Contactor Contacts | Inspect CJX2 contactor contacts for electrical pitting or carbon buildup; replace if worn. |

---

## 7. Commissioning Acceptance Sign-Off

| Parameter | Specification | Verified Value | Pass / Fail |
|:---|:---|:---|:---:|
| **Earth Ground Resistance** | < 1.0 Ω to pump casing | | |
| **Thermal Overload Setting** | Motor Nameplate FLA (8–9A) | | |
| **Dry-Run Cutoff Delay** | ≤ 15.0 seconds | | |
| **Max Runtime Cutoff** | ≤ 45 minutes | | |
| **E-Stop Latched Reaction** | < 100 milliseconds | | |

- **Field Site Location**: Leon, Iloilo, Philippines
- **Commissioning Engineer**: Mark Alvin Cadangin
- **System Specifications**: [docs/specs/README.md](docs/specs/README.md)
