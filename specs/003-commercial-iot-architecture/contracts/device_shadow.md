# Device Shadow Contract

The Device Shadow resolves race conditions between cloud clients (Android app) and the physical embedded device. It utilizes the `desired` vs `reported` state pattern.

## Location
`/devices/<device_id>/shadow/`

## Structure

```json
{
  "desired": {
    "pumpState": true,
    "mode": "AUTO",
    "clearError": false
  },
  "reported": {
    "pumpState": true,
    "mode": "AUTO",
    "clearError": false
  }
}
```

## Rules of Engagement

1. **Clients (Android App, Admin API)**: 
   - MUST ONLY write to the `desired` node.
   - MUST NEVER write to the `reported` node.
   - Read both nodes. If `desired != reported`, the UI should indicate a "pending" or "syncing" state.

2. **Device (ESP32 Firmware)**:
   - MUST listen for changes on the `desired` node.
   - MUST ONLY write to the `reported` node.
   - When a `desired` change is detected, the device evaluates if it is physically safe to apply the change (e.g., ignoring pump ON if dry-run lockout is active). 
   - After evaluation and actuation, the device writes the new physical state to `reported`.
