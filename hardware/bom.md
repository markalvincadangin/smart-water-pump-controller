# Bill of Materials (BoM)

Project: SmartFlow  
Location: Leon, Iloilo, Philippines  
Application: 1.5 HP single-phase deep-well pump automation (220V AC)

## 1) Safety-Critical Electrical Components

| Item | Component | Minimum Specification | Qty | Notes |
|---|---|---|---:|---|
| HV-01 | Miniature Circuit Breaker (MCB) | 2-pole, 20A, 220V AC | 1 | Main panel isolation and branch protection |
| HV-02 | Magnetic Contactor | CJX2-2510 equivalent, 220V AC coil | 1 | Motor power switching |
| HV-03 | Thermal Overload Relay (TOR) | LR2-D13, adjustable 7-10A | 1 | Set to motor FLA (typically 8-9A) |
| HV-04 | DIN Rail | 35 mm | 1 | Mount MCB, contactor, TOR |
| HV-05 | Pump power cable | 3-conductor, 12 AWG (L/N/PE) | 1 run | Pump feed must include protective earth |

Important:
- Earth (PE) is never switched through contactor or TOR.
- TOR T3/L3 are unused in single-phase install and must be insulated/capped.

## 2) Main Enclosure Low-Voltage Control

| Item | Component | Minimum Specification | Qty | Notes |
|---|---|---|---:|---|
| LV-01 | ESP32 DevKit V1 | 38-pin, 3.3V logic | 1 | Master controller |
| LV-02 | Relay module | 1-channel, 5V coil, opto-isolated preferred | 1 | Coil trigger path enable/disable |
| LV-03 | AC-DC adapter (main box) | 5V, >=3A | 1 | Powers ESP32, relay, local RS-485 module |
| LV-04 | RS-485 transceiver (main box) | MAX485-compatible | 1 | ESP32 UART2 to A/B bus |
| LV-05 | Terminal blocks | DIN or enclosed strip, rated for use | as needed | Neutral/low-voltage distribution |
| LV-06 | Push Button | Momentary, normally open (NO) | 1 | Smart Reset Button (Soft Reboot / Factory Reset) |

## 3) Tank-Side Sensor Node (Remote Enclosure)

| Item | Component | Minimum Specification | Qty | Notes |
|---|---|---|---:|---|
| TN-01 | NodeMCU V2 (ESP8266) | CP2102 variant preferred | 1 | Sensor node |
| TN-02 | RS-485 transceiver (tank box) | MAX485-compatible | 1 | NodeMCU UART0 to A/B bus |
| TN-03 | Ultrasonic sensor | JSN-SR04T-2.0 waterproof probe type | 1 | Tank level distance |
| TN-04 | Flow sensor | YF-G1, 1-inch | 1 | Flow and dry-run signal source |
| TN-05 | AC-DC adapter (distribution source) | 12V, >=3A | 1 | Injected over CAT6 power pairs |
| TN-06 | Buck converter | LM2596 or equivalent, 12V to 5V | 1 | Local 5V rail in tank box |
| TN-07 | Inline DC fuse | 500mA to 1A | 1 | Protects 12V branch to CAT6 |
| TN-08 | Termination resistors | 120 ohm, 1/4W | 2 | One at each RS-485 bus end |
| TN-09 | Decoupling capacitors | 100nF + 47uF to 470uF | several | Local rail stability |

## 4) Enclosures and Cable Plant

| Item | Component | Minimum Specification | Qty | Notes |
|---|---|---|---:|---|
| ENC-01 | Main enclosure | IP65 ABS, 300 x 400 x 200 mm | 1 | Main control panel |
| ENC-02 | Tank enclosure | IP65 ABS, approx 150 x 150 x 100 mm | 1 | Remote node near tank |
| ENC-03 | Cable glands (main box) | PG16 x2, PG9 x1 | 3 | Grid in, pump out, CAT6 out |
| ENC-04 | Outdoor CAT6 cable | 4 twisted pairs, 30-40 m | 1 run | RS-485 + distributed DC power |
| ENC-05 | Earthing hardware | PE lug, ring terminals, 12 AWG PE wire | as needed | Bonding and compliance |

## 5) Signal Conditioning Components

| Item | Component | Recommended Values | Qty | Notes |
|---|---|---|---:|---|
| SC-01 | JSN ECHO divider resistors | 10k (series) + 20k (to GND) | 1 set | 5V ECHO to ~3.3V MCU-safe |
| SC-02 | Flow SIG divider resistors | 1k (series) + 2k or 2.2k (to GND) | 1 set | Use if measured flow signal exceeds 3.3V |
| SC-03 | RC Snubber (Suppressor) | 0.1uF + 100 ohm (or pre-made 220V AC snubber) | 1 | Across contactor A1/A2 to absorb back-EMF |

## 6) Firmware Pin Baseline (As Deployed)

Source of truth:
- firmware/master_node/src/config/config.h
- firmware/sensor_node/src/config/config.h

ESP32 master pins:
- RELAY_PIN = GPIO4
- RS485_TX_PIN = GPIO17
- RS485_RX_PIN = GPIO25
- RS485_DE_RE_PIN = GPIO5
- PIN_RESET_BUTTON = GPIO32

NodeMCU sensor node pins:
- PIN_RS485_DE_RE = GPIO14 (D5)
- PIN_FLOW_INPUT = GPIO12 (D6)
- PIN_US_TRIG = GPIO5 (D1)
- PIN_US_ECHO = GPIO16 (D0)

## 7) Commissioning Acceptance Criteria

- TOR dial set and verified at motor nameplate FLA (typically 8-9A for current install).
- PE continuity from panel PE point to pump casing is less than 1 ohm.
- CAT6 A/B continuity and polarity verified end-to-end.
- No 220V conductors routed through low-voltage zone.
- LM2596 output trimmed to 5.00V before connecting NodeMCU/sensors.
- RS-485 link stable with periodic valid frames and no sustained CRC failures.

Revision: 2026-04-04 (aligned to deployed SmartFlow firmware pinout)
