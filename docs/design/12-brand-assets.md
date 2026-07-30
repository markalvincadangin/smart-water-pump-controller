# Brand Assets & Future Roadmap

## Required Brand Assets

To fully implement the SmartFlow brand, the following source assets must be generated and stored in version control:

### 1. Vector Logos (SVG)
- `smartflow-logo-horizontal.svg` (Primary, full color)
- `smartflow-logo-stacked.svg` (Vertical)
- `smartflow-icon.svg` (Standalone droplet/arcs)
- `smartflow-wordmark.svg` (Standalone text)
- `smartflow-icon-monochrome.svg` (Solid single path)

### 2. Raster Logos (PNG)
- High-resolution transparent PNG exports (1024x1024 minimum) of all vectors above for legacy systems or environments that do not support SVG.

### 3. Font Files (TTF/OTF)
- `Inter` font family files (Variable or Static weights: 400, 500, 600, 700).
- `Roboto` font family files (Regular 400).
- These must be included in the Android project under `app/src/main/res/font/`.

### 4. Android XML Assets
- `ic_launcher_foreground.xml` (VectorDrawable)
- `ic_launcher_background.xml` (VectorDrawable)
- `ic_splash_logo.xml` (VectorDrawable)

---

## Future Roadmap: Scaling the Design System

While currently optimized for a mobile Android application, the SmartFlow Design System is built to scale across the entire IoT ecosystem.

### 1. Web Dashboard (React/Next.js)
- **Adaptation:** The 8dp grid scales perfectly to web. The `design-tokens.md` can be exported directly into Tailwind CSS configuration (`tailwind.config.js`).
- **Layout:** On large screens, the stacked mobile cards will reflow into a multi-column dashboard grid. The `PumpStatusCard` remains top-left (highest priority), while historical `TelemetryCharts` span the wider horizontal space.

### 2. Tablet / Industrial Kiosk
- **Adaptation:** The UI will transition to a permanent two-pane layout. Navigation moves from a Bottom Bar to a persistent Left Navigation Rail. 
- **Typography:** The `Display` and `Headline` typography scales will be utilized more frequently to ensure legibility from a distance (e.g., an operator looking at a wall-mounted tablet).

### 3. Wear OS (Smartwatches)
- **Adaptation:** The interface will strip down to the bare minimum: Current Pump Status, Tank Level, and critical alarms.
- **Colors:** Deep blacks (`#000000`) will replace the Navy Background (`#0F172A`) to save battery on OLED screens. 

### 4. Fleet Management (Enterprise Platform)
- **Adaptation:** When managing hundreds of pumps, individual `PumpStatusCards` are too large. The design system will introduce high-density Data Tables. 
- **Semantics:** The color system's status colors (`secondary` Green, `error` Red) will be heavily utilized in compact "Status Dot" indicators to quickly scan a list of 500 pumps and find the 3 that are failing.
