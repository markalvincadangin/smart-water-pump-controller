# Smart Water Pump Controller — Web Dashboard

Next.js 14 + Firebase RTDB real-time dashboard for the ESP32 pump controller.  
Access is protected by **Google sign-in**; only authorized users can view and control the pump.

---

## Tech Stack

| Layer      | Technology                              |
|------------|-----------------------------------------|
| Framework  | Next.js 14 (App Router)                 |
| Language   | TypeScript                              |
| Styling    | Tailwind CSS                            |
| Charts     | Recharts                                |
| Icons      | Lucide React                            |
| Database   | Firebase Realtime Database              |
| Auth       | Firebase Google Authentication          |
| Fonts      | Syne · DM Sans · JetBrains Mono (Google)|

---

## Firebase Data Structure

The dashboard reads/writes the same paths as the ESP32 firmware:

```
/pump_system/
  status/                    ← ESP32 writes every 3s
    water_level_percent, is_running, flow_rate_lpm, is_error,
    is_sensor_error, is_overflow_error, is_sleeping,
    wifi_rssi, last_boot_reason, uptime_minutes

  control/                   ← Dashboard writes, ESP32 reads every 3s
    mode: "AUTO"             ← "AUTO" | "FORCE_ON" | "FORCE_OFF"
    clear_error: false       ← Set to true to acknowledge errors

  config/
    device/                  ← Dashboard writes, ESP32 reads every 30s
      tank_empty_cm, tank_full_cm, pump_start_level, pump_stop_level,
      dry_run_threshold_lpm, dry_run_timeout_sec, flow_calibration_factor,
      max_pump_runtime_min, sleep_*, sensor_failure_threshold, idle_*_ms
    notifications_by_user/   ← Per-user notification settings (bell icon)
      $uid/  enabled, email, dryRunAlert, lowLevelAlert, lowLevelThreshold,
             pumpStartedAlert, overflowAlert
```

---

## Quick Start

### 1. Prerequisites
- Node.js 18+
- A Firebase project with **Realtime Database**, **Email/Password**, and **Google Auth** enabled

### 2. Clone / copy this project
```bash
# From the repo root (smart-water-pump-controller):
cd dashboard
```

### 3. Install dependencies
```bash
npm install
```

### 4. Configure environment
```bash
cp .env.local.example .env.local
```
Edit `.env.local` and fill in your Firebase credentials. Optionally set `NEXT_PUBLIC_AUTHORIZED_UIDS` (comma-separated Firebase UIDs) to restrict which Google accounts can sign in.

**Where to find Firebase values:**  
Firebase Console → Project Settings → General → Your apps → Web → SDK setup and configuration

### 5. Enable Firebase services
In the Firebase Console:
1. **Realtime Database** → Create database → Start in test mode
2. **Authentication** → Sign-in methods:
   - **Email/Password** → Enable (ESP32 uses this)
   - **Google** → Enable (dashboard uses this)

### 6. Set Firebase security rules
Deploy rules from project root: `firebase deploy --only database`

Before deploying, edit `database.rules.json` and replace `YOUR_GOOGLE_UID` with your UID from Authentication → Users. For multiple UIDs, use `auth.uid === 'uid1' || auth.uid === 'uid2'` in the `control/mode` rule.

Only your Google UID can write to `control/mode/`. ESP32 (Email/Password) can read control and write to `control/clear_error`.

### 7. Run locally
```bash
npm run dev
```
Visit [http://localhost:3000](http://localhost:3000)

---

## Project Structure

```
dashboard/
├── app/
│   ├── layout.tsx          # Root layout (fonts, metadata)
│   ├── page.tsx            # Main dashboard page
│   └── globals.css         # Global styles + Tailwind
├── components/
│   ├── TankVisual.tsx      # Animated tank level graphic
│   ├── ModeControls.tsx    # AUTO / FORCE_ON / FORCE_OFF buttons
│   ├── HistoryChart.tsx    # Recharts area chart (level + flow)
│   ├── StatCard.tsx        # Metric display card
│   ├── StatusBar.tsx       # Top connection status bar (uptime, WiFi, badges)
│   ├── DeviceConfigSettings.tsx  # Gear icon — calibration & thresholds
│   └── NotificationSettings.tsx # Bell icon — email alert preferences
├── lib/
│   ├── firebase.ts         # Firebase init + Google Auth
│   ├── types.ts            # TypeScript interfaces
│   ├── usePumpData.ts      # Real-time data hook
│   └── useDeviceConfig.ts  # Device config read/write
├── .env.local.example      # Environment variable template
└── README.md
```

---

## Deployment (Vercel — recommended)

```bash
npm install -g vercel
vercel
```

Add your environment variables in:
Vercel Dashboard → Project → Settings → Environment Variables

Or use the Vercel CLI:
```bash
vercel env add NEXT_PUBLIC_FIREBASE_API_KEY
# repeat for each variable
```

---

## Dashboard Features

| Feature                | Description                                                       |
|------------------------|-------------------------------------------------------------------|
| Live tank level        | Animated tank graphic; StatCard shows configurable start/stop %   |
| Flow rate              | YF-G1 sensor data; low-flow warning uses configurable threshold   |
| Mode control           | AUTO / FORCE ON / FORCE OFF with instant Firebase push            |
| Dry-run acknowledge    | Red alert banner with ACK; message shows configured timeout (s)   |
| Device config (gear)   | Tank calibration, pump thresholds, safety, sleep schedule, advanced |
| Notifications (bell)   | Dry-run, low level, pump started, overflow alerts                |
| StatusBar              | ESP32 online/offline, uptime, WiFi RSSI, SENSOR/OVERFLOW badges  |
| History chart          | 60-point rolling area chart (level + flow)                        |
| Connection status      | Live/disconnected indicator with last-update time                |
| Responsive layout      | Works on mobile, tablet, and desktop                              |

---

## Security

- Dashboard access requires **Google sign-in**. Unauthorized users are redirected to `/login`.
- Firebase rules restrict **control writes** to your Google UID; ESP32 (Email/Password) can read control and write status.
- Optional: Set `NEXT_PUBLIC_AUTHORIZED_UIDS` to restrict which Google accounts can sign in.

## Safety Notes

- **FORCE ON** bypasses tank level automation. The TOR thermal protection on the physical hardware remains active regardless of dashboard state.
- **Dry-run lockout** can only be cleared via the ACK button after physically verifying the water source.
- **clear_error** acknowledges dry-run, sensor failure, and overflow errors in one action.

---

*See `docs/ENHANCEMENT_PLAN.md` and `docs/IMPLEMENTATION_VERIFICATION.md` for implemented features.*
