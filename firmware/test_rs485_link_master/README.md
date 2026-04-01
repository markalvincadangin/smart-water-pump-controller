# RS-485 Master Link Diagnostic Test (ESP32)

Folder: firmware/test_rs485_link_master

This is a dedicated communication test for RS-485 only.
It sends one request every 3 seconds and prints PASS/FAIL with error counters.

## Pin Map (default)
- TX: GPIO17
- RX: GPIO25
- DE/RE: GPIO5
- Baud: 115200

## Behavior
- Request: PING:<seq>
- Timeout per request: 500 ms
- Expects framed reply with payload similar to:
  HELLO;SEQ:<seq>;NODE_OK:1;
- Prints continuous diagnostics:
  - PASS payload=...
  - FAIL timeout
  - FAIL crc/frame
  - FAIL missing SEQ
  - FAIL seq-mismatch

Every 15s, it prints summary counters and hints.

## Flash
arduino-cli upload -p COMx --fqbn esp32:esp32:esp32 firmware/test_rs485_link_master

## Monitor
arduino-cli monitor -p COMx -c baudrate=115200
