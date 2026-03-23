# Sensor node — PlatformIO (NodeMCU V2 / ESP8266)

Tank-side RS-485 slave + flow + ultrasonic. See repo `firmware/README.md` for system context.

## Environments (`platformio.ini`)

| Environment | Purpose |
|-------------|---------|
| **`nodemcuv2`** | Production: RS-485 on UART0, debug logs on `Serial1` (GPIO2 TX). |
| **`nodemcuv2_debug_usb`** | Bench: readable logs on USB; RS-485 slave **disabled** (UART0 free). |
| **`nodemcuv2_ota`** | Same as production **plus** Wi-Fi + **ArduinoOTA** for wireless uploads (`espota`). |
| **`nodemcuv2_ota_usb`** | **Identical firmware** to `nodemcuv2_ota`; uploads over **USB** (`esptool`) — use for the first flash. |

Build / upload (USB serial, default):

```bash
cd firmware/platformio_sensor_node
pio run -e nodemcuv2 -t upload
```

Monitor production debug (USB-TTL RX → GPIO2):

```bash
pio device monitor -e nodemcuv2
```

---

## OTA uploads (`nodemcuv2_ota`)

### 1. One-time: create Wi-Fi secrets

```bash
cd firmware/platformio_sensor_node
cp src/config/secrets_ota.h.example src/config/secrets_ota.h   # Linux / macOS
# Windows PowerShell:
Copy-Item src/config/secrets_ota.h.example src/config/secrets_ota.h
```

Edit `src/config/secrets_ota.h`:

- `OTA_WIFI_SSID` / `OTA_WIFI_PASSWORD` — same LAN as your PC running PlatformIO.
- `OTA_HOSTNAME` — optional; default `swpc-sensor-node` (mDNS: `swpc-sensor-node.local`).
- `OTA_UPLOAD_PASSWORD` — recommended on shared Wi-Fi; must match upload auth below.

`secrets_ota.h` is **gitignored**; never commit it.

### 2. First flash over USB (required once)

`nodemcuv2_ota` uses **`upload_protocol = espota`**, so a normal USB upload would try Wi-Fi. For the **first** install, use the USB helper env (same binary):

Disconnect MAX485 from NodeMCU TX/RX if your board needs that for flashing, then:

```bash
pio run -e nodemcuv2_ota_usb -t upload
```

(On Windows, pick the COM port in PlatformIO / add `--upload-port COM5` if needed.)

Reconnect RS-485 after upload.

> If you see `Please specify IP address…` when running `pio run -e nodemcuv2_ota` (compile only), it is harmless on many PlatformIO versions; the firmware still builds. For uploads, always pass `--upload-port` with the device IP/hostname.

Watch debug on GPIO2 (`Serial1`) for a line like:

`[SN] OTA: IP 192.168.x.x`

### 3. Later uploads over Wi-Fi (espota)

With the device on the same network as your computer:

```bash
pio run -e nodemcuv2_ota -t upload --upload-port 192.168.1.42
```

If mDNS works on your OS, you can try:

```bash
pio run -e nodemcuv2_ota -t upload --upload-port swpc-sensor-node.local
```

**If you set `OTA_UPLOAD_PASSWORD`**, pass the password to PlatformIO (example — replace `secret`):

```ini
; In platformio.ini under [env:nodemcuv2_ota], optional:
upload_flags =
  --auth=secret
```

Or use the PlatformIO UI / CLI equivalent for OTA auth.

### 4. Return to “no Wi-Fi” production

When you no longer need OTA, flash the normal production env (no Wi-Fi stack):

```bash
pio run -e nodemcuv2 -t upload
```

(Still USB unless you OTA-flash a binary built for `nodemcuv2`; the `nodemcuv2` and `nodemcuv2_ota` binaries are compatible hardware-wise — both keep RS-485 enabled.)

---

## Troubleshooting OTA

- **Upload fails / timeout** — Firewall on PC; device not on same subnet; wrong IP; try `ping` the module IP.
- **mDNS `.local` fails** — Use the numeric IP from the `[SN] OTA: IP …` log.
- **Wi-Fi never connects** — Credentials wrong or 2.4 GHz only (ESP8266); node still runs RS-485 while Wi-Fi retries every 5 s.
- **Garbage on USB serial** — In production/OTA, UART0 is RS-485; use GPIO2 + USB-TTL for `[SN]` logs.
