# SmartFlow — Debug Infrastructure Reference

## Overview

Both firmware projects implement a unified 5-level structured log system.
Phase 1 of the refactor must be complete before any other firmware is modified.

---

## Log Level Constants

```cpp
#define LOG_ERROR   0   // Safety trips, hardware failures, crash detection — always output
#define LOG_WARN    1   // Degraded states, comm loss, approaching limits — always output
#define LOG_INFO    2   // State transitions, mode changes, boot events — always output
#define LOG_DEBUG   3   // Per-cycle readings, RS-485 frames — off in production
#define LOG_VERBOSE 4   // State machine internals, raw ISR counts — developer only
```

## Compile-Time Floor

```cpp
// In build config or shared header
// Development: LOG_COMPILE_FLOOR = LOG_DEBUG  → all levels compiled in
// Production:  LOG_COMPILE_FLOOR = LOG_INFO   → DEBUG/VERBOSE compiled out (zero overhead)
#ifndef LOG_COMPILE_FLOOR
  #define LOG_COMPILE_FLOOR LOG_DEBUG
#endif
```

## Log Format

```
[L][MODULE][MS] message content
```

- `L` = single char: `E`, `W`, `I`, `D`, `V`
- `MODULE` = 4–6 char uppercase: `PUMP`, `RS485`, `WIFI`, `FIREBASE`, `SENSOR`, `BOOT`, `NVS`, `SAFETY`
- `MS` = `millis()` zero-padded to 10 digits

Examples:
```
[E][PUMP][0045231] DRY_RUN lockout. flow=0.08LPM < 1.0LPM for 30s. Relay OFF.
[W][RS485][0046002] Frame timeout attempt 2/3. Retrying.
[I][BOOT][0001240] NVS config loaded. mode=AUTO_STANDBY cycles=142
[D][SENSOR][0047100] lvl=82% dist=45.2cm flow=8.30LPM err=0 seq=142
[V][SAFETY][0047105] DryRun timer: 2100ms / 30000ms. flow=0.08LPM
```

---

## ESP32 Implementation

### Shared header (`smart_water_pump_controller_shared.h`)

```cpp
#define LOG_ERROR   0
#define LOG_WARN    1
#define LOG_INFO    2
#define LOG_DEBUG   3
#define LOG_VERBOSE 4

#ifndef LOG_COMPILE_FLOOR
  #define LOG_COMPILE_FLOOR LOG_DEBUG
#endif

static const char LOG_LEVEL_CHAR[] = { 'E', 'W', 'I', 'D', 'V' };
extern uint8_t gLogLevel;

#define LOG(level, module, fmt, ...) \
  do { \
    if ((level) <= LOG_COMPILE_FLOOR && (level) <= gLogLevel) { \
      Serial.printf("[%c][%s][%010lu] " fmt "\n", \
        LOG_LEVEL_CHAR[level], module, millis(), ##__VA_ARGS__); \
    } \
  } while(0)
```

### Global declaration (`01_config.ino`)

```cpp
uint8_t gLogLevel = LOG_INFO;  // Default: INFO. Overridden by NVS or Firebase config.
```

### Remote log level via Firebase

In `readDeviceConfigFromFirebase()`:
```cpp
int remoteLogLevel = configJson.getInt("debug_log_level", gLogLevel);
if (remoteLogLevel >= LOG_ERROR && remoteLogLevel <= LOG_VERBOSE) {
  if (remoteLogLevel != gLogLevel) {
    LOG(LOG_INFO, "NVS", "Log level changed: %d → %d", gLogLevel, remoteLogLevel);
    gLogLevel = (uint8_t)remoteLogLevel;
  }
}
```

In `pushFirebaseStatus()`:
```cpp
statusJson.set("debug_log_level", (int)gLogLevel);
```

---

## NodeMCU Implementation

### Transport modes

