## Deployment Guide — v1.0.0

### Audience

This is the end-to-end deployment guide for operators and engineers deploying:

- Firebase project (RTDB + Auth + rules)
- Dashboard (Next.js)
- Cloud Functions (optional notifications)
- ESP32 master firmware
- ESP8266 sensor node firmware
- RS‑485 physical link

---

## 0) Safety and “don’t energize yet”

- Do not energize the 220V motor circuit until:
  - Low-voltage wiring is verified
  - The dashboard shows live status updates
  - Emergency stop and lockouts behave as expected

The TOR (thermal overload relay) must be correctly wired and set to motor FLA.

---

## 1) Prerequisites

### Accounts

- Firebase project (Realtime Database enabled)
- Google sign-in enabled for dashboard users
- Email/Password user created for ESP32 firmware
- (Optional) Resend account for email notifications

### Tools

- Node.js (dashboard + Firebase CLI + functions)
- Firebase CLI (`npm i -g firebase-tools`)
- Arduino IDE or PlatformIO for firmware builds/flashing

### Secrets

- Dashboard secrets live in `dashboard/.env.local` (never commit)
- Firmware secrets live in `firmware/**/secrets.h` (never commit)

---

## 2) Firebase setup

### 2.1 Create/enable Realtime Database

1. Firebase Console → Build → Realtime Database → Create database
2. Choose region
3. Start locked (rules will be deployed)

### 2.2 Enable Authentication

1. Firebase Console → Build → Authentication → Sign-in method
2. Enable:
   - Email/Password (ESP32)
   - Google (dashboard)
3. Create an Email/Password user for the ESP32 credentials used in firmware `secrets.h`.

### 2.3 Deploy database rules

From repo root:

```bash
firebase login
firebase use <your-project-id>
firebase deploy --only database
```

Rules are defined in `database.rules.json`.

### 2.4 Bootstrap the first admin

All control writes require admin rights based on:

```text
/pump_system/config/admins/{uid} = true
```

Bootstrap by setting your UID to true in the RTDB console (or using your bootstrap tooling, if present in this repo).

---

## 3) Dashboard deployment

### 3.1 Configure environment

Create `dashboard/.env.local` from `dashboard/.env.local.example` and set:

- `NEXT_PUBLIC_FIREBASE_API_KEY`
- `NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN`
- `NEXT_PUBLIC_FIREBASE_DATABASE_URL`
- `NEXT_PUBLIC_FIREBASE_PROJECT_ID`
- `NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET`
- `NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID`
- `NEXT_PUBLIC_FIREBASE_APP_ID`

(Optional)

- `NEXT_PUBLIC_AUTHORIZED_UIDS` (restrict sign-in to known users)
- `NEXT_PUBLIC_TANK_LABEL`
- `NEXT_PUBLIC_FIREBASE_VAPID_KEY` (push notifications)

### 3.2 Validate locally

```bash
cd dashboard
npm install
npm run validate
```

### 3.3 Deploy

Deploy on Vercel (recommended) or another Next.js host. Ensure the dashboard host is added to Firebase Auth “Authorized domains”.

---

## 4) Cloud Functions (optional notifications)

1. Install and build functions:

```bash
cd functions
npm install
npm run build
```

2. Configure secrets (e.g. Resend):

```bash
firebase functions:secrets:set RESEND_API_KEY
```

3. Deploy functions:

```bash
cd ..
firebase deploy --only functions
```

4. In dashboard, each user sets notification preferences under:

```text
/pump_system/config/notifications_by_user/{uid}
```

---

## 5) Firmware flashing

### 5.1 ESP32 master firmware

- Arduino: open `firmware/arduino_smart_water_pump_controller/arduino_smart_water_pump_controller.ino`
- PlatformIO: `firmware/platformio_smart_water_pump_controller/`

Copy secrets template:

- `firmware/arduino_smart_water_pump_controller/secrets.h.example` → `secrets.h`

Flash, then open Serial Monitor at 115200 baud and verify:

- Wi‑Fi connects
- Firebase initializes
- RS‑485 polling logs appear

### 5.2 ESP8266 sensor node firmware

- Arduino: `firmware/arduino_sensor_node/arduino_sensor_node.ino`
- PlatformIO: `firmware/platformio_sensor_node/`

Flash and verify it responds over RS‑485 (master logs should show valid frames).

---

## 6) RS‑485 link commissioning (field checklist)

Deployment requirements for stable RS‑485 over long cable runs:

- Correct A/B polarity at both ends
- Common ground reference between nodes (or use isolated transceiver)
- Proper termination and biasing for cable length/topology
- DE/RE wiring correct (half-duplex direction control)

Run tests:

- Unplug RS‑485: pump must **not** start; running pump must stop (unless bypass is active).
- Induce invalid frames/noise: master must reject CRC failures and continue using last good values without unsafe actuation.

---

## 7) Pre-energization functional safety test (before 220V)

With the pump power circuit **disconnected**:

- Verify dashboard can change `mode` and set `manual_desired`.
- Trigger `emergency_stop` and confirm:
  - Status shows `emergency_stop_latched=true`
  - Pump relay output is OFF
- Reset stop and confirm unlatch.
- Simulate dry-run (flow low while running) and confirm lockout requires `clear_error`.

Only after this passes should the motor circuit be energized.

