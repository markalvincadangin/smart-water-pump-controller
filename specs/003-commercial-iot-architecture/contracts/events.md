# Structured Event Contract

The structured event contract defines how the embedded device reports historical events (faults, state changes, lifecycle changes) to the cloud.

## Location
`/devices/<device_id>/events/`

## Structure

Events are pushed to the Realtime Database with a unique, monotonically increasing key (typically `evt_<timestamp>`).

```json
{
  "evt_1715629199": {
    "timestamp": 1715629199,
    "severity": "ERROR",
    "category": "SAFETY",
    "code": "DRY_RUN",
    "message": "Dry run detected."
  }
}
```

## Schema Fields

- `timestamp` (Integer): Unix epoch seconds.
- `severity` (String): Enum `INFO`, `WARN`, `ERROR`, `CRITICAL`.
- `category` (String): Enum `SAFETY`, `LIFECYCLE`, `NETWORK`, `SYSTEM`.
- `code` (String): Machine-readable fault or event code (e.g., `DRY_RUN`, `OVERFLOW`, `WIFI_DISCONNECT`, `OTA_START`).
- `message` (String): Human-readable context message.
