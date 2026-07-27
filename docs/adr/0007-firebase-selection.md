# ADR 0007: Firebase Realtime Database Backend

**Date**: 2026-07-27
**Status**: Accepted (Implementation Deferred to Epic 2)

## Context
SmartFlow requires a cloud backend to route commands, store telemetry, and manage device ownership. Traditional IoT solutions (AWS IoT Core, GCP IoT) require managing MQTT brokers, complex certificate provisioning, and separate database/API layers.

## Decision
We will retain Firebase Realtime Database (RTDB) alongside Firebase Authentication as the cloud backend for the V2 Commercial Platform.
The schema will be strictly structured to mirror professional IoT models (Device Shadow, Telemetry, Metadata) rather than an organic data dump.

## Consequences
**Positive**:
- Extremely fast time-to-market.
- Built-in real-time synchronization via WebSockets perfectly suits the Device Shadow pattern.
- Out-of-the-box offline persistence for the Android App.
- Minimal infrastructure management.

**Negative**:
- Not a specialized IoT platform; lacks built-in fleet management features.
- Device must hold persistent TLS connections, consuming slightly more heap than a lightweight MQTT client.
