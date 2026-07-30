# Motion System

Motion communicates state and causality. It is never required to understand a critical condition.

## Principles

1. Purposeful
2. Fast and readable
3. Non-blocking
4. Interruptible
5. Reduced-motion compatible
6. Safe for repeated exposure

## Duration tokens

| Token | Duration | Use |
|---|---:|---|
| `motion.instant` | 0ms | Reduced motion, immediate state change |
| `motion.fast` | 100ms | Press and selection feedback |
| `motion.standard` | 200ms | Component state transition |
| `motion.emphasized` | 300ms | Card expansion and navigation |
| `motion.slow` | 500ms | Large visual data transition |

Avoid transitions longer than 500ms in operational flows.

## Easing

- Standard: fast-out, slow-in
- Exit: fast-out
- Entrance: slow-in
- Linear: continuous physical flow only

## State behavior

### Idle

Static. Do not animate normal inactivity.

### Running

Use restrained motion:

- slow wave translation
- subtle progress movement
- low-amplitude badge breathing only when useful

Running motion must not resemble an alarm.

### Loading and connecting

Use standard progress treatment with supporting text. For initial dashboard loading, prefer structural skeletons. For stale data, show the last known value rather than a spinner.

### Commands

A command transition should show:

`Pressed → Pending → Accepted → Completed`

The transition must remain interruptible if the system reports rejection, timeout, or alarm.

### Critical alarms

Do not use continuous rapid flashing.

Use:

- persistent high-contrast alarm color
- clear icon
- cause and consequence text
- immediate action
- optional one-time entrance pulse
- haptic or audio alert when available and user-configured

## Data animation

- Animate between confirmed samples.
- Do not interpolate across missing data.
- Do not use easing that makes telemetry appear delayed or overshoot.
- Respect timestamps and sample cadence.

## Reduced motion

When reduced motion is enabled:

- remove infinite loops
- replace animated waves with static state
- disable parallax and shared-element motion
- keep critical state changes immediate
- preserve haptic/audio alerts according to user settings

Motion is supplemental; text, iconography, and structure remain complete without it.
