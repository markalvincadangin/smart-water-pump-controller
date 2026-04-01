# RS-485 Node Responder Test (NodeMCU ESP8266)

Folder: firmware/test_rs485_link_node

This is a dedicated communication responder for RS-485 only.
Keep this running while the master diagnostic test is polling.

## Pin Map (default)
- UART0 TX/RX used for RS-485 DI/RO
- DE/RE: D5 (GPIO14)
- Baud: 115200
- Optional debug log output: Serial1 (GPIO2 TX)

## Behavior
- Accepts commands:
  - PING:<seq>
  - PING
- Responds with framed payload and CRC:
  - HELLO;SEQ:<seq>;NODE_OK:1;
- Bad command response:
  - ERR:BAD_CMD;NODE_OK:1;

Every 5s, it prints node status on Serial1:
- commands received
- frames sent
- bad commands
- parse errors

## Flash
arduino-cli upload -p COMx --fqbn esp8266:esp8266:nodemcuv2 firmware/test_rs485_link_node

## Notes
- For one-laptop workflow, flash node first, keep it powered, then switch USB to master.
- This sketch is always-on and does not exit responder mode.
