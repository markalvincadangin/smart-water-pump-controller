# Phase 0 Research: App UI/UX Redesign & Integration

## 1. Firebase RTDB Schema Compatibility
**Decision**: We will implement the v2 schema for the Android App RTDB integration. 
**Rationale**: The `spec.md` states the Android app MUST integrate with Firebase RTDB to stream `/devices/{deviceId}/telemetry` and write to `/devices/{deviceId}/shadow/desired`. The firmware is assumed to already support this v2 schema.
**Alternatives considered**: Polling an HTTP API (rejected, Firebase RTDB provides zero-latency streams).

## 2. Jetpack Compose UI/UX Architecture
**Decision**: Use `androidx.compose.material3` for the Design System, and a custom `Canvas` element for the animated fluid tank wave. Navigation will be handled by `navigation-compose`.
**Rationale**: The `app/build.gradle.kts` already includes Material 3 and Navigation Compose. A custom `Canvas` is the standard way to draw and animate bezier curves (waves) in Compose without pulling in heavy animation libraries like Lottie for a simple fluid effect.
**Alternatives considered**: Using classic Android Views (rejected, project is built on Compose).

## 3. UI Token Mapping from Web to Mobile
**Decision**: We will translate the web dashboard's `sf` colors to a custom Compose `ColorScheme`.
- Primary: Cyan `#0EA5E9`
- Secondary: Teal/Emerald `#10B981`
- Tertiary: Amber `#F5A623`
- Error: Red `#EF4444`
- Background/Surface: Slate `#0F172A` / `#1E293B`
**Rationale**: Provides exact visual continuity with the legacy `archive/dashboard` while aligning with Material 3 standards.

## 4. State Management
**Decision**: We will use Android `ViewModel` combined with Kotlin `StateFlow` to manage Firebase streams and UI state.
**Rationale**: Ensures UI survives configuration changes (rotations) and properly handles lifecycle events, which is standard in modern Android development.
