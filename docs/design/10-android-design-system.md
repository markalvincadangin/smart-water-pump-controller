# Android Design System

SmartFlow is implemented with Jetpack Compose and Material 3 using semantic tokens.

## Theme rule

Do not hardcode brand or state colors inside components.

```kotlin
Modifier.background(MaterialTheme.colorScheme.primary)
```

Do not use:

```kotlin
Modifier.background(Color(0xFF0EA5E9))
```

Exceptions are limited to token-definition files.

## Theme strategy

- Dark theme is the operational default.
- Light theme is fully supported.
- Dynamic color is disabled for the core HMI because system status colors must remain stable.
- User theme preference may change surface mode, not alarm semantics.

## Edge-to-edge

- Draw behind system bars.
- Apply safe insets to tappable and readable content.
- Keep emergency controls outside gesture and cutout interference.
- Place snackbars above navigation.

## State architecture

Compose UI should render from an explicit immutable state model.

Recommended command state:

```kotlin
sealed interface CommandState {
    data object Ready : CommandState
    data object Pending : CommandState
    data object Accepted : CommandState
    data object Completed : CommandState
    data class Rejected(val reason: String) : CommandState
    data object TimedOut : CommandState
    data object OfflineBlocked : CommandState
}
```

The UI must not infer hardware completion from a successful network request alone.

## Adaptive icons

Provide:

- `ic_launcher_foreground.xml`
- `ic_launcher_background.xml`
- `ic_launcher_monochrome.xml`
- `ic_launcher.xml`
- `ic_launcher_round.xml`

Foreground artwork uses the approved icon and remains within the safe zone. Background uses solid `#0F172A`.

## Splash screen

Use the platform splash-screen API.

- background: `#0F172A`
- icon: approved SmartFlow icon
- no wordmark at small sizes
- transition: short crossfade into the real dashboard
- do not delay app readiness for decorative animation

## Responsive Compose layouts

- Compact: bottom navigation, single column
- Medium: navigation rail, two-column content where useful
- Expanded: persistent navigation and multi-column dashboard

Use window size and available space, not device-name detection.

## Accessibility implementation

- minimum 48dp touch targets
- semantics for name, role, value, and state
- polite live region for routine state changes
- assertive live region for critical alarms
- support 200% font scale
- reduced-motion behavior
- no fixed text heights

## Loading, stale, and offline

- Initial load: skeleton structure
- Network request: progress state with clear label
- Stale telemetry: last known value plus timestamp
- Offline: explicit control capability and fail-safe status
- Retry: automatic backoff plus user-visible retry where useful

## Component previews and tests

Every component should include previews for:

- dark and light
- normal, warning, critical, offline
- large text
- narrow width
- TalkBack semantics
- reduced motion
