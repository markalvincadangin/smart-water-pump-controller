# SmartFlow — RS-485 Protocol Specification

**Version:** 2.0 (updated in refactor — LDSC field added)
**CRC:** CRC16-Modbus, polynomial 0xA001, initial value 0xFFFF
**Baud:** 115200, 8N1
**Topology:** Half-duplex, master polls slave

---

## Frame Formats

### Request — ESP32 (master) → NodeMCU (slave)

```
REQ\n
```

Simple ASCII request. Master asserts DE/RE HIGH, sends `REQ\n`, then switches to receive
(DE/RE LOW) after `Serial2.flush()`.

### Response — NodeMCU → ESP32

```
STX LVL:<pct>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;LDSC:<n>;SEQ:<seq>;CRC:<hex4> ETX
```

Where `STX = 0x02` (ASCII SOT), `ETX = 0x03` (ASCII EOT).

### Field Definitions

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `LVL` | int | 0–100 | Water level percentage |
| `DIST` | float (1 dp) | 2.0–300.0 | Raw ultrasonic distance in cm |
| `FLOW` | float (2 dp) | 0.00–100.00 | Flow rate in L/min |
| `ERR` | int (bitmask) | 0–3 | Bit 0 = ultrasonic error, Bit 1 = flow error |
| `LDSC` | int | 0–255 | Level reading discard count since last frame |
| `SEQ` | uint8 | 0–255 | Wrapping sequence number |
| `CRC` | hex4 | 0000–FFFF | CRC16-Modbus — see CRC section below |

### Example Frame

```
\x02LVL:82;DIST:45.2;FLOW:8.30;ERR:0;LDSC:0;SEQ:142;CRC:A3F1\x03
```

---

## CRC Calculation

**Algorithm:** CRC16-Modbus
**Polynomial:** 0xA001 (reflected form of 0x8005)
**Initial value:** 0xFFFF
**Input reflection:** Yes
**Output reflection:** Yes
**XOR output:** 0x0000

**Covered bytes:** All ASCII payload bytes between STX and `CRC:` field, inclusive of all
semicolons, exclusive of STX/ETX and the `CRC:XXXX` field itself.

**For the example above, CRC covers:**
```
LVL:82;DIST:45.2;FLOW:8.30;ERR:0;LDSC:0;SEQ:142;
```

**Reference implementation (C):**
```c
uint16_t crc16_modbus(const uint8_t* data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
```

**Self-test vector:**
Input bytes: `LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;`
Expected CRC: compute at build time and store as `TC_EXPECTED_CRC` constant in TC-S-05.

---

## Timing Parameters

| Parameter | Value | Basis |
|-----------|-------|-------|
| Frame response timeout | 250 ms | RS-485 round-trip + sensor read latency |
| Max retries | 3 | 3 × 250 ms = 750 ms max stall |
| Turnaround guard — ESP32 | 80 µs | After `Serial2.flush()`, before DE LOW |
| Turnaround guard — NodeMCU | 60 µs | Before response transmission |
| Inter-byte stall reset | 20 ms | Bug M-03 fix |
| Poll interval (normal) | 3 s | Firebase update cycle |
| Poll interval (idle) | configurable | `idle_sensor_interval_ms` from Firebase config |

---

## Direction Control Pattern

### ESP32 (master) — sending request

```cpp
// Assert bus drive
digitalWrite(RS485_DE_RE_PIN, HIGH);
delayMicroseconds(10);  // Bus propagation settle

// Send request
Serial2.print("REQ\n");
Serial2.flush();  // CRITICAL: wait for TX FIFO to drain before releasing bus

// Release bus — 80µs guard
delayMicroseconds(80);
digitalWrite(RS485_DE_RE_PIN, LOW);

// Now listen for response
```

### NodeMCU (slave) — sending response

```cpp
// Assert bus drive
digitalWrite(PIN_RS485_DE_RE, HIGH);
delayMicroseconds(10);

// Send response frame
SN_SERIAL_RS485.write(0x02);  // STX
SN_SERIAL_RS485.print(frameBuffer);
SN_SERIAL_RS485.write(0x03);  // ETX
SN_SERIAL_RS485.flush();

// Release bus — 60µs guard
delayMicroseconds(60);
digitalWrite(PIN_RS485_DE_RE, LOW);
```

**CRITICAL:** `flush()` must be called before pulling DE LOW. Without it, the UART TX FIFO
may not have drained, and the last bytes are transmitted after the bus is released —
causing corrupted frames on the master side.

---

## LDSC Field — Backward Compatibility

`LDSC` is a new field added in refactor v2.0. The ESP32 parser must treat it as optional.

```cpp
// In parseSensorFrameStrict() or equivalent
int parsedLDSC = 0;  // Default if field absent

// After parsing required fields, attempt optional LDSC:
char* ldscPtr = strstr(payloadBuf, "LDSC:");
if (ldscPtr) {
  parsedLDSC = atoi(ldscPtr + 5);
}
remoteSensorLevelDiscardCount = parsedLDSC;
```

Old NodeMCU firmware without `LDSC` field continues to work with new ESP32 firmware.
New NodeMCU firmware with `LDSC` field works with both old and new ESP32 firmware.

---

## Error Handling

### Master-side (ESP32)

On CRC failure:
- Increment `rs485CrcErrorCount`
- Do not update `waterLevelPct` or `flowRateLpm`
- Retain last valid values
- If failures exceed `cfgSensorFailureThreshold`, set sensor error flag

On timeout:
- Increment retry counter
- Re-assert DE/RE and retransmit `REQ\n`
- After 3 timeouts: mark `remoteSensorStable = false`, log at `LOG_WARN`

### Slave-side (NodeMCU)

On partial frame stall (Bug M-03 fix):
- If `rxPos > 0` and no new byte for 20 ms → reset `rxPos = 0`
- Log at `LOG_DEBUG`

On unrecognized command:
- Ignore. Do not respond. Do not assert bus.

---

## Sequence Number Verification

The `SEQ` field wraps 0–255. The master should detect out-of-sequence responses:

```cpp
uint8_t expectedSeq = (lastGoodSeq + 1) & 0xFF;
if (parsedSeq != expectedSeq && remoteSensorStable) {
  LOG(LOG_WARN, "RS485", "Seq discontinuity: expected %d got %d", expectedSeq, parsedSeq);
}
lastGoodSeq = parsedSeq;
```

---

## Protocol Document Version Control

This document lives at `docs/specs/rs485_protocol.md`.
Any change to the frame format, field order, CRC algorithm, or timing parameters must:
1. Update this document with the new version number
2. Update both firmware implementations simultaneously
3. Verify backward compatibility or document the breaking change explicitly
