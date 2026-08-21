# SmartFlow Deployment Safety Checklist

SmartFlow controls a mains-powered pump. This checklist summarizes repository-level validation; it is not a substitute for applicable electrical rules, qualified inspection, or the component manufacturers' instructions.

## Before working on the enclosure

- [ ] Isolate and lock out mains power.
- [ ] Verify absence of voltage with suitable test equipment.
- [ ] Keep protective earth continuous and unswitched.
- [ ] Confirm mains and low-voltage wiring are physically separated and secured.
- [ ] Confirm cable glands, covers, terminal insulation, and enclosure seals are fitted.
- [ ] Confirm the contactor and thermal overload relay match the motor and supply.
- [ ] Set the overload relay from the motor nameplate and installation requirements, not from software assumptions.

## Low-voltage checks

- [ ] Verify regulated rails before connecting either microcontroller.
- [ ] Confirm ultrasonic ECHO and any 5 V sensor outputs are shifted to MCU-safe levels.
- [ ] Verify RS-485 A/B polarity, termination, biasing, and ground reference.
- [ ] Confirm contactor-coil suppression is installed as documented in [wiring notes](hardware/wiring_notes.md).
- [ ] Check that no secret-bearing local configuration is committed.

## Software validation while the motor circuit is isolated

1. Build and test the Android app:

   ```powershell
   ./gradlew.bat test
   ./gradlew.bat assembleDebug
   ```

2. Build and test Cloud Functions:

   ```bash
   cd functions
   npm ci
   npm run build
   npm test
   ```

3. Compile both firmware targets:

   ```bash
   pio run -d firmware/master_node
   pio run -d firmware/sensor_node
   ```

4. With the motor circuit still isolated, verify controller startup leaves the relay de-energized.
5. Verify invalid, missing, stale, and CRC-failed sensor data cannot start the pump.
6. Verify the Android app reports authoritative device state rather than assuming a command succeeded.

## Controlled commissioning

- [ ] Use the native Android app to verify AUTO, MANUAL, and COUNTDOWN intent while monitoring reported state.
- [ ] Verify emergency stop remains reachable in every mode and latch state.
- [ ] Verify dry-run detection stops the pump and remains locked until explicitly cleared.
- [ ] Verify maximum-runtime protection stops the pump in every mode.
- [ ] Verify minimum off-time prevents rapid cycling.
- [ ] Disconnect Wi-Fi and confirm local safety logic continues operating.
- [ ] Interrupt the tank link and confirm stale data blocks starts and stops a running pump as specified.
- [ ] Confirm the thermal overload relay operates independently of the controller.

Do not bypass safety inputs or energize exposed equipment to create screenshots or demonstrations.

## Acceptance evidence

Record the date, firmware revision, app revision, tester, measured supply values, overload setting, RS-485 result, safety-test outcomes, and any deviations. Owner-observed operation should not be described as independent certification.

Canonical behavior is defined in:

- [Firmware operational rules](docs/specs/firmware_operational_rules.md)
- [Firmware architecture](docs/specs/firmware.md)
- [RS-485 protocol](docs/specs/rs485_protocol.md)
- [Android application specification](docs/specs/app.md)
- [Hardware wiring notes](hardware/wiring_notes.md)
