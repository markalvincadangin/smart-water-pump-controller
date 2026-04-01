# SmartFlow — Brand & Design System Reference

## Identity

**Name:** SmartFlow
**Tagline:** Automated. Aware. Always on.
**Tone:** Industrial precision. Data-forward. Not decorative.
**Typography:** Geist (UI/display) + Geist Mono (values, metrics, technical readouts)
**Font hosting:** Self-hosted from `/public/fonts/` — no external CDN requests

---

## Color Tokens

Add to `tailwind.config.ts` under `theme.extend.colors`:

```typescript
sf: {
  blue:           '#185FA5',   // Primary — actions, active states, brand
  'blue-mid':     '#378ADD',   // Links, interactive elements
  'blue-light':   '#E6F1FB',   // Info backgrounds, sleeping state
  teal:           '#0F6E56',   // Success — pump running, healthy
  'teal-light':   '#E1F5EE',   // Success backgrounds
  amber:          '#BA7517',   // Warning — degraded, bypass, idle
  'amber-light':  '#FAEEDA',   // Warning backgrounds
  red:            '#A32D2D',   // Error — lockout, emergency stop
  'red-light':    '#FCEBEB',   // Error backgrounds
  green:          '#3B6D11',   // Secondary success
  'green-light':  '#EAF3DE',   // Secondary success backgrounds
  gray: {
    50:  '#F1EFE8',            // Page background
    100: '#D3D1C7',            // Borders
    200: '#B4B2A9',            // Disabled borders
    600: '#5F5E5A',            // Secondary text
    900: '#2C2C2A',            // Primary text
  }
}
```

---

## Semantic Color Mapping

| State | Background | Text / Border | Usage |
|-------|-----------|---------------|-------|
| Pump running | `sf-teal-light` | `sf-teal` | Status chip, card accent |
| Standby | `sf-gray-50` | `sf-gray-600` | Default neutral state |
| Warning | `sf-amber-light` | `sf-amber` | Bypass active, idle mode, manual warning |
| Error / lockout | `sf-red-light` | `sf-red` | DRY_RUN, overflow, sensor error |
| Sleeping | `sf-blue-light` | `sf-blue` | Scheduled sleep active |
| Cooldown | `sf-blue-light` | `sf-blue-mid` | Off-timer active |
| Bypass active | `sf-amber-light` | `sf-amber` | Level or flow bypass |
| Emergency stop | `sf-red` (solid) | `white` | E-stop button — always red |

---

## PWA Manifest

`dashboard/public/manifest.json`:

```json
{
  "name": "SmartFlow",
  "short_name": "SmartFlow",
  "description": "Automated water pump controller — Leon, Iloilo",
  "theme_color": "#185FA5",
  "background_color": "#F1EFE8",
  "display": "standalone",
  "start_url": "/",
  "icons": [
    { "src": "/icons/icon-192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "/icons/icon-512.png", "sizes": "512x512", "type": "image/png" }
  ]
}
```

---

## Dashboard Layout

```
Header (sticky)
  SmartFlow wordmark — Geist 700, sf-blue, 22px
  WiFi RSSI indicator (small badge)
  Last updated timestamp (Geist Mono, sf-gray-600, 12px)
  Dark / light theme toggle

/ — Main dashboard
  ┌─────────────────────────────────────┐
  │  Tank Level Card                     │
  │  Animated SVG fill · % · ~cm        │
  │  Start/Stop level markers           │
  │  Level estimate visual if active    │
  ├─────────────────────────────────────┤
  │  Pump Status Card                    │
  │  Run mode chip + cooldown countdown │
  │  Flow rate (LPM) · Uptime           │
  │  Last boot reason                   │
  ├──────────────┬──────────────────────┤
  │  Controls    │  Alerts              │
  │  Mode select │  Error cards         │
  │  E-stop      │  Manual warning      │
  │  Countdown   │  Cooldown notice     │
  ├──────────────┴──────────────────────┤
  │  Diagnostics (collapsible default)  │
  │  Log level · Discard · Heap         │
  │  Firebase fails · RS-485 stats      │
  └─────────────────────────────────────┘

/settings
  Tank calibration
  Thresholds
  Bypass controls (level + flow)
  Sleep schedule
  Notification preferences
  Advanced (log level control)
```

---

## Component Specifications

### Tank Level SVG

- Animated fill via CSS `transition: height 0.5s ease-in-out`
- Color: 0–20% = `sf-red`, 20–50% = `sf-amber`, 50–100% = `sf-teal`
- Level number: Geist 700, 32px, centered
- Distance sub-label: Geist Mono 400, 14px, `sf-gray-600`
- Level estimate: `~82%` prefix, italic sub-label "Flow estimate · +X.X L", `sf-amber`
- Start/Stop level markers: horizontal dashed lines on SVG

### Run Mode Chip

