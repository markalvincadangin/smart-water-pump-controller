# Layout System

SmartFlow uses a structured 8dp grid with a 4dp sub-grid.

## Spacing tokens

| Token | Value | Typical use |
|---|---:|---|
| `space.0` | 0dp | No separation |
| `space.0_5` | 4dp | Icon-to-label, tightly related values |
| `space.1` | 8dp | Small internal gap |
| `space.1_5` | 12dp | Compact component padding |
| `space.2` | 16dp | Mobile edge margin, standard card padding |
| `space.3` | 24dp | Section separation |
| `space.4` | 32dp | Major content separation |
| `space.6` | 48dp | Large empty state spacing |
| `space.8` | 64dp | Hero or wide-layout spacing |

Use 4dp only as a sub-grid. Primary alignment should fall on 8dp increments.

## Screen margins

- Compact mobile: 16dp
- Comfortable mobile / landscape: 24dp
- Tablet and expanded layouts: 32dp
- Large dashboard content should use a centered maximum-width container.

## Shape tokens

| Token | Value | Use |
|---|---:|---|
| `radius.xs` | 4dp | Small badges, tooltips |
| `radius.sm` | 8dp | Inputs, compact buttons |
| `radius.md` | 16dp | Standard cards |
| `radius.lg` | 24dp | Sheets and large dialogs |
| `radius.full` | 999dp | Pills and circular controls |

Corner radius must not imply interactivity on a non-interactive data surface.

## Tonal elevation

| Level | Intended use |
|---|---|
| 0 | Background canvas |
| 1 | Resting card |
| 2 | Selected or scrolling top app bar |
| 3 | Navigation and bottom sheet |
| 4 | Dialog |
| 5 | Highest modal layer |

Dark-mode elevation is expressed primarily through surface tone, not shadow.

## Responsive classes

| Class | Width | Pattern |
|---|---:|---|
| Compact | `<600dp` | Single-column dashboard |
| Medium | `600–839dp` | Two-column cards or navigation rail |
| Expanded | `≥840dp` | Multi-column dashboard, persistent navigation |

These are project layout thresholds and may be tuned after device testing.

## Dashboard hierarchy

1. Global alarm or connection banner
2. Pump status and primary command
3. Tank level and primary telemetry
4. Trends and secondary telemetry
5. History, settings, and advanced diagnostics

Do not let low-priority charts displace the current pump state.

## Safe areas and edge-to-edge

- Draw backgrounds edge-to-edge.
- Apply insets to content that must remain readable or tappable.
- Never place emergency controls under a system gesture zone.
- Keep snackbars above navigation.
- Validate display cutouts and landscape rotation.

## Density

Normal operation should remain calm and scan-friendly.

- Use cards for operational groups, not every individual value.
- Combine related label/value/unit data.
- Use progressive disclosure for raw sensor values.
- Fleet and enterprise layouts may use tables, but critical state remains visually prioritized.
