# Typography

SmartFlow requires typography that is highly legible on industrial HMIs, mobile screens outdoors in the sun, and dark pump rooms.

## Typefaces
1. **Primary Typeface:** **Inter**
   - *Role:* UI elements, buttons, data tables, metrics, navigation.
   - *Why:* Inter is a highly legible, variable neo-grotesque font specifically designed for computer screens. Its tall x-height and clear distinct characters (like distinguishing 'l', 'I', and '1') are vital for reading critical telemetry data like "11.1 GPM".
2. **Secondary Typeface:** **Roboto**
   - *Role:* Body copy, long-form text, documentation.
   - *Why:* The native Android standard. Provides a familiar, highly readable fallback for general text.

## Typography Scale (Material 3)

| Role | Font | Weight | Size | Line Height | Tracking (Letter Spacing) | Usage |
|------|------|--------|------|-------------|---------------------------|-------|
| **Display Large** | Inter | Bold (700) | 57sp | 64sp | -0.25sp | Hero metrics (e.g., "128 GPM" on a dashboard) |
| **Display Medium** | Inter | SemiBold (600)| 45sp | 52sp | 0sp | Secondary hero metrics |
| **Display Small** | Inter | Medium (500) | 36sp | 44sp | 0sp | Modal headers, large states |
| **Headline Large** | Inter | SemiBold (600)| 32sp | 40sp | 0sp | Page Titles |
| **Headline Medium**| Inter | Medium (500) | 28sp | 36sp | 0sp | Bottom sheet titles |
| **Headline Small** | Inter | Medium (500) | 24sp | 32sp | 0sp | Card Headers (e.g., "Pump Status") |
| **Title Large** | Inter | Medium (500) | 22sp | 28sp | 0sp | Dialog titles |
| **Title Medium** | Inter | Medium (500) | 16sp | 24sp | 0.15sp | List item titles, subtitle |
| **Title Small** | Inter | Medium (500) | 14sp | 20sp | 0.1sp | Small UI headers |
| **Body Large** | Roboto | Regular (400) | 16sp | 24sp | 0.5sp | Paragraphs, descriptions |
| **Body Medium** | Roboto | Regular (400) | 14sp | 20sp | 0.25sp | Secondary descriptions |
| **Body Small** | Roboto | Regular (400) | 12sp | 16sp | 0.4sp | Captions, footnotes |
| **Label Large** | Inter | Medium (500) | 14sp | 20sp | 0.1sp | Button text, Tabs, Data table headers |
| **Label Medium** | Inter | Medium (500) | 12sp | 16sp | 0.5sp | Badge text, Chips |
| **Label Small** | Inter | Medium (500) | 11sp | 16sp | 0.5sp | Overline, small graph axis labels |

## Hierarchy & Accessibility
- **Contrast:** All typography must meet WCAG AA contrast standards. Text on `background` and `surface` must achieve at least a 4.5:1 ratio.
- **Tabular Figures:** When displaying changing numbers (timers, flow rates, pressure), enable the `tnum` (tabular numbers) OpenType feature in Inter. This ensures characters have fixed widths, preventing the UI from "jittering" horizontally as numbers change rapidly.
- **Dynamic Type:** All sizes are defined in `sp` (scaleable pixels). The app must support user-adjusted font sizes up to 200% without breaking critical layouts.
