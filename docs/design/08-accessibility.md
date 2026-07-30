# Accessibility

SmartFlow must remain usable in bright light, dark pump rooms, noisy environments, stressful incidents, and with assistive technology.

## Standard

Target WCAG AA behavior for visual contrast, text resizing, focus, and non-text controls.

## Touch and pointer targets

- Minimum touch target: 48×48dp
- Minimum spacing between independent critical controls: 8dp
- Keep Emergency Stop separated from routine controls
- Support keyboard and switch access on large-screen devices
- Visible focus indication is required

## Contrast

- Normal text: at least 4.5:1
- Large text: at least 3:1
- Functional icons and graphical controls: at least 3:1
- Do not place white normal text on brand blue or brand green
- Test disabled states; reduced opacity must not make essential text unreadable

## Screen readers

Every functional control must expose:

- name
- role
- value
- state
- available action

### Live regions

- Routine pump state changes: polite
- Critical alarms: assertive
- Repeated telemetry updates: do not announce continuously
- Charts: provide a concise text summary and key thresholds

## Custom controls

Complex cards should expose grouped semantics and custom actions. Do not require users to navigate every decorative value.

## Text scaling

- Support up to 200% system font scaling.
- Avoid fixed-height text containers.
- Preserve alarm cause, consequence, and action.
- Allow horizontal rows to reflow vertically.
- Avoid ellipsizing critical messages.

## Color vision

Never rely on red, amber, green, or blue alone.

Pair status with:

- standard icon
- explicit text
- shape or structure
- location consistency

## Motion and sensory alternatives

- Respect reduced-motion settings.
- Do not use continuous flashing.
- Provide visual alternatives to sound.
- Provide non-audio alternatives to haptics.
- User-configurable alarm sound must not replace the visual alarm state.

## Language and cognitive load

- Use direct sentences.
- State one cause and one next action.
- Avoid unexplained codes.
- Preserve familiar placement of critical controls.
- Confirm destructive actions without unnecessary complexity.

## Accessibility test matrix

Before release, test:

- TalkBack
- keyboard / D-pad navigation
- 200% font scale
- dark and light themes
- grayscale
- common color-vision simulations
- reduced motion
- landscape
- smallest supported screen
- glare / low-brightness conditions
