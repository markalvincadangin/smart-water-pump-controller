# Quickstart: Validating the App Design System

To validate the newly implemented SmartFlow design system in the Android App:

## Prerequisites
- Android Studio or command-line Gradle.
- A physical Android device or an emulator running API 24 or higher.

## Setup & Run

1. **Build and Install**:
   Ensure you are in the root directory, then run the Gradle task to build and install the debug APK on your connected device/emulator:
   ```bash
   cd app
   ..\gradlew.bat installDebug
   ```

2. **Launch the App**:
   Open the SmartFlow app on your device.

## Validation Scenarios

### Scenario 1: Verify Theme Colors
- Check the top app bar, buttons, and backgrounds.
- The Primary color should be the SmartFlow Blue (`#0EA5E9`).
- Status indicators or secondary elements should use Green (`#10B981`).

### Scenario 2: Verify Typography
- Check the text rendering.
- Headings and body text should render cleanly in the **Inter** typeface.
- Secondary elements or specific labels may use **Roboto**.

### Scenario 3: Verify Dark Mode Support
- Go to your device's System Settings -> Display -> Enable **Dark Theme**.
- Return to the SmartFlow app.
- The app should seamlessly transition to a dark background (`#0F172A`) with legible white/light text and correctly contrasting buttons.

If all scenarios pass, the implementation successfully meets the feature specification.
