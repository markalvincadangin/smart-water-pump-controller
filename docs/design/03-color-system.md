# Color System

SmartFlow uses color as operational language. Brand color and system status must remain distinct and predictable.

## Core brand colors

| Token | Value | Meaning |
|---|---|---|
| `brand.blue` | `#0EA5E9` | Water, active control, normal telemetry |
| `brand.green` | `#10B981` | Online, healthy, successful |
| `brand.navy` | `#0F172A` | Industrial canvas and dark background |
| `brand.white` | `#FFFFFF` | High-emphasis light foreground |

## Accessibility correction

The approved brand colors remain unchanged, but foreground colors must be chosen for contrast:

| Background | Approved foreground | Approx. contrast |
|---|---|---:|
| `#0EA5E9` | `#0F172A` | 6.44:1 |
| `#10B981` | `#0F172A` | 7.04:1 |
| `#F59E0B` | `#451A03` | 6.97:1 |
| `#DC2626` | `#FFFFFF` | 4.83:1 |

Do not use white normal-sized text directly on `#0EA5E9` or `#10B981`.

## Semantic colors

| Role | Value | Use |
|---|---|---|
| `status.criticalAccent` | `#EF4444` | Alarm icons, borders, charts, attention marks |
| `status.criticalAction` | `#DC2626` | Filled destructive and emergency actions |
| `status.warning` | `#F59E0B` | Degraded or abnormal condition |
| `status.advisory` | `#0EA5E9` | Informational system event |
| `status.success` | `#10B981` | Confirmed healthy or online state |

### Rules

1. Red is reserved for critical conditions and destructive actions.
2. Amber means degraded or attention-required, not failure.
3. Green means confirmed healthy, connected, or completed.
4. Blue means normal operation, active control, or advisory information.
5. Never rely on color alone; pair it with text and iconography.

## Material 3 role mapping

### Shared roles

| Role | Value |
|---|---|
| `primary` | `#0EA5E9` |
| `onPrimary` | `#0F172A` |
| `secondary` | `#10B981` |
| `onSecondary` | `#0F172A` |
| `tertiary` | `#F59E0B` |
| `onTertiary` | `#451A03` |
| `error` | `#DC2626` |
| `onError` | `#FFFFFF` |

### Dark scheme

| Role | Value |
|---|---|
| `background` | `#0F172A` |
| `onBackground` | `#F1F5F9` |
| `surface` | `#111C31` |
| `surfaceContainerLow` | `#162238` |
| `surfaceContainer` | `#1E293B` |
| `surfaceContainerHigh` | `#273449` |
| `surfaceContainerHighest` | `#334155` |
| `onSurface` | `#F1F5F9` |
| `onSurfaceVariant` | `#CBD5E1` |
| `outline` | `#64748B` |
| `outlineVariant` | `#334155` |
| `primaryContainer` | `#0C4A6E` |
| `onPrimaryContainer` | `#E0F2FE` |
| `secondaryContainer` | `#064E3B` |
| `onSecondaryContainer` | `#D1FAE5` |
| `errorContainer` | `#7F1D1D` |
| `onErrorContainer` | `#FEE2E2` |
| `inverseSurface` | `#F8FAFC` |
| `inverseOnSurface` | `#0F172A` |

### Light scheme

| Role | Value |
|---|---|
| `background` | `#F8FAFC` |
| `onBackground` | `#0F172A` |
| `surface` | `#FFFFFF` |
| `surfaceContainerLow` | `#F8FAFC` |
| `surfaceContainer` | `#F1F5F9` |
| `surfaceContainerHigh` | `#E2E8F0` |
| `surfaceContainerHighest` | `#CBD5E1` |
| `onSurface` | `#0F172A` |
| `onSurfaceVariant` | `#475569` |
| `outline` | `#64748B` |
| `outlineVariant` | `#CBD5E1` |
| `primaryContainer` | `#BAE6FD` |
| `onPrimaryContainer` | `#082F49` |
| `secondaryContainer` | `#D1FAE5` |
| `onSecondaryContainer` | `#064E3B` |
| `errorContainer` | `#FEE2E2` |
| `onErrorContainer` | `#7F1D1D` |
| `inverseSurface` | `#0F172A` |
| `inverseOnSurface` | `#F8FAFC` |

## Data visualization

Use color consistently across charts:

- Water level / flow: Primary blue
- Healthy target band: Muted green
- Warning threshold: Amber
- Critical threshold: Red
- Historical comparison: Slate
- Missing data: dashed outline plus “No data” label

Charts must remain understandable in grayscale and for color-vision deficiencies.

## Elevation

In dark mode, use tonal elevation rather than heavy shadow. Higher surfaces become slightly lighter and may receive a very subtle primary tint.

Do not use glow as an elevation substitute.
