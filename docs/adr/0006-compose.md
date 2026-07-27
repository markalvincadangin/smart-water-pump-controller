# ADR 0006: Jetpack Compose and MVVM

**Date**: 2026-07-27
**Status**: Accepted (Implementation Deferred to Epic 3)

## Context
The SmartFlow MVP utilized a Next.js web dashboard. While functional, it cannot support native BLE provisioning (Web Bluetooth is restricted on mobile), nor can it cleanly support robust background push notifications or local network device discovery. 

## Decision
We will build a native Android client utilizing Jetpack Compose and the Model-View-ViewModel (MVVM) architecture.
- **Compose**: Modern, declarative UI framework.
- **ViewModel**: Manages UI state and survives configuration changes.
- **Repository**: Single source of truth abstracting the Firebase backend and local BLE stack.

## Consequences
**Positive**:
- Access to native Android APIs (BLE, Wi-Fi configuration, Push).
- Snappier, more premium user experience.

**Negative**:
- Abandons the cross-platform nature of the web dashboard.
- Requires dedicated Android engineering effort.
