# Safety-Critical HMI Guidelines

SmartFlow manages physical hardware (water pumps) that can cause property damage or hardware destruction (burnout) if operated incorrectly. Therefore, our UI must adhere to industrial Human-Machine Interface (HMI) standards, heavily inspired by **ISA-101**.

## 1. Calm Normal State
- **Principle:** A system operating normally should be visually quiet.
- **Implementation:** When the pump is running normally and water levels are safe, the UI should use primarily muted blues and dark grays. Do not use bright, flashy colors to celebrate normal operation. The user should be able to look at the screen, see no red or amber, and immediately know everything is fine.

## 2. Actionable Alarms
- **Principle:** Every alarm must have a clear cause and an obvious path to resolution.
- **Implementation:** 
  - If a "Dry Run" occurs, do not just show "Error Code 404". 
  - Show: **"Dry Run Detected. Pump halted to prevent damage."**
  - Provide an action: **"Acknowledge & Reset"** (only if the user verifies water is restored).

## 3. Situational Awareness
- **Principle:** The operator must understand the context of the data.
- **Implementation:** 
  - Display trends, not just current values. A flow rate of "50 GPM" is less useful than a sparkline showing the flow rate has been steadily dropping over the last 10 minutes.
  - The `TelemetryChart` component exists specifically to provide this context.

## 4. Alarm Priority & Color Semantics
We use a strict three-tier alarm system:
- **Priority 1: Critical (Red - `#EF4444`)**
  - *Definition:* Imminent equipment damage or overflow. Requires immediate user intervention. System automatically halts.
  - *Examples:* Dry Run, Tank Overflow.
- **Priority 2: Warning (Amber - `#F59E0B`)**
  - *Definition:* Abnormal condition that may lead to a critical alarm if ignored.
  - *Examples:* Sensor data is stale (network dropout), Tank level below 10%.
- **Priority 3: Advisory (Blue - `#0EA5E9`)**
  - *Definition:* System generated notification, no immediate danger.
  - *Examples:* Scheduled pump start, Firmware update available.

## 5. Operator Feedback & Human Error Prevention
- **Feedback:** When a user sends a command (e.g., "Start Pump"), the UI must immediately reflect an "Actuating..." or "Pending" state, preventing the user from mashing the button repeatedly while waiting for the cloud/RS-485 round trip.
- **Error Prevention (Two-Step Actions):** Destructive or critical actions (Emergency Stop, Resetting Alarms, Overriding Auto Mode) must require a two-step confirmation (e.g., Tap to open dialog -> Hold to confirm).

## 6. Dashboard Information Architecture
*The Dashboard is the primary screen. It follows a strict visual hierarchy.*
1. **Global Status Banner (Top):** Only visible if there is a network error or critical alarm.
2. **Hero Metric (Center-Top):** The `PumpStatusCard`. The most critical piece of information: Is the pump on or off?
3. **Primary Telemetry (Middle):** `TankLevelCard`. The main driving variable for the pump.
4. **Secondary Telemetry (Bottom):** Flow rate charts, historical usage, settings.
