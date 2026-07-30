# Android Design System

The SmartFlow Design System is built specifically for implementation in **Jetpack Compose**, leveraging **Material Design 3 (M3)**.

## Material 3 Foundation
Our theme replaces the default `MaterialTheme` parameters. Developers should not use hardcoded hex values in Compose modifiers.
- **DO:** `Modifier.background(MaterialTheme.colorScheme.primary)`
- **DON'T:** `Modifier.background(Color(0xFF0EA5E9))`

## Edge-to-Edge Design
- The app must draw behind the system bars to provide a modern, immersive experience.
- Use `WindowCompat.setDecorFitsSystemWindows(window, false)` in the `MainActivity`.
- Use Compose's `WindowInsets` to add padding to content that must not be obscured by the navigation bar or status bar (e.g., `Modifier.safeDrawingPadding()`).

## Android Adaptive Icons
- The app icon must be provided as an Adaptive Icon (`ic_launcher.xml` and `ic_launcher_round.xml`).
- It consists of two layers:
  1. **Background:** A solid `#0F172A` vector shape.
  2. **Foreground:** The SmartFlow droplet and signal arcs, scaled to fit safely within the 66dp central safe zone.
- **Monochrome Theme:** The app must provide a monochromatic vector for Android 13+ themed icons, ensuring it looks correct on user's custom home screens.

## SplashScreen API
- The app must implement the Android 12+ SplashScreen API (`androidx.core:core-splashscreen`).
- **Configuration:**
  - `windowSplashScreenBackground`: `#0F172A`
  - `windowSplashScreenAnimatedIcon`: The SmartFlow icon.
  - The exit animation should cleanly crossfade into the Compose dashboard.

## Dynamic Color Compatibility
- Android 12 introduces "Material You" (Dynamic Color), which extracts theme colors from the user's wallpaper.
- **SmartFlow Rule:** Because we are a safety-critical HMI, **we DO NOT use Dynamic Color for the core dashboard.** Using a user's pastel pink wallpaper theme could compromise the legibility of critical alarms or confuse the established color semantics (where Green means Online and Red means Error). 
- We strictly enforce our custom brand `ColorScheme` regardless of the user's OS wallpaper settings.

## UX Principles & Android Best Practices
- **Progressive Disclosure:** Hide advanced telemetry (like raw sensor voltages) behind an "Advanced Details" sheet. Keep the main dashboard clean.
- **Skeleton Screens:** When loading data from Firebase, do not show a blank screen or a simple spinner. Show a skeleton outline of the `PumpStatusCard` to reduce perceived latency.
- **Retry Behavior:** Network failures should present a clear "Retry" button or automatically poll with an exponential backoff.
- **Snackbar Placement:** Snackbars (`inverseSurface`) must float *above* the Bottom Navigation Bar, not behind it.
