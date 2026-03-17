## RS-485 Tank Link — Protocol

This document defines the minimal protocol between:

- The **tank-side NodeMCU V2 (ESP8266, CP2102) node** (near the JSN-SR04T sensor), and
- The **main ESP32 controller** in the pump enclosure.

### Physical layer

- Bus: RS-485 half-duplex over CAT6 twisted pair (Green/Green-White).
- Speed: 115200 baud, 8N1 (default UART settings).
- Termination: optional 120 Ω across A/B at both ends if noise is observed.
- **CLARIFIED power:** The main enclosure uses a **local 5V adapter** for the ESP32. A **separate 12V adapter** is injected into CAT6 and travels to the tank enclosure, where an LM2596 buck converter generates the local 5V rail for the NodeMCU, sensors, and MAX485.
- **CLARIFIED grounding:** Run CAT6 **GND** alongside A/B and tie it to both transceivers’ GND pins (ESP32/MAX485 side and NodeMCU/MAX485 side). RS-485 is differential, but the transceivers still require a shared reference to stay within common-mode limits.

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

### FINAL UART + DE/RE pins (production)

**ESP32 (main enclosure):**

- TX2 (GPIO17) → MAX485 DI
- RX2 (GPIO16) ← MAX485 RO
- DE/RE (GPIO5) → MAX485 DE+RE (tied)

**NodeMCU V2 (tank enclosure):**

- TX (GPIO1) → MAX485 DI
- RX (GPIO3) ← MAX485 RO
- DE/RE (D5 / GPIO14) → MAX485 DE+RE (tied)

> **Flashing note (NodeMCU):** Disconnect MAX485 from NodeMCU TX/RX during USB flashing, then reconnect.

