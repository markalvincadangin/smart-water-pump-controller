# Quickstart: App Design System Validation

**Feature**: 009-app-design-system

This guide describes how to validate that the Jetpack Compose Design System (Colors, Typography, Shapes, Splash Screen) has been correctly implemented.

## Prerequisites
- Android Studio with JDK 21
- Android device or emulator running API 31+ (Android 12+) to verify the Splash Screen

## Validation Steps

### 1. Build and Deploy
```bash
cd app
.\gradlew.bat assembleDebug
```
Deploy the generated APK to your emulator or device.

### 2. Splash Screen Verification
- **Action**: Launch the app from the home screen.
- **Expected Outcome**: You should briefly see the new standalone SmartFlow logo (droplet and arcs) centered on the screen, matching the brand background color.

### 3. Theme Application (Light/Dark Mode)
- **Action**: Navigate to the main dashboard.
- **Expected Outcome**:
  - The UI uses `SmartFlowTheme` (no default Material purple).
  - Background is `BackgroundLight` or `BackgroundDark` depending on system settings.
  - Buttons and interactive elements use `BluePrimary` (`#0EA5E9`) or `GreenSecondary` (`#10B981`).

### 4. Typography (Downloadable Fonts)
- **Action**: Inspect text elements on the screen.
- **Expected Outcome**: The primary typeface should clearly be **Inter**, and secondary/body text should be **Roboto**. Turn off network access and clear app data to verify the fallback system font works cleanly if fonts cannot be downloaded.

### 5. Dynamic Colors Verification
- **Action**: On an Android 12+ device, change the system wallpaper to a bright color (e.g., Pink). Relaunch the SmartFlow app.
- **Expected Outcome**: The app must **NOT** adopt the pink wallpaper colors. It must strictly retain the SmartFlow brand identity (Navy, Blue, Green).
