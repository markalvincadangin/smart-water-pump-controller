# Color System

Our color system is derived from our brand palette and adapted into a comprehensive Material Design 3 (M3) ColorScheme. Every color serves a functional purpose, mapping brand identity to UI semantics.

## Core Brand Colors
- **Primary Blue:** `#0EA5E9` (Water, active states, primary actions, normal telemetry)
- **Secondary Green:** `#10B981` (Success, online status, safe conditions)
- **Background / Navy:** `#0F172A` (Industrial canvas, app background)
- **Accent White:** `#FFFFFF` (High-emphasis text, clear contrast)

---

## Material 3 ColorScheme Mapping

### Primary & Secondary (Brand & Accent)
- **`primary`:** `#0EA5E9`
  - *Usage:* Primary buttons, active FABs, active tab indicators, progress bars (water level).
- **`onPrimary`:** `#FFFFFF`
  - *Usage:* Text and icons displayed on top of `primary` elements.
- **`primaryContainer`:** `#004B71` (Dark Theme) / `#BAE6FD` (Light Theme)
  - *Usage:* Large interactive surfaces, selected cards.
- **`onPrimaryContainer`:** `#E0F2FE` (Dark Theme) / `#00314A` (Light Theme)
  - *Usage:* Text and icons on `primaryContainer`.
- **`secondary`:** `#10B981`
  - *Usage:* Online status indicators, successful connections, "System Healthy" badges.
- **`onSecondary`:** `#FFFFFF`
- **`secondaryContainer`:** `#064E3B` (Dark) / `#D1FAE5` (Light)
- **`onSecondaryContainer`:** `#A7F3D0` (Dark) / `#065F46` (Light)

### Semantic & Status (Safety Critical)
Following ISA-101, alarms must be distinct.
- **`error` (Critical/Emergency):** `#EF4444` (Red)
  - *Usage:* Dry-run triggers, overflow alarms, emergency stop buttons.
- **`onError`:** `#FFFFFF`
- **`errorContainer`:** `#7F1D1D` (Dark) / `#FEE2E2` (Light)
- **`onErrorContainer`:** `#FECACA` (Dark) / `#991B1B` (Light)
- **`tertiary` (Warning/Attention):** `#F59E0B` (Amber/Yellow)
  - *Usage:* Sensor stale warnings, connection retrying, low water (but not empty).
- **`onTertiary`:** `#451A03` (Dark) / `#FFFFFF` (Light)

### Surfaces & Backgrounds
- **`background`:** `#0F172A` (Dark) / `#F8FAFC` (Light)
  - *Usage:* The lowest layer of the app. The canvas.
- **`onBackground`:** `#F1F5F9` (Dark) / `#0F172A` (Light)
  - *Usage:* Primary text on the background.
- **`surface`:** `#1E293B` (Dark) / `#FFFFFF` (Light)
  - *Usage:* Standard cards, dialogs, bottom sheets. Elevated above background.
- **`onSurface`:** `#F1F5F9` (Dark) / `#0F172A` (Light)
- **`surfaceVariant`:** `#334155` (Dark) / `#F1F5F9` (Light)
  - *Usage:* Dividers, borders, subtle secondary backgrounds (like a disabled button or unselected chip).
- **`onSurfaceVariant`:** `#94A3B8` (Dark) / `#475569` (Light)
  - *Usage:* Secondary text, helper text, inactive icons.
- **`outline`:** `#475569` (Dark) / `#CBD5E1` (Light)
  - *Usage:* Outlined buttons, card borders, text field borders.

### Inverse Colors
- **`inverseSurface`:** `#F8FAFC` (Dark) / `#0F172A` (Light)
  - *Usage:* Snackbars and temporary alerts that need to heavily contrast with the environment.
- **`inverseOnSurface`:** `#0F172A` (Dark) / `#F1F5F9` (Light)
- **`inversePrimary`:** `#7DD3FC` (Dark) / `#0284C7` (Light)

## Application Rules
1. **Never use `error` (Red) for decorative purposes.** Red is strictly reserved for critical hardware alarms (Dry run, Overflow, E-Stop).
2. **Never use `secondary` (Green) as a generic background.** Green means "System Normal / Online".
3. **Use `surface` for elevation.** In dark mode, do not use drop shadows. Instead, lighten the surface color slightly using M3 tonal elevation overlays.
