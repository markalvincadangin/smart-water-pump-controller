# SmartFlow — RS-485 Protocol Specification
### Document version: 1.0 | 2026-03-31
### Applies to: NodeMCU V2 firmware v2.x (Phase 2) + ESP32 master firmware v2.x (Phase 3)

This document is the **authoritative contract** for the RS-485 half-duplex link between
the ESP32 master controller and the NodeMCU V2 sensor node. All firmware implementations
and test sketches must conform to this specification. Changes to this document require
corresponding changes to both firmware implementations.

---

## 1. Physical Layer

| Parameter | Value |
|-----------|-------|
| Protocol  | RS-485 half-duplex (MAX485 or equivalent) |
| Baud rate | 115,200 bps |
| Frame format | 8N1 (8 data bits, no parity, 1 stop bit) |
| Topology | Point-to-point (1 master, 1 slave) |
| Direction control | DE/RE tied to single GPIO (HIGH = transmit, LOW = receive) |

### Wiring

```
ESP32 GPIO17 (TX2) ─── MAX485 DI
ESP32 GPIO25 (RX2) ─── MAX485 RO
ESP32 GPIO5  (DE)  ─── MAX485 DE + RE (tied)

NodeMCU GPIO1 (TX/UART0) ─── MAX485 DI
NodeMCU GPIO3 (RX/UART0) ─── MAX485 RO
NodeMCU D5/GPIO14       ─── MAX485 DE + RE (tied)
```

**Deployment requirement:** RS-485 requires correct biasing resistors and cable termination
for lengths above ~1 m. A failed ground reference between nodes is the most common field fault.

---

## 2. Message Formats

### 2.1 Request (ESP32 → NodeMCU)

```
REQ\n
```

ASCII text, newline-terminated. No framing, no CRC. The master transmits this,
then immediately switches to receive mode (DE/RE LOW) before the slave can respond.

### 2.2 Response (NodeMCU → ESP32)

```
STX  payload  CRC:XXXX  ETX
```

Where:
- `STX` = byte `0x02` (start-of-text)
- `ETX` = byte `0x03` (end-of-text)
- `payload` = ASCII key-value pairs separated by `;`
- `CRC:XXXX` = 4 hex-digit CRC16 appended to the payload

**Complete frame example:**
```
\x02LVL:82;DIST:45.2;FLOW:8.30;ERR:0;LDSC:0;SEQ:142;CRC:3F9A\x03
```

---

## 3. Payload Fields

| Field | Type | Range | Required | Description |
|-------|------|-------|----------|-------------|
| `LVL`  | int | 0–100 | Yes | Water level as percentage of tank capacity |
| `DIST` | float (1 dp) | 2.0–300.0 | Yes | Raw ultrasonic distance in cm (preferred measurement) |
| `FLOW` | float (2 dp) | 0.00–100.00 | Yes | Flow rate in L/min |
| `ERR`  | int (bitmask) | 0–7 | Yes | Sensor error flags (see §3.1) |
| `LDSC` | int | 0–255 | Optional | Level discard count since last frame (see §3.2) |
| `SEQ`  | uint8 | 0–255 | Yes | Wrapping frame sequence number |
| `CRC`  | hex4 | 0000–FFFF | Yes | CRC16-Modbus (see §4) |

### 3.1 ERR Field Bitmask

| Bit | Value | Meaning |
|-----|-------|---------|
| 0 | `0x01` | Ultrasonic sensor error (timeout or all samples invalid) |
| 1 | `0x02` | Flow sensor error (excessive noise pulses — hysteretic, see §3.3) |

`ERR:0` = all sensors nominal. `ERR:3` = both ultrasonic and flow faults active.

### 3.2 LDSC Field (Phase 2, optional)

`LDSC` (Level Discard Count) carries the number of level readings rejected by the plausibility
filter since the last successful frame. Capped at 255. Value 0 indicates no discards.

**Backward compatibility:** The ESP32 parser treats `LDSC` as optional. If absent from the
frame, `remoteSensorLevelDiscardCount` is set to 0. Old NodeMCU firmware without `LDSC`
operates correctly with new ESP32 firmware.

### 3.3 Flow Error Hysteresis

The `ERR` bit 1 (flow error) is set/cleared with hysteresis to prevent oscillation:
- **Assert:** `disc_count > 50` for **3 consecutive** 1-second windows
- **Clear:** `disc_count ≤ 20` for **5 consecutive** 1-second windows
- **Hold:** `20 < disc_count ≤ 50` — current state preserved

### 3.4 Field Ordering

Fields must appear in the order: `LVL`, `DIST`, `FLOW`, `ERR`, `LDSC` (if present), `SEQ`, `CRC`.
The `CRC` field is always last before `ETX`. The receiver uses `strstr()` field search and is
tolerant of unknown fields between known fields, but field order must not place `CRC` before `SEQ`.

---

## 4. CRC Algorithm

