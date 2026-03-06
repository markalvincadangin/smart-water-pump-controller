# Complete Deployment Guide — Smart Water Pump Controller

**Use this document when you have the go signal to deploy the full system.**  
It covers Firebase, Cloud Functions, Dashboard, and ESP32 firmware in the correct order.

---

## Go signal checklist

Before starting, confirm:

- [ ] **Hardware** — Enclosure wired per `hardware/wiring_notes.md`; no 220V power applied until the **Pre-Energization checklist** (Phase 7) is done.
- [ ] **Accounts** — Google (for dashboard), Firebase project (Blaze plan if using Functions), Resend (for email alerts).
- [ ] **Tools** — Node.js 18+ (for dashboard and Firebase CLI), Arduino IDE 2.x with ESP32 board support, Firebase CLI (`npm install -g firebase-tools`).
- [ ] **Secrets** — You will create `secrets.h` and `.env.local` from examples; **never commit** these files.

---

## Deployment order (high level)

| Phase | What | Where |
|-------|------|--------|
| 1 | Firebase project & Realtime Database | Firebase Console |
| 2 | Authentication (Email/Password + Google) | Firebase Console |
| 3 | Database rules | `firebase deploy --only database` |
| 4 | Cloud Functions + Resend | `firebase deploy --only functions` |
| 5 | Dashboard (env + build + host) | Vercel or other |
| 6 | ESP32 firmware | Arduino IDE → Upload |
| 7 | Post-deploy smoke test | Browser + Serial |

---

## Phase 1 — Firebase project and Realtime Database

