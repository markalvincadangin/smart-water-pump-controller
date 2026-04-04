# RS-485 Tank Link

This document defines the active tank-node communication contract between NodeMCU sensor node and ESP32 controller.

## Link profile

- Bus: RS-485 half-duplex
- Baud: 115200 (8N1)
- Topology: single master (ESP32), single slave (NodeMCU)
- Medium: CAT6 pair for A/B plus shared GND reference

## Production pin map

### ESP32 controller

- TX2 GPIO17 -> MAX485 DI
- RX2 GPIO25 <- MAX485 RO
- DE/RE GPIO5 -> MAX485 DE+RE (tied)

### NodeMCU sensor node

- TX GPIO1 -> MAX485 DI
- RX GPIO3 <- MAX485 RO
- DE/RE GPIO14 (D5) -> MAX485 DE+RE (tied)

## Request and response contract

### Master request

```text
REQ\n
```

### Slave response frame

```text
\x02LVL:<pct>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;LDSC:<n>;SEQ:<seq>;CRC:<hex4>\x03
```

Field meaning:

- `LVL`: water level percent integer
- `DIST`: ultrasonic distance in cm (1 decimal)
- `FLOW`: flow rate in L/min (2 decimals)
- `ERR`: sensor error bitmask from node
- `LDSC`: level-discard count (0 to 255, saturating)
- `SEQ`: sequence counter (0 to 255)
- `CRC`: CRC16-Modbus over payload before `CRC:`

## Reliability expectations

- ESP32 applies timeout and retry for missed/invalid frames
- Node frame parser handles inter-byte stall reset
- Master parser supports backward compatibility where `LDSC` may be absent

## Wiring and grounding requirements

- Use twisted pair for A/B
- Carry shared GND between enclosures
- Add 120 ohm termination at ends if noise/reflections appear
- Keep RS-485 away from mains switching lines where possible

## Power architecture baseline

- ESP32 enclosure: local 5V supply
- Tank enclosure: independent 12V feed with buck conversion to local 5V for NodeMCU/sensors/transceiver

## Field verification checklist

1. Node sends framed payload continuously at expected interval.
2. ESP32 receives valid CRC frames with stable `SEQ` progression.
3. Disconnect/reconnect test transitions to offline and recovers cleanly.
4. Sensor fault induces expected `ERR` behavior and recovery after fault clears.
