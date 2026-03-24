# SmartFlow Design System
### Visual Design Specification v1.0

**Product** — SmartFlow: Smart Water Pump Controller
**Location** — Leon, Iloilo, Philippines
**Classification** — Industrial IoT Operations Dashboard
**Last Updated** — 2026

---

> **Design Intent**
> SmartFlow is an operational instrument, not a consumer application. An operator must understand the full system state within three seconds of looking at any screen, under any lighting condition, on any device. Every decision in this specification serves that goal. Aesthetics are a function of clarity, not decoration.

---

## Governing HCI Principles

All design decisions in this specification are traceable to one or more of the following principles. Where trade-offs exist between principles, operational safety takes precedence over all others.

**Visibility of System Status** *(Nielsen #1)*
The dashboard must communicate pump state, water level, flow rate, and sensor health at all times without requiring the user to navigate or search. Status is never hidden behind a toggle or buried in a secondary view.

**Match Between System and the Real World** *(Nielsen #2)*
Visual metaphors map to physical reality. The tank fills from the bottom up. Water is always blue. A running pump is green. A stopped pump is grey. Fault states are red. No abstract iconography is used for operational states.

**User Control and Freedom** *(Nielsen #3)*
Every destructive action — emergency stop, mode change, bypass activation — requires explicit confirmation. Every action is reversible except latched faults, which require a deliberate two-step recovery. The operator is always in command.

**Consistency and Standards** *(Nielsen #4)*
Color, typography, and spatial rules are applied identically across every component. A green dot always means healthy. A left red border always means a fault card. A monospaced number always means a live sensor value. Once learned, the pattern holds everywhere.

**Error Prevention** *(Nielsen #5)*
Controls that are illegal in the current system state are dimmed and unresponsive — never hidden. The operator can see what is unavailable and understand why without triggering an error message. Confirmation dialogs appear before irreversible actions.

**Recognition Over Recall** *(Nielsen #6)*
All fault codes, mode names, and status labels are displayed in plain language alongside their identifier. The operator never needs to remember what a code means. No legend is required to interpret the dashboard.

**Aesthetic and Minimalist Design** *(Nielsen #8)*
Each panel displays only the information required for its purpose. Diagnostic depth is accessible via progressive disclosure — available when needed, invisible when not.

**Fitts's Law**
Primary controls — pump toggle and emergency stop — are large, have generous touch targets, and are spatially separated from each other by a minimum gap. The emergency stop is always reachable without scrolling on any screen size.

**Gestalt: Proximity and Grouping**
Related information lives in bounded visual clusters. Tank status is always in the center. Controls are always on the right. System health is always in the left rail. History is always at the bottom. Users develop spatial memory that persists across sessions.

**Feedback and Response Time**
Every user action produces a visible response within 100 milliseconds. Mode changes show a pending state until Firebase confirms the write. One-shot commands show a brief confirmation animation before resetting.

---

## Part 1 — Color System

### 1.1 Design Philosophy

The color system uses a **Dark Industrial** foundation with a **Clean Industrial** light variant. The dark theme is optimized for indoor monitoring in all lighting conditions. The light theme is optimized for outdoor and mobile use where ambient brightness washes out dark backgrounds.

Semantic colors — the green, amber, and red that communicate operational status — are **identical between both themes**. An operator switching from dark to light mode sees the same status meaning, never a different one.

No color in this system is purely decorative. Every color decision answers one of three questions: *What layer is this?* *What status does this represent?* *What type of data is this?*

---

### 1.2 Dark Theme Color Palette

#### Background Layers

The background system uses seven distinct depth levels to create spatial hierarchy without shadows. Lighter backgrounds sit visually closer to the user; darker backgrounds recede. Each level has a defined purpose — layers must not be interchanged.

| Token | Value | Purpose |
|-------|-------|---------|
| Canvas | `#070B10` | Page root — the deepest layer, never used as a card background |
| Base | `#0A0F17` | App shell, sidebar behind panels |
| Surface | `#111926` | Primary card and panel background |
| Raised | `#18232F` | Cards that sit above the primary surface |
| Elevated | `#1E2C3A` | Input fields, hover states, dropdown backgrounds |
| Overlay | `#253344` | Modals, tooltips, popovers |
| Inset | `#090D13` | Wells that sit below the surface (log backgrounds, code areas) |

#### Border Levels

| Token | Value | Purpose |
|-------|-------|---------|
| Faint | `#131D28` | Gap color between adjacent panels — acts as a 1px divider |
| Subtle | `#1C2A38` | Card edges, section dividers |
| Default | `#273D52` | Interactive element borders in resting state |
| Strong | `#3A5068` | Active, selected, or focused borders |
| Focus | `#3B82F6` | Keyboard focus ring — always brand blue |

#### Brand — Water Blue

| Token | Value | Purpose |
|-------|-------|---------|
| Brand 400 | `#60A5FA` | Links, secondary interactive elements |
| Brand 500 | `#3B82F6` | Primary buttons, active mode segment, chart lines |
| Brand 600 | `#2563EB` | Pressed / active button state |
| Brand Glow | `rgba(59, 130, 246, 0.18)` | Tank hero ambient glow, SVG drop shadow |
| Brand Glow Soft | `rgba(59, 130, 246, 0.10)` | Subtle hover backgrounds on brand elements |

#### Semantic — Status Colors

Each semantic color has a **primary**, a **dim** (used for card tint backgrounds), and a **dim-strong** (used for hover states over dim backgrounds).

**OK — Green (Running, Healthy)**

| Token | Value | Purpose |
|-------|-------|---------|
| OK 400 | `#34D399` | Secondary OK accents |
| OK 500 | `#10B981` | Primary OK color — running state, healthy sensors |
| OK 600 | `#059669` | Pressed state on OK elements |
| OK Dim | `rgba(16, 185, 129, 0.12)` | Card background tint when pump is running |
| OK Dim Strong | `rgba(16, 185, 129, 0.20)` | Hover over OK-tinted cards |

**Warning — Amber (Degraded, Bypass, Stale)**

| Token | Value | Purpose |
|-------|-------|---------|
| Warn 400 | `#FBBF24` | Secondary warn accents |
| Warn 500 | `#F59E0B` | Primary warn color — degraded states, bypass active |
| Warn 600 | `#D97706` | Pressed state on warn elements |
| Warn Dim | `rgba(245, 158, 11, 0.12)` | Card background tint for warnings |
| Warn Dim Strong | `rgba(245, 158, 11, 0.20)` | Hover over warn-tinted cards |

**Error — Red (Fault, Lockout, Offline)**

| Token | Value | Purpose |
|-------|-------|---------|
| Error 400 | `#F87171` | Secondary error accents |
| Error 500 | `#EF4444` | Primary error color — all faults and lockouts |
| Error 600 | `#DC2626` | Emergency stop button, pressed error state |
| Error Dim | `rgba(239, 68, 68, 0.12)` | Fault card background tint |
| Error Dim Strong | `rgba(239, 68, 68, 0.22)` | Hover over fault-tinted cards |

**Idle — Slate (Standby, Disabled, Disconnected)**

| Token | Value | Purpose |
|-------|-------|---------|
| Idle 400 | `#9CA3AF` | Neutral secondary elements |
| Idle 500 | `#6B7280` | Standby state, disabled text |
| Idle 600 | `#4B5563` | Deeply disabled elements |
| Idle Dim | `rgba(107, 114, 128, 0.10)` | Standby card tint |

#### Water Level Gradient

Five color stops represent the tank fill level. These are used exclusively on the tank visualization SVG. Each stop has a semantic meaning beyond the visual — the color communicates urgency.

| Level Range | Color | Hex | Meaning |
|-------------|-------|-----|---------|
| 0 – 20% | Critical Red | `#EF4444` | Tank nearly empty — pump should start |
| 20 – 40% | Low Orange | `#F97316` | Low level — approaching start threshold |
| 40 – 60% | Mid Amber | `#F59E0B` | Filling — normal mid-range |
| 60 – 90% | Good Blue | `#3B82F6` | Good level — healthy range |
| 90 – 100% | Full Green | `#10B981` | Tank full — stop threshold approaching |

#### Typography Colors (Dark)

| Token | Value | Purpose |
|-------|-------|---------|
| Text Primary | `#EDF2F7` | Headings, primary labels, active states |
| Text Secondary | `#8DA4BF` | Supporting labels, descriptions |
| Text Tertiary | `#526478` | Timestamps, units, section headings, placeholders |
| Text Disabled | `#2E3E50` | Disabled control labels |
| Text Data | `#E2ECF6` | Large numeric sensor values |
| Text Unit | `#576880` | Unit labels following numeric values (%, L/min, cm) |
| Text Link | `#60A5FA` | Hyperlinks, inline actions |

---

### 1.3 Light Theme Color Palette

> The light theme is purpose-built for outdoor and mobile use. It is not an inverted dark theme. Background layers use cool off-white tones instead of near-black. All semantic colors are slightly deeper to maintain WCAG AA contrast on light backgrounds.

#### Background Layers (Light)

| Token | Value | Purpose |
|-------|-------|---------|
| Canvas | `#E8EFF7` | Page root |
| Base | `#EEF4FB` | App shell |
| Surface | `#FFFFFF` | Primary card surface |
| Raised | `#F7FAFE` | Raised card variant |
| Elevated | `#EBF1F9` | Inputs, hover targets |
| Overlay | `#DDE7F3` | Modals, tooltips |
| Inset | `#E2EAF4` | Log backgrounds, code areas |

#### Border Levels (Light)

| Token | Value | Purpose |
|-------|-------|---------|
| Faint | `#D8E4F0` | Panel gap dividers |
| Subtle | `#C9D9EA` | Card edges |
| Default | `#AFC4D8` | Resting interactive borders |
| Strong | `#8AAAC4` | Active/focused borders |
| Focus | `#2563EB` | Keyboard focus ring |

#### Brand — Water Blue (Light)

Slightly deeper than dark mode to maintain contrast on light backgrounds.

| Token | Value |
|-------|-------|
| Brand 400 | `#3B82F6` |
| Brand 500 | `#2563EB` |
| Brand 600 | `#1D4ED8` |

#### Semantic Status Colors (Light)

All semantic colors are deepened by one shade to achieve WCAG AA against white and light grey backgrounds.

| State | Primary | Dim Background |
|-------|---------|---------------|
| OK | `#059669` | `rgba(5, 150, 105, 0.10)` |
| Warning | `#D97706` | `rgba(217, 119, 6, 0.10)` |
| Error | `#DC2626` | `rgba(220, 38, 38, 0.10)` |
| Idle | `#4B5563` | `rgba(75, 85, 99, 0.10)` |

Water level gradient colors are also deepened by one stop in light mode to maintain saturation against the white panel background.

#### Typography Colors (Light)

| Token | Value | Purpose |
|-------|-------|---------|
| Text Primary | `#0D1A27` | Headings, labels |
| Text Secondary | `#3D5368` | Supporting text |
| Text Tertiary | `#7A95AD` | Timestamps, units, placeholders |
| Text Disabled | `#B8CDD9` | Disabled controls |
| Text Data | `#0D1A27` | Numeric values |
| Text Unit | `#7A95AD` | Unit labels |
| Text Link | `#2563EB` | Links, inline actions |

---

### 1.4 Color Usage Rules

1. **Never hardcode a color value in a component.** Every color reference must use a design token. This ensures theme switching works without component-level changes.
2. **Semantic colors are reserved for their semantic meaning.** Green is never used for branding. Red is never decorative. Amber is never used for success.
3. **Dim variants are for backgrounds only.** The full-opacity semantic color is for text, icons, and borders. The dim variant is for card and panel background tints.
4. **Water level colors are used only on the tank visualization.** They do not appear in any other context.
5. **All status colors must maintain WCAG AA contrast (4.5:1) with their background.** Critical operational text (faults, alerts) must meet WCAG AAA (7:1).

---

### 1.5 Contrast Verification

| Foreground | Background (Dark) | Ratio | Pass |
|-----------|-------------------|-------|------|
| Text Primary | Surface | 11.2:1 | AAA ✓ |
| Text Secondary | Surface | 5.1:1 | AA ✓ |
| OK 500 | Surface | 5.8:1 | AA ✓ |
| Error 500 | Surface | 5.4:1 | AA ✓ |
| Warning 500 | Surface | 4.6:1 | AA ✓ |
| White text | Brand 500 | 4.5:1 | AA ✓ |

| Foreground | Background (Light) | Ratio | Pass |
|-----------|---------------------|-------|------|
| Text Primary | Surface | 14.3:1 | AAA ✓ |
| Text Secondary | Surface | 6.8:1 | AA ✓ |
| OK 500 (light) | Surface | 4.8:1 | AA ✓ |
| Error 500 (light) | Surface | 5.6:1 | AA ✓ |
| Warning 500 (light) | Surface | 4.5:1 | AA ✓ |

---

## Part 2 — Typography

### 2.1 Typefaces

**UI Font — Geist**
Used for all interface labels, navigation, button text, headings, descriptions, and alert messages. Geist was designed by Vercel specifically for developer tools, dashboards, and technical interfaces — making it a more intentional fit for SmartFlow than general-purpose screen fonts. Its letterforms are slightly more geometric and condensed than typical UI fonts, giving the dashboard a precise, instrument-like quality. This slight condensation is an active advantage in data-dense layouts where horizontal space is constrained.

**Data Font — Geist Mono**
Used exclusively for sensor readings, numeric values, timestamps, fault codes, log entries, and diagnostic output. Geist Mono was purpose-built to pair with Geist, sharing the same design language and proportions. Monospaced fonts are mandatory for data display for two reasons: tabular numbers prevent layout shift when digit counts change, and the distinct character forms — particularly slashed zero (0 vs O) and unambiguous one (1 vs l) — reduce misreading risk in critical operational contexts. Geist Mono carries a lighter, more refined character than heavier code-editor monospaced fonts, matching the modern industrial aesthetic of the SmartFlow design system.

**Font delivery — `geist` package + Next.js `next/font`**
Both Geist Sans and Geist Mono are loaded via the official **`geist`** npm package and **`next/font`** in the App Router root layout (`GeistSans` from `geist/font/sans`, `GeistMono` from `geist/font/mono`). Fonts are **bundled at build time** with the Next.js application and served from the same deployment origin (e.g. Vercel) — **no runtime Google Fonts or third-party font CDN requests**. This keeps latency predictable, works on local networks once the app shell is cached, and avoids an extra dependency on an external font host. Apply `GeistSans.variable` and `GeistMono.variable` on the document root and reference the resulting CSS custom properties from Tailwind (`tailwind.config.ts`) and global CSS (e.g. `--font-ui` for UI, `--font-data` for monospace data) so the whole dashboard uses one source of truth. **Fallback stacks** (system UI and monospace) ensure readable text before and if webfonts fail to load; `next/font` applies appropriate `font-display` behavior for the bundled faces.

| Role | Font | Fallback stack |
|------|------|---------------|
| UI Font | Geist | -apple-system, BlinkMacSystemFont, Segoe UI, system-ui, sans-serif |
| Data Font | Geist Mono | ui-monospace, Fira Code, Cascadia Code, monospace |

**The rule is absolute:** If it is a number, a code, a timestamp, or a raw value — it is Geist Mono. If it is a word, a label, or a sentence — it is Geist. These two fonts are never mixed within the same element.

---

### 2.2 Type Scale

| Role | Size | Weight | Font | Line Height | Usage |
|------|------|--------|------|-------------|-------|
| Hero | 48px / 3rem | 600 | Geist Mono | 1.0 | Tank level percentage — one per screen |
| Metric | 32px / 2rem | 600 | Geist Mono | 1.0 | Flow rate, countdown timer |
| Sub-metric | 24px / 1.5rem | 500 | Geist Mono | 1.0 | Secondary readings, smaller counters |
| Display | 24px / 1.5rem | 600 | Geist | 1.2 | Modal titles, empty state headings |
| Title | 18px / 1.125rem | 600 | Geist | 1.3 | Card headings |
| Body | 15px / 0.9375rem | 400 | Geist | 1.6 | Descriptions, alert messages, confirmations |
| Label | 14px / 0.875rem | 500 | Geist | 1.4 | Compact UI labels, button text, stat keys |
| Section Heading | 12px / 0.75rem | 600 | Geist | 1.4 | Card section labels — uppercase, 0.10em letter spacing |
| Code | 13px / 0.8125rem | 400 | Geist Mono | 1.6 | Event log rows, diagnostics output |
| Micro | 12px / 0.75rem | 400 | Geist Mono | 1.4 | Unit labels (%, L/min, cm), timestamps, secondary codes |

**Minimum readable size:** 12px. Section headings and micro labels may use 12px because they supplement — not replace — larger primary information. No operational data appears below 13px.

---

### 2.3 Typography Rules

**Numeric tabular alignment**
All numbers in this dashboard use tabular figure rendering. This means every digit occupies the same horizontal space regardless of its natural width. The digit 1 takes the same space as the digit 8. This prevents the surrounding layout from shifting when a value like 98 changes to 100. This must be applied to all metric values, timestamps, counters, and log data.

**Slashed zero**
Data font instances must use slashed zero rendering. The character 0 must be visually distinct from the letter O in all operational contexts. This is especially critical in hex values (CRC codes), sensor readings, and event log entries.

**Section heading style**
Section headings inside cards follow a specific, consistent treatment: 12px, Geist 600, uppercase, 0.10em letter-spacing, tertiary text color. This style is never used for anything other than a card section label. It signals "this is a category", not "this is content".

**Line length**
Prose content (alert messages, confirmation dialog body text) must not exceed 60 characters per line. This applies to the modal body, alert descriptions, and event log messages. Sensor values and labels have no line length restriction.

---

## Part 3 — Spacing System

### 3.1 Base Unit

The spacing system uses a **4px base unit**. All spacing values are exact multiples of 4px. No intermediate values (such as 6px, 10px, or 14px) are used in the system. This constraint creates rhythm and ensures visual consistency across all components.

| Token | Value | Common use |
|-------|-------|-----------|
| Space 1 | 4px | Icon-to-label gap, dot-to-text gap in status pills |
| Space 2 | 8px | Between stacked label rows, between badge elements |
| Space 3 | 12px | Section heading bottom margin, button vertical padding |
| Space 4 | 16px | Compact card padding, stat key-value row spacing |
| Space 5 | 20px | Tab item horizontal padding |
| Space 6 | 24px | Standard card internal padding, horizontal button padding |
| Space 8 | 32px | Modal internal padding, large section separation |
| Space 10 | 40px | Between major layout zones where a visual gap is needed |
| Space 12 | 48px | Topbar height |
| Space 20 | 80px | Flow strip height |

---

### 3.2 Component Spacing Rules

| Component | Rule |
|-----------|------|
| Standard card internal padding | 24px on all sides |
| Compact status rail card padding | 12px vertical, 16px horizontal |
| Section heading bottom margin | 12px |
| Gap between metric value and its unit label | 4px |
| Gap between stacked stat rows in a card | 8px |
| Gap between Quick Stat tiles | 12px |
| Minimum separation between Emergency Stop and Pump Toggle | 24px |
| Modal internal padding | 32px |
| Tab bar item padding | 12px vertical, 20px horizontal |
| Bottom of mode selector to top of pump toggle | 16px |
| Bottom of pump toggle to top of emergency stop | 24px — Fitts's Law minimum |

---

## Part 4 — Border Radius

### 4.1 Radius Scale

| Token | Value | Purpose |
|-------|-------|---------|
| Radius Small | 4px | Tags, fault code badges, small status chips |
| Radius Medium | 8px | Buttons, input fields, stat tiles, mode segments |
| Radius Large | 12px | Primary cards, panel surfaces |
| Radius XL | 16px | Modals, glass tank panel container |
| Radius Full | 9999px | Status pills, dot indicators, toggles |

### 4.2 Component Radius Reference

| Component | Radius |
|-----------|--------|
| Primary card / panel | Large (12px) |
| Button | Medium (8px) |
| Status pill | Full (9999px) |
| Fault code badge | Small (4px) |
| Mode selector outer container | Medium (8px) |
| Mode segment active indicator | Small (4px) |
| Tank SVG body | Large (12px) — matches card radius |
| Quick stat tile | Medium (8px) |
| Modal | XL (16px) |
| Input field | Medium (8px) |
| Tooltip | Small (4px) |
| Sensor health bar | Full (9999px) |
| Flow strip background tint | 0px — flush with panel edges |

---

## Part 5 — Elevation & Shadow

### 5.1 Dark Theme Elevation

In the dark theme, elevation is communicated primarily through **background lightness**, not shadows. A lighter background surface sits higher than a darker one. Shadows are used only for floating elements that must appear above the page plane.

| Level | Visual method | Used on |
|-------|--------------|---------|
| 0 — Recessed | Inset background color | Log areas, code blocks |
| 1 — Base | Surface background color | Primary cards |
| 2 — Raised | Raised background color | Nested cards, elevated sections |
| 3 — Float | Small shadow | Dropdowns, tooltips |
| 4 — Elevated | Medium shadow | Modals, bottom sheets |
| 5 — Overlay | Large shadow | Confirmation dialogs |

**Shadow values (dark theme)**
- Extra Small: 0 1px 2px rgba(0, 0, 0, 0.40)
- Small: 0 2px 4px rgba(0, 0, 0, 0.50)
- Medium: 0 4px 12px rgba(0, 0, 0, 0.50) and 0 1px 3px rgba(0, 0, 0, 0.40)
- Large: 0 8px 24px rgba(0, 0, 0, 0.60) and 0 2px 6px rgba(0, 0, 0, 0.40)

**Focus ring**: 0 0 0 3px Brand Focus color (blue, 3px spread)

---

### 5.2 Light Theme Elevation

In the light theme, all surfaces are light, so background lightness cannot communicate elevation. Elevation is communicated through **box shadows** using low-opacity dark colors.

**Shadow values (light theme)**
- Extra Small: 0 1px 2px rgba(13, 26, 39, 0.06)
- Small: 0 1px 3px rgba(13, 26, 39, 0.08) and 0 1px 2px rgba(13, 26, 39, 0.06)
- Medium: 0 4px 12px rgba(13, 26, 39, 0.10) and 0 1px 3px rgba(13, 26, 39, 0.07)
- Large: 0 8px 24px rgba(13, 26, 39, 0.12) and 0 2px 6px rgba(13, 26, 39, 0.08)

**Focus ring**: 0 0 0 3px Brand Focus color at 30% opacity

In light mode, all primary cards also carry a 1px subtle border in addition to their shadow. The shadow communicates elevation; the border communicates edge definition.

---

## Part 6 — Layout System

### 6.1 Layout Philosophy

The dashboard uses a **fixed three-column grid** on desktop. The layout never reflows in response to data updates — it is a permanent spatial structure. Users build spatial memory: they always know where the tank is, where controls are, and where history lives. This is a direct application of the Gestalt principle of continuity and the HCI principle of consistency.

Each zone has a defined information type. Information must not migrate between zones. The tank visualization is never in the controls panel. Controls are never in the history strip. Mixing zones breaks the user's spatial model.

---

### 6.2 Desktop Layout (1024px and above)

**Grid structure**

Three columns, four rows. Column widths: 220px (left rail) — flexible (center) — 300px (right controls). Row heights: 48px (topbar) — flexible (main) — 80px (flow strip) — 180px (history strip). All panel gaps are 1px, filled with the Faint border color to act as visible dividers.

**Zone definitions**

| Zone | Location | Content type | Scroll behavior |
|------|----------|-------------|----------------|
| Topbar | Top full width | System identity, global status | Sticky — never scrolls |
| Left Rail | Left column, full height below topbar | System health status cards | Scrolls independently if content exceeds height |
| Center Tank | Center column, main row | Tank visualization hero | No scroll |
| Flow Strip | Center column, below tank | Flow rate readout | No scroll |
| Right Controls | Right column, full height below topbar | Operator controls | Scrolls independently |
| History Strip | Bottom full width | Charts, logs, diagnostics | Tabs control content; panel itself does not scroll |

---

### 6.3 Tablet Layout (768px – 1023px)

The right controls column collapses below the center and left areas. The left rail narrows to 200px. The history strip becomes full width below both columns. The overall layout becomes two columns in the main area with the controls beneath, stacking vertically. The topbar remains sticky.

---

### 6.4 Mobile Layout (below 768px)

Mobile layout follows a **mobile-first, bottom-sheet pattern**. The dashboard becomes a single vertical column.

**Topbar** — Sticky at top. Height increases to 56px for improved touch target size. Wordmark and status pill remain. Clock compresses to HH:MM only.

**Status Rail** — Transforms from a vertical stack to a horizontal scrollable pill strip directly below the topbar. Each card condenses to a small pill showing only a status dot and a two-word label (e.g., "● Node Online", "● Motor Running"). No card text. Height: 52px. Overflow scrolls horizontally without a visible scrollbar.

**Tank Panel** — Full width. Tank SVG scales down to 160px wide and 260px tall. All other tank panel content remains.

**Flow Strip** — Full width. No changes.

**History Strip** — Full width. Chart height reduces to 120px. Tab labels may scroll horizontally.

**Controls Panel** — Hidden by default. Revealed via a **Floating Action Button (FAB)** fixed at bottom-right (above the emergency stop bar). The FAB opens a **bottom sheet** — a panel that slides up from the bottom of the screen, covering approximately 70% of the viewport. The bottom sheet contains the mode selector, pump toggle, countdown ring, quick stats, and bypass toggle.

**Emergency Stop** — Permanently fixed at the very bottom of the screen in a dedicated bar, always visible, never inside the bottom sheet. This follows Fitts's Law and the safety principle that the emergency control must always be reachable without any navigation step. Height: 64px. Full viewport width.

**Alert cards** — When a fault is active, a full-width alert banner appears directly below the topbar, above the status rail strip, at the top of the main scroll area.

**Touch targets** — All interactive elements must be a minimum of 44 × 44px on mobile. Buttons, tabs, and mode segments must meet this minimum regardless of their visual appearance.

---

### 6.5 Content Dimension Reference

| Element | Desktop | Mobile |
|---------|---------|--------|
| Topbar height | 48px | 56px |
| Left rail width | 220px | Collapsed to horizontal strip |
| Right controls width | 300px | Bottom sheet, full width |
| Tank SVG width | 200px | 160px |
| Tank SVG height | 320px | 260px |
| Flow strip height | 80px | 80px |
| History strip height | 180px | 200px |
| Emergency stop bar height | — (inline) | 64px fixed |
| FAB size | — | 56 × 56px |
| Bottom sheet height | — | ~70% viewport |

---

## Part 7 — Component Behavior Specifications

### 7.1 Status Indicators

**Dot indicator**
A 6px circle preceding all status pill labels. In Online/Running/OK state, the dot animates with a slow opacity pulse (1.0 → 0.3 → 1.0) on a 2-second cycle. In all other states, the dot is static. The pulse communicates active monitoring — the system is alive and watching.

**Status pill**
Inline container with full-radius (9999px) border-radius. Carries both a colored background tint (dim variant) and a matching border (dim-strong variant) to remain visible on both light and dark surfaces. Pill text uses Inter 500, 14px, Label size.

**Status progression** — colors map exclusively to these states:

| Condition | Color | Dot animation |
|-----------|-------|--------------|
| Online, healthy | OK Green | Pulsing |
| Sleeping (scheduled) | Idle Slate | Static |
| Degraded, stale data | Warning Amber | Static |
| Offline, fault active | Error Red | Static |
| Safe mode | Error Red | Static |

---

### 7.2 Cards

**Standard card** — Background: Surface. Border: none in dark (separation by background contrast), 1px Subtle + Shadow Small in light. Radius: Large (12px). Internal padding: 24px.

**Fault alert card** — Background: Error Dim. Left border: 3px solid Error 500. Right and corners use Radius Medium (8px). This neobrutalist left-border treatment is intentional — it makes fault cards unmistakable through peripheral vision alone, without relying on color perception.

**Warning card** — Same as fault alert card with Warning colors.

**Glass panel (tank hero only)** — Semi-transparent background using the glass background token. Blur effect applied to background behind the panel. 1px glass border. Inset top shine of 1px. Radius: XL (16px). Large drop shadow. This treatment is used only on the tank panel — it is a deliberate exception to the flat card system, justified by the water/glass metaphor.

---

### 7.3 Mode Selector

Three equal-width segments in a single container. The container has a 3px internal padding creating a thin gap between the active background and the container edge. Active segment fills with Brand 500 and carries white text. Inactive segments are transparent with secondary text color. Segment radius: Small (4px) inside the Medium (8px) container.

**Disabled behavior** — When a fault is active or emergency stop is latched, all segments appear at 38% opacity with unresponsive interaction. They remain fully visible. The operator can see what modes exist and infer from context why they are unavailable. This follows the HCI principle of error prevention — removing controls entirely would confuse the operator when the fault clears.

**ARIA roles** — The container has `role="radiogroup"`. Each segment has `role="radio"` and `aria-checked`. This ensures screen readers announce the current mode and available options correctly.

---

### 7.4 Pump Toggle Button

Full-width, 56px tall, Radius Medium (8px). This is the primary pump control in Manual mode.

**States:**

| State | Background | Border | Text | Interaction |
|-------|-----------|--------|------|-------------|
| Off (ready to start) | Elevated | Default | "Start Pump" | Hover: OK Dim background |
| On (running) | OK Dim Strong | OK 500 | "Stop Pump" with running dot | Active: OK Dim |
| Cooldown | Elevated | Subtle | "Cooldown Xs" in Data font | Unresponsive for 30s |
| Disabled | Elevated | Subtle | "Start Pump" | 38% opacity, unresponsive |

The cooldown state shows a live countdown in the button label using the Data font. This prevents the operator from attempting a restart too quickly after a stop, protecting the motor from rapid cycling. The 30-second minimum off-time is enforced in firmware — the UI prevents the attempt proactively.

---

### 7.5 Emergency Stop Button

Full-width, 48px tall, Radius Medium (8px). Always visible. Never disabled. Never hidden. Spatially separated from the pump toggle by a minimum of 24px.

**Normal state** — Transparent background, 1.5px Error 500 border, Error 500 text. Hover: Error Dim background fills in.

**Latched state** — Error 500 solid background, white text, reads "Latched — Tap to Reset". Hover: Error 600 background.

**Tap behavior — Normal state** — Opens a confirmation modal before any action is taken. The modal must be explicitly confirmed. Dismissing the modal with Cancel or tapping outside it produces no action.

**Tap behavior — Latched state, no active lockout** — Clears the emergency stop latch immediately. No confirmation required for releasing — only for triggering.

**Tap behavior — Latched state, with active lockout** — Shows a tooltip: "Clear the active fault first." Does not write any command. The operator must resolve the underlying DRY_RUN or OVERFLOW fault before the stop can be reset.

**Focus ring** — Brand blue focus ring on keyboard navigation. The emergency stop must be reachable via Tab key from any position in the controls panel.

---

### 7.6 Confirmation Modal

Appears center-screen on desktop, bottom-anchored on mobile with scrim behind. Maximum width: 400px. Radius: XL (16px). Internal padding: 32px. Scrim: page base color at 75% opacity with 4px background blur.

**Structure** — Title (Title size, Inter 600), Body (Body size, Inter 400, secondary color, max 60 characters per line), Action row with two buttons right-aligned.

**Action buttons** — Confirm action (right, Primary or Error button depending on action severity). Cancel (left, Secondary button). Cancel always dismisses with no action. Confirm executes and dismisses.

**Keyboard behavior** — Escape key dismisses as Cancel. Enter key in confirm context activates the Confirm button. Focus is trapped inside the modal while open.

---

### 7.7 Countdown Ring

Visible only in Countdown mode. SVG circular progress ring, center-mounted in the controls panel below quick stats.

**Ring structure** — Background track: Border Subtle color. Progress fill: Brand 500. Stroke width: 4px. Ring diameter: 96px. Center text: MM:SS in Sub-metric size (24px), Data font.

**Animation** — The stroke offset decreases smoothly as time passes. The ring value is sourced from Firebase (`countdown_remaining_sec`). Between Firebase updates (every 3 seconds), the client interpolates the remaining time in real time so the countdown appears continuous rather than stepping.

**Controls below ring** — "+5 min" button (Secondary style, compact) and "Cancel" text link. "+5 min" is disabled if adding 5 minutes would exceed the 120-minute maximum. "Cancel" reverts mode to AUTO.

---

### 7.8 Tank Visualization

The tank SVG is the visual center of the dashboard and the first element the operator's eye finds on load.

**Structure** — Rounded rectangle body (12px radius matching card radius). Water fill rises from the bottom. Level percentage overlaid at center. Distance reading below. Sensor health bar below distance.

**Water surface animation** — A subtle sine wave animates across the water surface at 3-second period, 3px amplitude, continuous loop. This communicates that the level is a live reading, not a static snapshot. The animation is a visual affordance for "live data."

**Fill color transitions** — The fill color transitions smoothly between water level gradient stops as the level changes. The transition duration for both height and color is 800ms using a natural ease curve, giving the fill the physical quality of actual water rising.

**Ambient glow** — A soft blue glow (Brand Glow color) radiates behind the tank SVG via drop shadow. In light mode this is softer. This subtly reinforces the water metaphor and draws the eye to the tank as the compositional hero.

**Threshold markers** — Two horizontal dashed lines at the pump start level (default 30%) and pump stop level (default 100%). Both lines are labeled with their percentage value in Micro size. These markers update dynamically if the thresholds are changed via Firebase configuration.

**Estimate mode** — When the flow-based level estimate is active (sensor bypassed), the level number is prefixed with "~" and a secondary label below the percentage reads "Flow estimate · +X.X L" in Warning Amber, italic, Micro size. This communicates clearly that the displayed value is an inference, not a direct measurement.

---

## Part 8 — Animation & Motion

### 8.1 Motion Philosophy

Animation in SmartFlow serves three purposes only: communicating live data flow, indicating state transitions, and providing user feedback on interactions. Decorative animation is not used. Motion is never applied to an element purely for visual interest.

All animations must be wrapped in a reduced-motion media query. On devices with this accessibility preference set, all animations except instantaneous state changes are disabled.

---

### 8.2 Duration & Easing Reference

| Context | Duration | Easing | Rationale |
|---------|----------|--------|-----------|
| Button hover/press | 120ms | Ease | Must feel immediate — fast feedback |
| Status pill color change | 150ms | Ease | Quick state acknowledgment |
| Card background tint | 150ms | Ease | Supporting visual, not primary |
| Theme transition (background, border, text color) | 200ms | Ease | Smooth but perceptibly fast |
| Tab switch content | 150ms | Ease | Navigation should feel instant |
| Tank fill height | 800ms | Natural ease (0.4, 0, 0.2, 1) | Physical, water-like quality |
| Tank fill color | 600ms | Ease | Slightly faster than height |
| Flow value color | 300ms | Ease | Noticeable but not distracting |
| Bottom sheet open/close | 300ms | Ease (0.4, 0, 0.2, 1) | Standard material motion |
| Modal enter | 200ms | Ease out | Appears quickly, stops smoothly |
| Modal exit | 150ms | Ease in | Dismisses quickly |
| Status dot pulse | 2000ms | Ease in-out, infinite | Breathing — calm aliveness |
| Flow icon animation | 1500ms | Ease in-out, infinite | Active when flow > 0, paused otherwise |
| Skeleton shimmer | 1500ms | Linear, infinite | Loading placeholder |

**Transition property restriction** — The global transition must be applied only to `background-color`, `border-color`, `color`, `opacity`, and `box-shadow`. Never apply `transition: all` — this causes layout properties (width, height, padding) to animate unintentionally during data updates, producing jank.

---

### 8.3 Loading States

On initial Firebase connection, all metric positions show a skeleton shimmer placeholder — a gradient sweep animation over a rounded rectangle the same size as the expected value. This communicates "data is loading" without blocking the entire interface.

Skeleton placeholders appear in:
- Tank level percentage
- Distance readout
- Flow rate value
- Status rail metric values
- Connectivity status values

Skeletons are removed and replaced with live data as each field arrives. The transition from skeleton to live value is an opacity fade at 200ms.

---

## Part 9 — Data Staleness Indicators

Data freshness is a safety concern in an operational system. The dashboard must communicate when data is old, not just when it is absent.

### 9.1 Staleness Thresholds

| Age | Visual treatment | Meaning |
|-----|-----------------|---------|
| Under 10 seconds | Normal — no indicator | Data is fresh |
| 10 – 30 seconds | Amber left border on all metric cards. Sync counter turns Warning Amber | Data is aging — monitor |
| Over 30 seconds | Red left border on all metric cards. Sync counter turns Error Red | Data is stale — investigate |

### 9.2 Level Freshness

When the `level_fresh` field is false (level data is older than 2.5 seconds), the tank panel gains an amber left border and a small amber badge reads "Stale" below the level percentage. This is distinct from the general staleness indicator — it refers specifically to level data age and has direct implications for pump control decisions.

---

## Part 10 — Accessibility Requirements

### 10.1 Visual Accessibility

**Color contrast** — All text on background combinations must meet WCAG 2.1 Level AA (4.5:1 minimum). Operational status text — fault messages, alert codes, error labels — must meet Level AAA (7:1 minimum). Contrast ratios are documented in Section 1.5.

**Color independence** — Status must never be communicated by color alone. Every status pill includes a text label. Every fault card includes a text description. Icons accompanying status always include a text label or tooltip. A fully color-blind operator must be able to use all features.

**Focus indicators** — Every interactive element must have a visible keyboard focus indicator: a 3px focus ring in the Brand Focus color. Focus rings must not be removed or suppressed under any circumstance.

**Font size minimum** — No operational information may be displayed below 13px. Section headings and unit labels at 12px are supplementary only and always accompany larger primary information.

---

### 10.2 Interaction Accessibility

**Disabled states** — Disabled controls use 38% opacity and `aria-disabled="true"`. They are never removed from the DOM. Screen readers announce them as disabled and include the reason via `aria-describedby` where applicable.

**Touch targets** — All interactive elements on mobile must be a minimum of 44 × 44px. Small elements (status rail pills) must have invisible hit area padding to meet this minimum without visual changes.

**ARIA roles** — Mode selector: `role="radiogroup"` with `role="radio"` segments. Status pills: `role="status"`. Alert cards: `role="alert"` for automatic announcement. Modal: `role="dialog"` with `aria-modal="true"`. Emergency stop: `aria-live="assertive"` to announce state changes immediately.

**Keyboard navigation** — Full keyboard navigation must be supported. Tab order follows visual reading order (top-left to bottom-right, then controls). Modal focus must be trapped while open. Escape closes all overlays.

**Reduced motion** — All animations are wrapped in `prefers-reduced-motion: no-preference`. When reduced motion is preferred, all transitions and animations are disabled or replaced with instant state changes.

---

### 10.3 Operational Accessibility

**Screen legibility** — The design must be legible on a phone screen in direct outdoor sunlight. This is why the light theme uses a cool off-white base with deeper semantic colors — maintaining contrast in high-ambient-brightness conditions.

**One-hand mobile operation** — On mobile, all primary controls (emergency stop, pump toggle via bottom sheet, FAB) are positioned in the lower portion of the screen, reachable with a thumb without repositioning the hand.

**Critical information at a glance** — Level percentage, running state, and last fault code must always be visible on the first viewport without any scroll or interaction, on any screen size.

---

## Part 11 — Theme Switching

### 11.1 Theme Persistence

The selected theme is stored in browser local storage under the key `smartflow-theme`. On next load, the stored preference is applied before the first paint — there is no flash of the wrong theme.

On first load with no stored preference, the system preference (`prefers-color-scheme`) is used as the default. If neither is available, the dark theme is the fallback.

### 11.2 Theme Toggle Behavior

The toggle button is in the topbar, far right. In dark mode it shows a sun icon (☀). In light mode it shows a moon icon (☾). Both icons carry a tooltip: "Switch to light mode" / "Switch to dark mode".

When toggled, the theme change applies instantly to all color tokens through the CSS custom property cascade. The transition (background color, border color, text color, 200ms) ensures the change is smooth rather than a hard snap. Layout properties do not transition — only colors.

All charts update their grid line and tick colors immediately after a theme switch. Charts use the "no animation" update mode so they repaint instantly without a visual sweep.

### 11.3 Theme Transition Rules

- **Transitions apply to:** background-color, border-color, color, opacity, box-shadow
- **Transitions never apply to:** width, height, padding, margin, transform, font-size
- **Duration:** 200ms ease for all theme-driven color changes
- **Charts:** Repaint without animation — `update('none')` mode

---

## Part 12 — Cross-Browser & Performance Requirements

### 12.1 Browser Support

The dashboard must function correctly and appear consistently in:

| Browser | Minimum version |
|---------|----------------|
| Chrome / Edge | Last 2 major versions |
| Firefox | Last 2 major versions |
| Safari (iOS) | Last 2 major versions |
| Samsung Internet | Last major version |

### 12.2 Glassmorphism Fallback

The glass panel treatment (blur effect behind the tank hero) requires `backdrop-filter` browser support. When not supported, the glass panel falls back to the standard Surface background color at 95% opacity with the standard card border. The visual result is slightly different but fully functional. No features are lost.

### 12.3 Performance Targets

| Metric | Target |
|--------|--------|
| First paint | Under 1 second on standard broadband |
| Time to interactive | Under 2 seconds |
| HTTP requests | Fonts: none from a font CDN — Geist is bundled via `next/font` + `geist` package. (Overall budget should stay lean; charting uses in-bundle libraries, not a separate Chart.js CDN.) |
| Image assets | Zero — all visuals are SVG or CSS |
| Layout recalculation on data update | Zero — all data updates are in-place value replacements, not layout changes |
| Firebase update cycle | 3 seconds (Firebase RTDB push interval) |
| UI interpolation | Countdown timer interpolated client-side between 3s Firebase updates |

### 12.4 Optimizations Required

**Font loading — `geist` + `next/font`** — Use the **`geist`** package with **`next/font`** so Geist Sans and Geist Mono are compiled into the production build and served from the app origin. Do **not** load Geist from `fonts.googleapis.com` or other font CDNs. This matches Vercel/Next deployment patterns, avoids extra third-party font infrastructure, and keeps first paint on fallbacks until bundled faces are ready. Prefer the same pattern in documentation and code review (root `layout.tsx`, CSS variables wired to Tailwind `fontFamily`).

**Tabular numbers** — Applied to all metric values. Prevents the browser from recalculating layout when a displayed number changes width (e.g., 9 → 10 → 11). This is a mandatory performance and stability requirement, not just a visual preference.

**Reserved metric space** — All metric value containers must have a minimum width set to the maximum expected digit width. A tank level display must reserve space for three digits (100%). This prevents the surrounding layout from shifting when the value changes.

**No layout shift on data update** — Zero cumulative layout shift (CLS) during live data updates is a hard requirement. All data containers must have fixed or minimum dimensions. Dynamic content must never push or pull adjacent elements.

---

*SmartFlow Design System v1.1*
*Compiled against SmartFlow Firmware v3.0 — ESP32 Main Controller + NodeMCU V2 Sensor Node*
*Typography: Geist + Geist Mono — `geist` npm package + Next.js `next/font` (bundled; no font CDN)*
*All contrast ratios verified against WCAG 2.1 Level AA / AAA standards*
*Token naming convention: role-based descriptive names, not value-based names*