1. Go to [Firebase Console](https://console.firebase.google.com) and create or select your project.
2. **Realtime Database**
   - **Build → Realtime Database → Create database**
   - Choose a region (e.g. **asia-southeast1** — same as Functions later).
   - Start in **locked mode** (rules will be deployed in Phase 3).
3. Copy the **database URL** (e.g. `https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app`). You will need it for:
   - Dashboard `.env.local` → `NEXT_PUBLIC_FIREBASE_DATABASE_URL`
   - Firmware `secrets.h` → `DATABASE_URL`
4. **Project settings (Web app)**
   - Project Settings (gear) → **General** → **Your apps** → add/select Web app.
   - Copy: **API Key**, **Auth domain**, **Project ID**, **Storage bucket**, **Messaging sender ID**, **App ID** for dashboard `.env.local`.

---

## Phase 2 — Authentication

1. **Build → Authentication → Sign-in method**
2. Enable **Email/Password** (used by the ESP32).
3. Enable **Google** (used by the dashboard).
4. **Create ESP32 user**
   - **Authentication → Users → Add user**
   - Email: e.g. `pump-esp32@yourdomain.com`
   - Password: strong password (store in `secrets.h` as `FIREBASE_EMAIL` and `FIREBASE_PASSWORD`).
5. **Authorized domains**
   - **Authentication → Settings → Authorized domains**
   - Ensure `localhost` is present for local dev.
   - After deploying the dashboard, add your host (e.g. `your-app.vercel.app` or your custom domain).

---

## Phase 3 — Database rules

1. **Set your Firebase project in CLI**
   ```bash
   cd path/to/smart-water-pump-controller
   firebase login
   firebase use your-project-id
   ```

2. **Edit UIDs in rules**
   - Open `database.rules.json`.
   - Replace the example UIDs in `control/mode` and `config/device` with your own.
   - To get your UID: **Firebase Console → Authentication → Users** → copy the UID of your Google account(s) that may control the pump.
   - Format: `auth.uid === 'YOUR_UID_1' || auth.uid === 'YOUR_UID_2'` (add more if needed).

3. **Deploy rules**
   ```bash
   firebase deploy --only database
   ```
   - Confirm in Firebase Console → Realtime Database → Rules that the new rules are active.

---

## Phase 4 — Cloud Functions (email notifications)

**Prerequisites:** Firebase Blaze (pay-as-you-go) plan, [Resend](https://resend.com) account.

1. **Resend**
   - Sign up at resend.com, verify email.
   - **API Keys → Create API Key** → copy key (starts with `re_`).

2. **Functions build and secret**
   ```bash
   cd functions
   npm install
   npm run build
   ```
   - Set the Resend key (you will be prompted to paste it):
   ```bash
   firebase functions:secrets:set RESEND_API_KEY
   ```

3. **Optional:** Custom “From” email  
   Create `functions/.env` (do not commit) with:
   ```env
   RESEND_FROM_EMAIL=Smart Water Pump <alerts@yourdomain.com>
   ```
   (If omitted, Resend sends from `onboarding@resend.dev`.)

4. **Deploy Functions**
   ```bash
   cd ..
   firebase deploy --only functions
   ```
   - Confirm in Firebase Console → Functions that `onStatusChange` is deployed and region matches your database (e.g. asia-southeast1).

---

## Phase 5 — Dashboard

### 5.1 Local setup and env

1. **Install and env**
   ```bash
   cd dashboard
   npm install
   cp .env.local.example .env.local
   ```

2. **Edit `.env.local`** with values from Firebase Console (Phase 1 & 2):

   | Variable | Where to get it |
   |----------|------------------|
   | `NEXT_PUBLIC_FIREBASE_API_KEY` | Project Settings → General → Your apps → apiKey |
   | `NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN` | authDomain |
   | `NEXT_PUBLIC_FIREBASE_DATABASE_URL` | Realtime Database URL (Phase 1) |
   | `NEXT_PUBLIC_FIREBASE_PROJECT_ID` | projectId |
   | `NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET` | storageBucket |
   | `NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID` | messagingSenderId |
   | `NEXT_PUBLIC_FIREBASE_APP_ID` | appId |
   | `NEXT_PUBLIC_AUTHORIZED_UIDS` | Optional: comma-separated Firebase UIDs, or leave empty to allow any signed-in user |

3. **Verify build**
   ```bash
   npm run build
   ```
   - Fix any TypeScript or build errors before deploying.

### 5.2 Deploy to Vercel

1. Push your repo to GitHub (ensure no `secrets.h`, `.env.local`, or `serviceAccountKey.json` are committed).

2. [Vercel](https://vercel.com) → **Add New Project** → import your repo.

3. **Environment variables**  
   Add every `NEXT_PUBLIC_*` from `.env.local` in the Vercel project settings (Production, Preview if desired).

4. **Deploy**  
   Trigger deploy (or push to main). Note the URL (e.g. `your-app.vercel.app`).

5. **Firebase Authorized domains**  
   Firebase Console → **Authentication → Settings → Authorized domains** → **Add domain** → add `your-app.vercel.app` (and custom domain if used).

6. **Sign-in test**  
   Open the dashboard URL → sign in with Google. If `NEXT_PUBLIC_AUTHORIZED_UIDS` is set, the signed-in user’s UID must be in that list or sign-in may be rejected.

---

## Phase 6 — ESP32 firmware

### 6.1 Arduino IDE setup

1. **Arduino IDE 2.x** — [arduino.cc/en/software](https://www.arduino.cc/en/software).

2. **ESP32 board support**
   - **File → Preferences** → Additional Boards Manager URLs, add:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - **Tools → Board → Boards Manager** → search **esp32** (Espressif) → Install (2.0.11 or later).

3. **Libraries** (Sketch → Include Library → Manage Libraries)
   - **Firebase ESP Client** by Mobizt — install (≥ 4.4.14).
   - **ArduinoJson** by Benoit Blanchon — install **v6.x** (e.g. 6.21.5). Do **not** use v7 (API incompatible).

### 6.2 Secrets and board settings

1. **Create `secrets.h`**
   - In the same folder as `smart_pump_controller.ino`:
     - Copy `firmware/smart_pump_controller/secrets.h.example` to `firmware/smart_pump_controller/secrets.h`.
   - Edit `secrets.h` and set:
     - `WIFI_SSID`, `WIFI_PASSWORD` — your 2.4 GHz WiFi (ESP32 does not support 5 GHz).
     - `API_KEY`, `DATABASE_URL` — from Firebase (Project Settings + Realtime Database URL).
     - `FIREBASE_EMAIL`, `FIREBASE_PASSWORD` — the Email/Password user you created in Phase 2 for the ESP32.

2. **Board settings (Tools menu)**

   | Setting | Value |
   |--------|--------|
   | Board | **ESP32 Dev Module** |
   | Upload Speed | 115200 |
   | CPU Frequency | 240MHz (WiFi/BT) |
   | Flash Frequency | 80MHz |
   | Flash Mode | QIO |
   | Flash Size | 4MB (32Mb) |
   | Partition Scheme | Default 4MB with spiffs |
   | PSRAM | Disabled |
   | Port | Your ESP32 COM port |

### 6.3 Open, verify, upload

1. **Open**  
   **File → Open** → `firmware/smart_pump_controller/smart_pump_controller.ino`  
   (The `.ino` must stay inside the folder of the same name.)

2. **Verify**  
   Click **Verify** (✓). Resolve any compile errors (e.g. wrong ArduinoJson version, missing `secrets.h`).

3. **Upload**
   - Connect ESP32 via USB.
   - **Tools → Port** → select the correct COM port.
   - Click **Upload** (→).
   - If you see “Failed to connect”: hold the **BOOT** button on the ESP32, click Upload again, release BOOT when “Connecting…” appears.

4. **Serial Monitor**
   - **Tools → Serial Monitor** → 115200 baud.
   - Confirm: `[WIFI] Connected!`, `[FIREBASE] Initialized`, and periodic `[SENSOR]` / `[FIREBASE] Status pushed` before closing.

---

## Phase 7 — Post-deploy smoke test

Use this checklist after everything is deployed.

### Dashboard

- [ ] Open dashboard URL → sign in with Google.
- [ ] Page loads; you see status (or “ESP32 offline” if the device is not pushing yet).

### Device config

- [ ] Click **gear icon** (Device config).
- [ ] **Seed defaults (if empty)** → **Save**.
- [ ] When ESP32 is online, it picks up config within ~30 seconds (check Serial or next status update).

### Notifications

- [ ] Click **bell icon** → enable notifications, set your email, choose alert types → **Save**.
- [ ] Trigger a test (see `docs/NOTIFICATIONS_SETUP.md`): e.g. in Firebase Console set `pump_system/status/is_running` to `true` (after setting to `false` first) for “Pump started” email.
- [ ] Confirm email received and Resend dashboard shows delivery.

### Control and ESP32

- [ ] Change mode (AUTO / FORCE_ON / FORCE_OFF) from dashboard.
- [ ] Confirm ESP32 responds (Serial Monitor and/or relay state).

### Pre-energization (before 220V)

- [ ] Complete the **Phase 4 — Pre-Energization Checklist** in the main `README.md` (multimeter, tug test, TOR dial, earth, voltage dividers, etc.).
- [ ] Only then switch the MCB on for the first time.

---

## Quick reference commands

From repo root:

```bash
# Database rules
firebase deploy --only database

# Cloud Functions
cd functions && npm run build && cd ..
firebase deploy --only functions

# Dashboard (local)
cd dashboard && npm run build
```

---

## Security reminders

- **Never commit:** `secrets.h`, `.env.local`, `functions/.env`, `serviceAccountKey.json`, or any file containing API keys or passwords.
- **Restrict control:** Keep `database.rules.json` so only your UIDs can write `control/mode` and `config/device`.
- **Authorized domains:** Only add domains you control to Firebase Authentication.

---

## Where to get help

| Topic | File |
|-------|------|
| Hardware wiring | `hardware/wiring_notes.md`, `hardware/enclosure_layout.md` |
| Firmware calibration, Serial, troubleshooting | `firmware/README.md` |
| Dashboard setup, env vars | `dashboard/README.md` |
| Notifications (Resend, testing alerts) | `docs/NOTIFICATIONS_SETUP.md` |
| Short deploy checklist | `docs/DEPLOY_CHECKLIST.md` |
| Device config from DB | `docs/FIRMWARE_CONFIG_FROM_DATABASE.md` |

---

*Smart Water Pump Controller — full deployment guide. Follow phases in order for a complete, production-ready deployment.*
