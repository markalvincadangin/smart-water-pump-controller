# SmartFlow — Web Dashboard

Next.js 14 + Firebase RTDB real-time dashboard for the ESP32 pump controller.  
Access is protected by **Google sign-in**; only authorized users can view and control the pump.

**Current specs:** See [docs/specs/dashboard.md](../docs/specs/dashboard.md) and [docs/specs/firmware.md](../docs/specs/firmware.md).

---

## Tech Stack

| Layer      | Technology                              |
|------------|-----------------------------------------|
| Framework  | Next.js 14 (App Router)                 |
| Language   | TypeScript                              |
| Styling    | Tailwind CSS                            |
| Charts     | Recharts                                |
| Icons      | Local SVG asset set + selected Lucide UI icons |
| Database   | Firebase Realtime Database              |
| Auth       | Firebase Google Authentication          |
| Fonts      | Geist · Geist Mono (Vercel/Google)      |

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
Edit `.env.local` and fill in your Firebase credentials. Optionally:

- Set `NEXT_PUBLIC_AUTHORIZED_UIDS` (comma-separated Firebase UIDs) to restrict which Google accounts can sign in.
- Set `NEXT_PUBLIC_TANK_LABEL` to customize the tank label text shown under the logo (defaults to `"SmartFlow · 660L Tank"` if omitted).

**Where to find Firebase values:**  
Firebase Console → Project Settings → General → Your apps → Web → SDK setup and configuration

**Push notifications (optional):** Add `NEXT_PUBLIC_FIREBASE_VAPID_KEY` from Firebase Console → Project Settings → Cloud Messaging → Web Push certificates → Generate key pair. See `docs/operations/NOTIFICATIONS_SETUP.md` section 4b.

For the full RTDB contract and dashboard behavior reference, see:

- `docs/specs/firmware.md`
- `docs/specs/dashboard.md`

### 5. Enable Firebase services
In the Firebase Console:
1. **Realtime Database** → Create database → Start in test mode
2. **Authentication** → Sign-in methods:
   - **Email/Password** → Enable (ESP32 uses this)
   - **Google** → Enable (dashboard uses this)

### 6. Set Firebase security rules
Deploy rules from project root: `firebase deploy --only database`

Before deploying, review `database.rules.json`. The recommended setup is:

- Admin access is controlled via the admins map:

  ```
  pump_system/config/admins/{uid} = true
  ```

- Only admin UIDs can write to `control/mode/` and `control/clear_error`. ESP32 (Email/Password) can read control and write status.
- A small, hardcoded UID allowlist may be kept temporarily for bootstrap, but the admins map should be the long-term single source of truth.

### 7. Run locally
```bash
npm run dev
```
Visit [http://localhost:3000](http://localhost:3000)

---

## Validation (recommended)

Run the full local validation (lint + build + Lighthouse CI):

```bash
npm run validate
```

---

## Project Structure

```
dashboard/
├── app/
│   ├── api/firebase-messaging-sw/  # Dynamic FCM service worker
│   ├── layout.tsx          # Root layout (fonts, metadata)
│   ├── manifest.ts         # PWA manifest (installable app)
│   ├── page.tsx            # Main dashboard page
│   └── globals.css         # Global styles + Tailwind
├── components/
│   ├── TankVisual.tsx      # Animated tank level + start/stop reference lines, MANUAL_OFF + stale/estimate styling
│   ├── ModeControls.tsx    # AUTO / MANUAL / COUNTDOWN selector
│   ├── RunControls.tsx     # MANUAL ON/OFF toggle, Semi-Auto Timer (COUNTDOWN), countdown timer/stop/add-time, inline Clear Error
│   ├── HistoryChart.tsx    # Dual Y-axis area chart (level + flow)
│   ├── StatCard.tsx        # Metric display card
│   ├── StatusBar.tsx       # Top bar: connectivity, mode, warning badges
│   ├── DashboardHeader.tsx # Title, pump status badge, overflow menu
│   ├── DashboardMainGrid.tsx   # Main layout: tank + stats + controls
│   ├── DashboardHistorySection.tsx # History chart wrapper
│   ├── DashboardSystemInfo.tsx     # System info panel (heap, sensors, connectivity)
│   ├── DashboardSkeleton.tsx       # Loading skeleton for main grid
│   ├── ActivityPanel.tsx   # Audit log / activity feed
│   ├── CollapsibleSection.tsx      # Expandable section wrapper
│   ├── OverflowMenu.tsx    # Three-dot menu for secondary actions
│   ├── DeviceConfigSettings.tsx    # Gear icon — calibration & thresholds
│   ├── NotificationSettings.tsx    # Bell icon — email + push alert preferences
│   ├── InfoTooltip.tsx     # Reusable help tooltip (hover/tap)
│   ├── AppIcon.tsx         # Typed map to public SVG chrome icons (heroicons)
│   └── InstallPrompt.tsx   # PWA install banner
├── lib/
│   ├── firebase.ts         # Firebase init + Google Auth
│   ├── fcm.ts              # FCM push token helpers
│   ├── types.ts            # TypeScript interfaces (PumpStatus, PumpControl, DeviceConfig)
│   ├── usePumpData.ts      # Real-time RTDB data hook
│   ├── useDeviceConfig.ts  # Device config read/write
│   ├── usePresence.ts      # Online/offline presence tracking
│   ├── audit.ts            # Audit log helpers
│   ├── alertRanking.ts     # Alert priority ranking
│   ├── faultCodes.ts       # Fault code descriptions
│   └── useNotificationConfig.ts
├── public/logos/           # Brand SVGs (manifest + metadata: brandmark, wordmark, combinationmark)
├── public/icons/heroicons/ # App chrome SVGs (via AppIcon.tsx)
├── public/favicon.ico      # Browser tab icon
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
| Mode control           | AUTO / MANUAL (intent ON/OFF) / COUNTDOWN with instant Firebase push |
| Smart runs             | Manual and countdown runs with pill-button duration selector; auto-stop on timer, tank full, or safety fault |
| Dry-run acknowledge    | Red alert banner with ACK; message shows configured timeout (s)   |
| Device config (gear)   | Tank calibration, pump thresholds, safety, sleep schedule, advanced — with **tooltips** (hover/tap for help) |
| Notifications (bell)   | Email + **push** (phone/browser); dry-run, low level, pump started, overflow alerts — with **tooltips** |
| StatusBar              | ESP32 online/offline, uptime, WiFi RSSI, mode, error/warning badges |
| System info panel      | ESP32 heap, sensor health, Firebase connectivity, pump telemetry |
| History chart          | 60-point rolling area chart (level + flow)                        |
| Connection status      | Live/disconnected indicator with last-update time                |
| Responsive layout      | Works on mobile, tablet, and desktop                              |
| **PWA (installable)**  | Add to Home Screen / Install app — native-like experience on mobile |

---

## Security

- Dashboard access requires **Google sign-in**. Unauthorized users are redirected to `/login`.
- Firebase rules restrict **control writes** to admin UIDs; ESP32 (Email/Password) can read control and write status.
- Optional: Set `NEXT_PUBLIC_AUTHORIZED_UIDS` to restrict which Google accounts can sign in. Admins for advanced settings and privileged controls are managed via the Realtime Database at `pump_system/config/admins/{uid} = true`.

## Safety Notes

- Dashboard does **not** provide any “override safety” controls. All safety protections remain enforced by firmware.
- **Dry-run lockout** can only be cleared via the ACK button after physically verifying the water source.
- **clear_error** acknowledges dry-run, sensor failure, and overflow errors in one action.

---

Historical release notes remain under `docs/releases/` but may not match current code.
