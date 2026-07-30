# Layout System

SmartFlow's layout system relies on an 8dp grid, Material 3 elevation models, and robust geometric shapes.

## Spacing Grid
We use a strict **8dp** base grid for all padding, margins, and component sizing. 
- **Base Unit:** `8dp`
- **Increments:** 4dp (Half), 8dp (1x), 16dp (2x), 24dp (3x), 32dp (4x), 48dp (6x), 64dp (8x).
- **Usage:** 
  - `4dp`: Spacing between tightly coupled elements (e.g., an icon and its text label).
  - `8dp`: Standard spacing between items in a list or inside a card.
  - `16dp`: Standard margin for the edge of the screen on mobile, and standard padding inside a Card.
  - `24dp`: Spacing between distinct sections on a dashboard.

## Shapes
We use geometric, rounded shapes to balance the harshness of industrial data with a modern, approachable aesthetic. Corner radii communicate interactivity.
- **Extra Small (4dp):** Checkboxes, small tooltips, system badges.
- **Small (8dp):** Text fields, buttons (if not fully pill-shaped), small chips.
- **Medium (16dp):** Standard Cards (`TankLevelCard`, `PumpStatusCard`).
- **Large (24dp):** Bottom Sheets, large modal dialogs.
- **Full / Pill (100%):** Primary Action Buttons (FABs), standard active chips.

## Elevation (Material 3)
In SmartFlow, we rely primarily on **Tonal Elevation** (color shifts) rather than heavy drop shadows, especially in Dark Mode where shadows are invisible.
- **Level 0 (0dp):** `background`. The base canvas.
- **Level 1 (1dp):** `surface` with a slight `primary` tint. Used for resting Cards.
- **Level 2 (3dp):** Used for Top App Bars when scrolling, and slightly elevated Cards (e.g., a selected state).
- **Level 3 (6dp):** Bottom Sheets and Nav Bars.
- **Level 4 (8dp):** Dialogs.
- **Level 5 (12dp):** Modal dialogs or elements requiring absolute highest z-index.

## Responsive Layout & Safe Areas
- **Edge-to-Edge:** The app must draw edge-to-edge behind the system navigation and status bars. Use `WindowInsets` to apply padding only where necessary (e.g., adding `navigationBarsPadding` to a Bottom Navigation, but letting the background color bleed behind it).
- **Adaptive:** While primarily designed for portrait mobile use, the layout must safely adapt to landscape mode. In landscape, vertically scrolling lists should transition to grids (e.g., `LazyVerticalGrid` with 2 columns) to optimize horizontal space.