**Algorithm:** CRC16-Modbus  
**Polynomial:** `0xA001` (bit-reversed form of `0x8005`)  
**Initial value:** `0xFFFF`  
**Output width:** 16 bits, formatted as 4 uppercase hex digits (zero-padded)  
**Input:** ASCII payload bytes starting from `LVL:` up to and including the `;` after `SEQ:<n>` — i.e., everything between STX and `CRC:`, not including STX or the `CRC:` label itself.

**Reference implementation (C):**
```cpp
uint16_t crc16_modbus(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0xA001);
      else              crc = (uint16_t)(crc >> 1);
    }
  }
  return crc;
}
```

**Self-test vector:**  
Input: `LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;`  
Expected CRC: compute at compile time and embed in TC-S-05 (see Phase 5 test spec).

---

## 5. Half-Duplex Timing

| Parameter | Value | Basis |
|-----------|-------|-------|
| Request interval | 1000 ms (normal), 10000 ms (idle mode) | `RS485_REQ_INTERVAL_MS` |
| Frame response timeout | 250 ms | `RS485_FRAME_TIMEOUT_MS` |
| Max retries per poll cycle | 3 | `RS485_MAX_RETRIES` |
| Max stall before offline | 750 ms (3 × 250 ms timeout) | |
| DE/RE turnaround guard (ESP32) | 80 µs | `RS485_TX_TURNAROUND_US`, TIA-485-A |
| DE/RE turnaround guard (NodeMCU) | 60 µs | `RS485_TX_TURNAROUND_US` in sensor_node_shared.h |
| Inter-byte stall reset (NodeMCU) | 20 ms | Phase 2 M-03 fix |
| Sensor declared offline | 5000 ms since last valid frame | `REMOTE_SENSOR_OFFLINE_MS` |
| Stability latch assert | 3 consecutive valid frames | `REMOTE_STABLE_ONLINE_N` |
| Stability latch clear | 3 consecutive failed frames | `REMOTE_STABLE_OFFLINE_N` |

---

## 6. Master Acceptance Rules (Safety-Critical)

The ESP32 master rejects a frame and does **not** update state if any of the following are true:

1. No STX/ETX framing
2. `CRC:` field absent or not 4 valid hex digits
3. CRC mismatch (computed ≠ received)
4. `LVL:` field absent or value out of range [0, 100]
5. `FLOW:` field absent or value out of range [0.0, 100.0]
6. `ERR:` field absent or value out of range [0, 7]
7. `SEQ:` field absent
8. `DIST:` present but value out of range [1.0, 300.0]
9. Level computed from DIST is outside [-5%, 105%] (after tank calibration)

On rejection, the master increments `remoteSensorConsecutiveFailCount` and
`ultrasonicCycleTimeoutCount`. After `REMOTE_STABLE_OFFLINE_N` consecutive failures,
`remoteSensorStable = false`, which gates pump starts (fail-safe).

---

## 7. Sequence Number Behaviour

- `SEQ` is a `uint8_t` on the NodeMCU — wraps 255 → 0.
- The master stores `lastSeqSeen` and counts duplicate frames (`dupSeqCount`).
- Duplicate detection is informational only — does not cause rejection.
- The master does not enforce strict sequence ordering; only duplicate detection is active.

---

## 8. Level Derivation Preference

When `DIST:` is present and valid in a frame, the master re-derives `waterLevelPct` from
the raw distance using the configured tank calibration (`cfgTankEmptyCm`, `cfgTankFullCm`).
This prevents calibration drift from accumulating in the NodeMCU's percentage calculation.

When `DIST:` is absent or invalid, the master falls back to the `LVL:` field as-is.

---

## 9. Backward Compatibility Table

| NodeMCU firmware | ESP32 firmware | LDSC | DIST | Compatible? |
|---|---|---|---|---|
| Pre-Phase 2 (no LDSC, no DIST) | Phase 3 (parses optionally) | 0 (default) | absent | ✅ Yes |
| Phase 2 (LDSC + DIST present) | Pre-Phase 1 (legacy parser) | ignored | ignored | ⚠️ LVL used; CRC still checked |
| Phase 2 | Phase 3 | parsed | parsed | ✅ Full |

---

## 10. RS-485 Receive Buffer Sizes

| Node | Buffer constant | Size | Rationale |
|------|----------------|------|-----------|
| ESP32 (`RS485_RX_LINE_MAX`) | `smart_water_pump_controller_shared.h` | 128 bytes | Increased from 96 in Phase 3 to accommodate LDSC field |
| NodeMCU (`rxLine[]`) | `03_rs485_slave.ino` | 32 bytes | REQ\n command is short; no increase needed |
| NodeMCU payload buffer | `03_rs485_slave.ino` | 104 bytes | Increased from 96 in Phase 2 for LDSC field |
| NodeMCU frame buffer | `03_rs485_slave.ino` | 128 bytes | Wraps payload + STX/ETX/CRC |
