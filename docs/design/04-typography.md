# Typography

## Brand wordmark

The official SmartFlow wordmark is approved artwork.

- Do not re-typeset it.
- Do not approximate it with Inter, Roboto, or another font.
- Use the exported wordmark or approved logo lockup.

## Product typefaces

### Primary UI typeface: Inter

Use for:

- navigation
- buttons
- controls
- telemetry
- data tables
- headings
- labels

Inter's clear numeral and character forms support fast scanning of operational data.

### Secondary typeface: Roboto

Use for:

- long-form body copy
- documentation
- Android-native fallback
- legal or explanatory text

Avoid mixing the two typefaces within one short component.

## Material 3 type scale

| Role | Font | Weight | Size | Line height | Tracking |
|---|---|---:|---:|---:|---:|
| Display Large | Inter | 700 | 57sp | 64sp | -0.25sp |
| Display Medium | Inter | 600 | 45sp | 52sp | 0sp |
| Display Small | Inter | 500 | 36sp | 44sp | 0sp |
| Headline Large | Inter | 600 | 32sp | 40sp | 0sp |
| Headline Medium | Inter | 500 | 28sp | 36sp | 0sp |
| Headline Small | Inter | 500 | 24sp | 32sp | 0sp |
| Title Large | Inter | 500 | 22sp | 28sp | 0sp |
| Title Medium | Inter | 500 | 16sp | 24sp | 0.15sp |
| Title Small | Inter | 500 | 14sp | 20sp | 0.1sp |
| Body Large | Roboto | 400 | 16sp | 24sp | 0.5sp |
| Body Medium | Roboto | 400 | 14sp | 20sp | 0.25sp |
| Body Small | Roboto | 400 | 12sp | 16sp | 0.4sp |
| Label Large | Inter | 500 | 14sp | 20sp | 0.1sp |
| Label Medium | Inter | 500 | 12sp | 16sp | 0.5sp |
| Label Small | Inter | 500 | 11sp | 16sp | 0.5sp |

## Telemetry and units

- Enable tabular figures for changing values.
- Keep the number and unit visually grouped.
- Use a non-breaking space between values and units when supported: `128 GPM`.
- Do not use color as the only indicator of a threshold breach.
- Use consistent decimal precision:
  - flow: project-defined precision
  - pressure: project-defined precision
  - level: whole percentage unless extra precision is actionable
- Show the measurement timestamp when data may be stale.

## Hierarchy rules

1. One hero metric per primary card.
2. Units must be smaller than the measured value but remain legible.
3. Labels describe meaning; values show state.
4. Avoid all caps for sentences and alarm explanations.
5. Reserve uppercase for short safety actions such as `EMERGENCY STOP`.
6. Do not place critical text below 12sp.
7. Do not truncate alarm cause or required action.

## Scaling and reflow

- Support system font scaling to 200%.
- Do not hardcode text-container height.
- Rows containing critical text must be allowed to become columns.
- Preserve command controls and alarm actions when text expands.
- Test at compact width, landscape, and large-screen layouts.