```cpp
// In sensor node shared header
#ifndef DEBUG_USB_MODE
  #define DEBUG_USB_MODE 0
#endif

#if DEBUG_USB_MODE == 1
  #warning "DEBUG_USB_MODE=1: RS-485 is DISABLED. Never flash to a deployed device."
  #define SN_SERIAL_DEBUG Serial       // UART0 → USB
  // RS-485: MAX485 DI/RO must be physically disconnected in this mode
#else
  #define SN_SERIAL_DEBUG Serial1      // GPIO2 TX-only → USB-TTL adapter
  // RS-485: UART0 (GPIO1 TX / GPIO3 RX) → MAX485
#endif

#define SN_LOG_LEVEL_CHAR "EWIDV"

#ifndef SN_LOG_COMPILE_FLOOR
  #define SN_LOG_COMPILE_FLOOR LOG_DEBUG
#endif

extern uint8_t snLogLevel;

#define LOG_SN(level, module, fmt, ...) \
  do { \
    if ((level) <= SN_LOG_COMPILE_FLOOR && (level) <= snLogLevel) { \
      SN_SERIAL_DEBUG.printf("[%c][%s][%010lu] " fmt "\n", \
        SN_LOG_LEVEL_CHAR[level], module, millis(), ##__VA_ARGS__); \
    } \
  } while(0)
```

### Setting log level on NodeMCU

No WiFi/Firebase on NodeMCU. Log level is set at compile time via `SN_LOG_COMPILE_FLOOR`
or via the `snLogLevel` global (which can be initialized from a compile-time constant).

### Field debugging without reflash

```
1. Connect USB-TTL adapter:
   - Adapter RX → NodeMCU GPIO2
   - Adapter GND → NodeMCU GND
2. Open terminal at 115200 baud
3. Debug output streams via GPIO2
4. RS-485 on UART0 (GPIO1/3) is completely unaffected
```

Note: The NodeMCU onboard LED is wired to GPIO2. It will flicker with Serial1 output.
This is expected and does not indicate a fault.

---

## Serial Volume Triage

Migration table for existing `Serial.printf` / `Serial.println` calls:

| Original content | Target level | Rationale |
|---|---|---|
| Safety trips, TOR detection, hardware failures | `LOG_ERROR` | Must always be visible |
| DRY_RUN, OVERFLOW, comm loss, sensor errors | `LOG_WARN` | Must always be visible |
| Mode changes, boot sequence, NVS load | `LOG_INFO` | Visible in production |
| RS-485 frame bytes, CRC values, per-cycle readings | `LOG_DEBUG` | Gated in production |
| State machine timer values, raw ISR counts | `LOG_VERBOSE` | Developer only |

**Rate limiting for WARN:** Messages that repeat on the same sustained condition must be
rate-limited to once per 60 seconds:

```cpp
static uint32_t lastWifiWarnMs = 0;
if (!wifiConnected && millis() - lastWifiWarnMs > 60000) {
  LOG(LOG_WARN, "WIFI", "WiFi disconnected. Attempting reconnect...");
  lastWifiWarnMs = millis();
}
```

**Target production output volume:** ≥80% reduction from baseline in steady-state
normal operation. Only boot sequence + error/warning events reach the serial output.

---

## PlatformIO Build Flags

In `platformio.ini` for the ESP32 project:

```ini
[env:production]
build_flags =
  -DLOG_COMPILE_FLOOR=2       ; LOG_INFO — DEBUG/VERBOSE compiled out

[env:development]
build_flags =
  -DLOG_COMPILE_FLOOR=3       ; LOG_DEBUG — all levels compiled in
```

In `platformio.ini` for the NodeMCU project:

```ini
[env:production]
build_flags =
  -DDEBUG_USB_MODE=0          ; Serial1/GPIO2 for debug, UART0 for RS-485
  -DSN_LOG_COMPILE_FLOOR=2    ; LOG_INFO

[env:bench]
build_flags =
  -DDEBUG_USB_MODE=1          ; USB Serial for debug (RS-485 disabled)
  -DSN_LOG_COMPILE_FLOOR=3    ; LOG_DEBUG
```

In Arduino IDE: set `LOG_COMPILE_FLOOR` and `DEBUG_USB_MODE` in the shared header directly,
and re-comment before production flash.

---

## Phase 1 Exit Criteria Checklist

- [ ] `LOG()` macro in both firmware projects
- [ ] `LOG_SN()` macro with `DEBUG_USB_MODE` routing in NodeMCU project
- [ ] All existing `Serial.printf` and `Serial.println` migrated to `LOG()`
- [ ] `gLogLevel` remote control via Firebase config working on ESP32
- [ ] `debug_log_level` field in Firebase status push
- [ ] `#warning` fires when `DEBUG_USB_MODE=1` is compiled
- [ ] Serial output volume in `LOG_INFO` mode is ≥80% reduced from baseline
- [ ] No functional behavior changes — observability only
