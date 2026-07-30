# Accessibility

SmartFlow is an industrial IoT application. It must be usable in harsh conditions, under stress, and by users with varying abilities.

## WCAG AA Standards
We adhere to WCAG 2.1 AA standards for all UI components.

## Touch Targets
- **Minimum Size:** All interactive elements (buttons, switches, icon buttons) MUST have a minimum touch target size of `48x48dp`.
- **Spacing:** There must be adequate spacing (at least `8dp`) between touch targets to prevent accidental mis-taps. This is especially critical for safety operations (e.g., placing the E-Stop button far away from the standard toggle).

## Contrast
- **Text:** All text must have a minimum contrast ratio of `4.5:1` against its background.
- **Large Text:** Text larger than 18pt (or 14pt bold) must have a ratio of `3.0:1`.
- **Non-Text Elements:** Icons and graphical objects (like gauge lines) must have a contrast ratio of `3.0:1`.
- *Note on Dark Mode:* The `primary` blue (`#0EA5E9`) against the `background` navy (`#0F172A`) has been verified to meet these standards.

## Screen Readers (TalkBack)
- Every image, icon, and visual gauge MUST have a `contentDescription`.
- **State Changes:** When the pump state changes (e.g., from Idle to Running), the app must use `Modifier.semantics { liveRegion = LiveRegionMode.Polite }` to announce the change to the user. Critical alarms should use `LiveRegionMode.Assertive`.
- **Custom Actions:** Complex cards should use custom accessibility actions rather than forcing the user to swipe through every single metric on the card.

## Scalable Typography
- The app must not hardcode heights on text containers.
- Users can scale their system fonts up to 200%. The UI must reflow gracefully (e.g., wrapping text to a new line or transitioning a Row to a Column) without truncating critical data.

## Color Blindness
- **Never rely on color alone.** 
- If a state is "Error" (Red), it must also be accompanied by a clear icon (e.g., a Warning Triangle) and explicit text ("ERROR: Dry Run"). 
- The difference between `secondary` Green and `error` Red can be difficult for some users to perceive; the accompanying iconography and text ensure the message is still delivered.
