# Motion System

Animation in SmartFlow is functional, not decorative. It serves to draw attention to state changes, indicate system activity, and provide reassurance.

## Animation Principles
1. **Purposeful:** Every animation must explain a change in state or relationship.
2. **Snappy but Smooth:** Industrial systems require fast responses. UI animations should be quick (200-300ms) but use smooth easing curves (Standard Material Easing: Fast out, Slow in).
3. **Non-Blocking:** Animations must never prevent the user from interacting with critical controls (like an E-Stop).

## Core Motion States

### 1. Idle
- **Visuals:** Static. No motion.
- **Rationale:** An idle pump is safe. Motion implies activity. Do not animate things unnecessarily.

### 2. Running (Active Flow)
- **Visuals:** Subtle, continuous motion. Examples include a slow, infinite panning of a wave SVG inside the `TankLevelCard`, or a slow pulse (alpha 0.8 to 1.0) on the `PumpStatusCard`'s active badge.
- **Rationale:** Immediately communicates to the operator that hardware is physically moving/pumping without requiring them to read text.

### 3. Loading / Connecting
- **Visuals:** Indeterminate circular progress indicators.
- **Rationale:** Use the standard Material loading spinner to indicate network requests (e.g., polling Firebase).

### 4. Emergency / Alarm
- **Visuals:** Rapid pulsing or flashing. If a critical hardware alarm triggers (e.g., Dry Run), the `error` colored elements should pulse rapidly (e.g., 2 times per second) to draw immediate visual attention.
- **Rationale:** Alarms must break through visual noise.

## Transitions
- **Navigating:** Use standard Compose crossfades or slide-in transitions between main dashboard tabs.
- **Expanding Cards:** When a user taps a card to see more details, use a `SharedTransition` or `animateContentSize` to smoothly grow the card, maintaining the user's spatial awareness of where the data came from.

## Reduced Motion
- **Accessibility Requirement:** The app MUST respect the Android system-level "Remove animations" setting. 
- **Implementation:** When reduced motion is enabled, infinite animations (running waves, pulsing alarms) must be replaced with static states. Transitions should instantly snap rather than tween.
