# Logo Guidelines

## Approved artwork status

The approved SmartFlow logo is immutable brand artwork.

Do not redraw, reinterpret, regenerate, re-typeset, or approximate it. Production assets must be exported from the approved master artwork.

Allowed production refinements are limited to:

- optical alignment
- spacing and kerning
- vector precision
- consistent stroke widths
- pixel-grid alignment
- gradient consistency
- adaptive-icon safe-zone placement
- export quality

## Logo anatomy

1. **Droplet:** Water and the controlled resource
2. **Flowing ring:** Continuous circulation and automation
3. **Signal arcs:** Connectivity, telemetry, and remote monitoring
4. **Wordmark:** The approved “SmartFlow” artwork; it is not recreated with a UI font

## Approved lockups

- **Primary horizontal:** Icon left, wordmark right
- **Stacked:** Icon centered above wordmark
- **Icon only:** App icons, favicons, avatars, compact UI
- **Wordmark only:** Headers and constrained layouts where the symbol is redundant

Do not create unapproved arrangements.

## Clear space

Define `X` as the width of the central droplet.

Maintain at least `1X` clear space on all sides of:

- the icon
- the horizontal lockup
- the stacked lockup
- the wordmark

The clear-space area must not contain text, borders, photographs, control indicators, or container edges.

## Minimum size

| Asset | Recommended minimum | Absolute minimum | Notes |
|---|---:|---:|---|
| Full icon | 24px | 16px | At 16px use the simplified favicon export |
| Horizontal logo | 96px wide | 48px wide | Use icon-only when the wordmark becomes unclear |
| Stacked logo | 72px wide | 56px wide | Avoid for dense toolbars |
| Print icon | 0.25in | — | Verify output device |
| Print horizontal | 0.75in wide | — | Use vector artwork |

## Theme treatments

### Light background

- Full-color icon
- `Smart` in dark navy
- `Flow` in approved blue
- Background: white or approved light neutral

### Dark background

- Full-color icon
- `Smart` in white or approved light neutral
- `Flow` in approved blue
- Background: `#0F172A` or another approved dark neutral

### Monochrome

- Entire mark in solid white on dark
- Entire mark in solid black on light
- Preserve all negative spaces
- Do not outline the artwork

## Export requirements

- SVG is the master interchange format.
- PNG exports must use true transparency when specified.
- Do not add a matte color around transparent edges.
- Preserve sRGB color for screen assets.
- Use vector PDF for print.
- Export at integer dimensions and inspect at 100% scale.
- Do not upscale a raster logo to create a larger master.

## App icon rules

### Android adaptive icon

- Foreground and background are separate layers.
- Foreground uses the approved icon only.
- Keep critical artwork inside the central safe zone.
- Background is solid `#0F172A` unless an approved light variant is required.
- Supply a monochrome vector for Android themed icons.

### iOS app icon

- No transparency
- No pre-applied rounded corners
- Use a solid approved background
- Keep the icon optically centered

### Favicon

- Use a simplified icon export at 16px and 32px.
- Prioritize silhouette and spacing over gradient detail.
- Do not substitute a generic droplet.

## Incorrect usage

Never:

1. Stretch, skew, rotate, or distort the mark.
2. Recolor outside the approved system.
3. change the approved silhouette or proportions.
4. add glow, drop shadow, bevel, or 3D effects.
5. place on a busy image without an approved contrast field.
6. alter icon-to-wordmark spacing.
7. outline the logo.
8. remove required negative spaces.
9. recreate the wordmark with Inter, Roboto, or another font.
10. combine pieces from different logo revisions.

## Production QA

Before release, verify:

- correct source master
- correct lockup
- correct theme treatment
- exact canvas dimensions
- transparent or solid background as specified
- balanced margins
- no clipping under app-icon masks
- correct filename and version
- visual match against the approved reference
