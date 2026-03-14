## RS-485 Tank Link — Protocol

This document defines the minimal protocol between:

- The **tank-side NodeMCU v3 (ESP8266) node** (near the JSN-SR04T sensor), and
- The **main ESP32 controller** in the pump enclosure.

### Physical layer

- Bus: RS-485 half-duplex over CAT6 twisted pair (Green/Green-White).
- Speed: 115200 baud, 8N1 (default UART settings).
- Termination: optional 120 Ω across A/B at both ends if noise is observed.
- Power: Both nodes share the same 5V and GND rails via CAT6.

### Message format

Each update is a single ASCII line:

```text
LVL:<percent>;ERR:<flag>\r\n
```

- `<percent>`: integer 0–100, water level percentage.
- `<flag>`: `0` (OK) or `1` (ultrasonic sensor error).

Examples:

```text
LVL:87;ERR:0\r\n
LVL:12;ERR:1\r\n
```

### Behaviour

- The tank node sends one frame every 1–2 seconds.
- On repeated JSN timeouts or invalid readings, the node sets `ERR:1`.
- When readings recover, the node sets `ERR:0` again.

The main ESP32:

- Treats missing frames for several seconds as "sensor offline" and may set `isSensorError`.
- Uses `LVL` as the authoritative `waterLevelPct` for pump logic and Firebase telemetry.

