# Component Library

The SmartFlow Component Library contains the reusable building blocks for the Android application.

## Core Components

### 1. StatusChip
- **Purpose:** To display the current high-level state of a system (Online, Offline, Error).
- **States:** 
  - *Success:* `secondary` background, `onSecondary` text.
  - *Warning:* `tertiary` background, `onTertiary` text.
  - *Error:* `error` background, `onError` text.
- **Behavior:** Static, non-clickable. Used as a visual anchor.
- **Design Rationale:** Needs to be immediately identifiable via color.

### 2. PumpStatusCard
- **Purpose:** The primary control interface for a pump.
- **Variants:** `Idle`, `Running`, `Error`, `Offline`.
- **States:** 
  - When `Running`, the card features a subtle pulse animation or an animated waveform graphic using the Primary Blue.
  - When `Error`, the card border is highlighted in `error` red, and the primary action button changes to "Acknowledge" or "Reset".
- **Behavior:** Tapping the card expands it for detailed telemetry. The card contains a primary switch or FAB for manual control.
- **Accessibility:** Must announce state changes (e.g., "Pump is now running") to screen readers.

### 3. TankLevelCard
- **Purpose:** Visualizes ultrasonic sensor data.
- **Variants:** Circular Gauge or Vertical Fill Bar.
- **States:** Fill level animates smoothly to changes. Color shifts from Primary Blue to Warning Amber if level drops below 20%.
- **Behavior:** Non-interactive, purely data visualization.
- **Design Rationale:** Users process visual fill levels faster than raw numbers. The `tnum` font must be used for the percentage text.

### 4. ConnectionBanner
- **Purpose:** Indicates when the app is disconnected from the Cloud or operating in local-only (Bluetooth/LAN) mode.
- **States:** Hidden when fully online. Appears as a top-anchored banner when state degrades.
- **Behavior:** Pushes content down (does not overlay). 
- **Design Rationale:** Connectivity is critical in IoT. The user must always know if they are looking at stale data.

### 5. EmergencyStopDialog
- **Purpose:** The highest priority interaction. Immediately cuts power to the relay.
- **States:** Triggered by hardware constraints or manual user action.
- **Behavior:** Modal dialog. Dims the background by 60%. Uses `error` red heavily. Requires explicit confirmation (e.g., "Slide to Stop" or a very clear "CONFIRM STOP" button) to prevent accidental triggers, but must not be overly complex to execute.

## Iconography
- **Family:** Material Symbols (Rounded variant).
- **Usage Rules:** 
  - Use *Filled* icons for active states or selected navigation items.
  - Use *Outlined* icons for inactive states or secondary actions.
- **Safety Icons:** Ensure standard recognizable symbols are used (e.g., a standard triangle with an exclamation mark for warnings). Do not reinvent safety iconography.
