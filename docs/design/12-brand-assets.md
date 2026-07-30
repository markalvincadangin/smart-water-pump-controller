# Brand Assets and Governance

## Required master assets

### Vector

- `smartflow-logo-horizontal.svg`
- `smartflow-logo-stacked.svg`
- `smartflow-icon.svg`
- `smartflow-wordmark.svg`
- `smartflow-logo-monochrome.svg`
- `smartflow-icon-monochrome.svg`

### Raster

- horizontal light, dark, white, black
- stacked light, dark, white, black
- icon full color, white, black
- wordmark light and dark
- Android adaptive foreground and background
- Play Store feature graphic
- iOS app icon
- favicon sizes
- splash asset

## Source-of-truth rule

All exports must be produced from approved vector masters.

Do not:

- use an AI-generated preview as a production master
- upscale a PNG
- combine pieces from mismatched revisions
- recreate the wordmark with a font
- manually recolor individual exports without updating the master

## File naming

Use lowercase kebab case for brand exports.

Examples:

- `smartflow-logo-horizontal-dark.png`
- `smartflow-logo-stacked-light.svg`
- `smartflow-icon-monochrome.svg`
- `ic-launcher-foreground.png`

Platform-required Android resource names may use underscores.

## Versioning

Store each approved release under a versioned directory.

```text
brand/
  v1.1.0/
    vector/
    png/
    android/
    ios/
    web/
    print/
```

Do not overwrite a released master without incrementing the version.

## Export matrix

| Use | Format | Color space | Background |
|---|---|---|---|
| Web/UI | SVG, PNG | sRGB | Transparent where specified |
| Android | VectorDrawable, PNG | sRGB | Layer-specific |
| iOS | PNG | sRGB | Solid |
| Print | PDF/SVG | Print-managed | As specified |
| Favicon | SVG/PNG/ICO | sRGB | Transparent or approved solid |

## Asset QA checklist

- correct approved source
- correct revision
- exact canvas size
- correct aspect ratio
- correct lockup
- balanced optical margins
- no clipping
- clean transparency
- correct theme treatment
- negative spaces preserved
- no glow or shadow
- filename matches inventory
- preview tested on intended background
- checksum recorded in manifest

## Font governance

Inter and Roboto are implementation dependencies, not brand assets to redistribute casually.

Keep font files only in authorized project repositories and comply with their licenses. Do not package font binaries with general brand-download bundles unless licensing and distribution have been reviewed.

## Roadmap

The design system can extend to:

- web dashboard
- tablet and industrial kiosk
- Wear OS
- enterprise fleet management
- documentation and support portal

Each platform may adapt layout density, but safety semantics, alarm priorities, logo integrity, and core color meaning remain unchanged.
