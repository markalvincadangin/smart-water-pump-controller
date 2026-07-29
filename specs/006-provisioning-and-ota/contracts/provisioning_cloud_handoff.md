# Provisioning Cloud Handoff Contract

## Scope

This contract begins only after BLE has delivered the terminal `status: "provisioned"` response. The ESP32 may then close BLE while joining Wi-Fi and bootstrapping its separate `device:{deviceId}` cloud identity.

## App behavior

1. The app retains `{ deviceId, pairingProof }` only in the active `ProvisioningViewModel`; it must never persist, log, or expose the proof.
2. The app calls `claimDevice({ deviceId, pairingProof })` immediately, then retries only retryable callable results every two seconds for at most 45 attempts (90 seconds total).
3. During every attempt the visible state is `WaitingForCloud(attempt, maxAttempts)` and clearly states that BLE is expected to be unavailable while the device joins Wi-Fi.
4. A successful callable response ends provisioning and routes to the owner dashboard.
5. A non-retryable claim result ends the attempt, clears the in-memory proof, and renders its bounded user-safe message.
6. A 90-second timeout retains the in-memory proof and offers `Retry cloud registration`; it must not automatically start a BLE scan. The user may separately choose `Start provisioning again`, which discards the proof.

## Cloud boundary

`claimDevice` remains the sole cloud-readiness check and the atomic ownership mutation. Android must not directly read `/devices/{deviceId}/pairing/current`, `/ownership`, or the owner index to preflight readiness. The callable must continue to return `CLAIM_UNAVAILABLE` for a not-yet-published verifier without disclosing existing-owner data.

## Clean-test reset boundary

The reset tool deletes all RTDB paths and Firebase Auth users only after a backup and explicit acknowledgement. If the caller requests a named registry seed, it writes only:

`/deviceRegistry/{deviceId}` = `{ state: "active", secretName, updatedAtMs }`

`secretName` is a non-secret Secret Manager resource name. The tool must never copy a Secret Manager value into RTDB, output, versioned source, or logs.