```tsx
// Run mode → display label + colors
const MODE_LABELS: Record<string, { label: string; bg: string; text: string }> = {
  'AUTO_STANDBY':    { label: 'AUTO — Standby',      bg: 'bg-sf-gray-50',    text: 'text-sf-gray-600' },
  'AUTO':            { label: 'AUTO — Running',       bg: 'bg-sf-teal-light', text: 'text-sf-teal' },
  'AUTO_COOLDOWN':   { label: `AUTO — Cooldown`,      bg: 'bg-sf-blue-light', text: 'text-sf-blue-mid' },
  'MANUAL_ON':       { label: 'MANUAL — On',          bg: 'bg-sf-teal-light', text: 'text-sf-teal' },
  'MANUAL_OFF':      { label: 'MANUAL — Off',         bg: 'bg-sf-gray-50',    text: 'text-sf-gray-600' },
  'MANUAL_COOLDOWN': { label: `MANUAL — Cooldown`,    bg: 'bg-sf-blue-light', text: 'text-sf-blue-mid' },
  'COUNTDOWN':       { label: 'Countdown',            bg: 'bg-sf-blue-light', text: 'text-sf-blue' },
  'STOPPED':         { label: 'Emergency Stop',       bg: 'bg-sf-red-light',  text: 'text-sf-red' },
};
```

For COOLDOWN modes, append live countdown from `pump_cooldown_remaining_sec` using
`setInterval` client-side (not Firebase polling) for smooth countdown.

### Emergency Stop Button

- Always visible on mobile — pin to bottom of viewport on mobile screens
- Background: `sf-red` (solid), text: white, Geist 700
- Never hidden at any scroll position
- Disabled only while Firebase write is in-flight

### Debug Log Level Control

```tsx
const LOG_LEVELS = ['ERROR', 'WARN', 'INFO', 'DEBUG', 'VERBOSE'];
// Segmented control or <select> — writes to /pump_system/config/device/debug_log_level
// Show amber warning when set above INFO (2):
// "Verbose logging may increase Firebase write volume"
// Current level reflects debug_log_level from Firebase status (not config)
```

### Level Estimate Visual (Chart)

When `level_estimate_active: true`:
- Chart line: switch from solid `sf-teal` to dashed `sf-amber` at the bypass point
- Mark bypass start with vertical dashed line labeled "Sensor bypass"
- Tank SVG: `~82%` prefix in Geist Mono italic, sub-label "Flow estimate" in `sf-amber`

---

## Dashboard Bug Fixes Required (from Phase 0 Audit)

These must be applied regardless of what the audit finds — they are baseline quality requirements:

```typescript
// 1. Firebase listener cleanup
useEffect(() => {
  const ref = dbRef(db, '/pump_system/status');
  const unsub = onValue(ref, (snap) => { /* ... */ });
  return () => unsub();  // ← REQUIRED cleanup
}, []);

// 2. Typed Firebase data — no 'as any'
interface PumpStatus {
  water_level_percent?: number;
  is_running: boolean;
  run_mode: string;
  // ... all fields from firebase_schema.md
}
const status = snap.val() as PumpStatus;

// 3. Null-safe data access
const level = status?.water_level_percent ?? null;
const isRunning = status?.is_running ?? false;

// 4. Settings validation
if (pumpStartLevel >= pumpStopLevel) {
  setError('Start level must be less than stop level');
  return;
}

// 5. Pending state on Firebase writes
const [isPending, setIsPending] = useState(false);
const handleModeChange = async (mode: string) => {
  setIsPending(true);
  await set(dbRef(db, '/pump_system/control/mode'), mode);
  setIsPending(false);
};
// Disable all controls while isPending === true

// 6. Skeleton loader example
{status ? <LevelCard status={status} /> : <LevelCardSkeleton />}
```

---

## Rebranding String Reference

| Old | New | Where |
|-----|-----|-------|
| `Smart Water Pump Controller` | `SmartFlow` | All user-visible strings |
| `smart-water-pump-controller` (in title/meta) | `smartflow` | HTML meta, package.json name |
| Dashboard `<title>` | `SmartFlow` | app/layout.tsx |
| Header brand text | `SmartFlow` | components/Header.tsx |
| PWA manifest `name` | `SmartFlow` | public/manifest.json |
| PWA manifest `short_name` | `SmartFlow` | public/manifest.json |
| Firmware boot banner | `SmartFlow vX.X` | setup() Serial.println |
| Docs headers | `SmartFlow` | All docs/*.md |
| Root README `# Smart Water Pump Controller` | `# SmartFlow` | README.md |

**DO NOT rename:**
- `arduino_smart_water_pump_controller/` folder
- `arduino_smart_water_pump_controller.ino` primary sketch file
- `smart_water_pump_controller_shared.h` (can rename later if desired, but risky)
