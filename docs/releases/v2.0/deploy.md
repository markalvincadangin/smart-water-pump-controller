# Deployment Guide · v2.0

End‑to‑end deployment guide for the Smart Water Pump Controller, aligned with
firmware/dashboard design **v2.0**.

This file consolidates and supersedes the older `docs/DEPLOY_GUIDE.md`.

---

## 1. Prerequisites

- **Hardware** wired and tested per `hardware/*` docs (no 220V power until final checks).
- **Accounts**:
  - Google account for dashboard sign‑in.
  - Firebase project (Blaze plan if using Cloud Functions).
  - Resend account for email notifications (optional but recommended).
- **Tools**:
  - Node.js 18+ (dashboard, Firebase CLI).
  - Arduino IDE 2.x (or PlatformIO) with ESP32 board support.
  - Firebase CLI: `npm install -g firebase-tools`.
- **Secrets**:
  - Will create `firmware/.../secrets.h` and `dashboard/.env.local` from provided examples.
  - Never commit these files.

---

## 2. High‑Level Phases

| Phase | What                                 | Where                 |
|-------|--------------------------------------|-----------------------|
| 1     | Firebase project + Realtime Database | Firebase Console      |
| 2     | Authentication (Email/Password, Google) | Firebase Console   |
| 3     | Database rules                       | `firebase deploy`     |
| 4     | Cloud Functions + Resend (optional)  | Firebase + Resend     |
| 5     | Dashboard (env + build + deploy)     | Local + Vercel/host   |
| 6     | ESP32 firmware                       | Arduino IDE / PlatformIO |
| 7     | Post‑deploy smoke test               | Browser + Serial      |

---

## 3. Firebase Project & Realtime Database

1. In [Firebase Console](https://console.firebase.google.com), create or select a project.
2. **Realtime Database**:
   - Build → Realtime Database → **Create database**.
   - Choose region (e.g. `asia-southeast1`).
   - Start in locked mode; rules will be deployed later.
3. Copy the **database URL** (e.g.
   `https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app`).
   You will use this in:
   - `dashboard/.env.local` → `NEXT_PUBLIC_FIREBASE_DATABASE_URL`.
   - `firmware/.../secrets.h` → `DATABASE_URL`.
4. From Project Settings → General → Web app, copy:
   - API key, Auth domain, Project ID, Storage bucket, Messaging sender ID, App ID.
   These populate the `NEXT_PUBLIC_FIREBASE_*` vars in `dashboard/.env.local`.

---

## 4. Authentication

1. Firebase Console → Build → **Authentication → Sign‑in method**:
   - Enable **Email/Password** (used by the ESP32).
   - Enable **Google** (used by the dashboard).
2. Create the **ESP32 service user**:
   - Authentication → Users → Add user.
   - Email: e.g. `pump-esp32@yourdomain.com`.
   - Password: strong password; store in `secrets.h` as `FIREBASE_EMAIL` / `FIREBASE_PASSWORD`.
3. Authorized domains:
   - Authentication → Settings → Authorized domains.
   - Ensure `localhost` is present for local dev.
   - After dashboard deployment, add your dashboard host (e.g. `your-app.vercel.app`).

---

## 5. Database Rules

1. From repo root, log in and select project:
   ```bash
   firebase login
   firebase use your-project-id
   ```
2. Configure **admin access**:
   - `database.rules.json` uses `pump_system/config/admins/{uid}` as the long‑term
     source of truth for admin users.
   - Optionally set a temporary hard‑coded UID allowlist for bootstrap, but prefer
     the `admins` map going forward.
3. Deploy rules:
   ```bash
   firebase deploy --only database
   ```
   Confirm rules are active in Console → Realtime Database → Rules.

---

## 6. Cloud Functions & Notifications (Optional)

If you want email/push notifications for dry‑run, low tank, overflow, etc.:

1. **Resend account**:
   - Sign up at `https://resend.com`, verify email.
   - Create API key (`re_...`).
2. Build Functions and set secret:
   ```bash
   cd functions
   npm install
   npm run build
   firebase functions:secrets:set RESEND_API_KEY
   ```
3. (Optional) `functions/.env`:
   ```env
   RESEND_FROM_EMAIL=Smart Water Pump <alerts@yourdomain.com>
   ```
4. Deploy Functions:
   ```bash
   cd ..
   firebase deploy --only functions
   ```
5. Configure per‑user notification preferences from the dashboard bell icon.

---

## 7. Dashboard (Next.js)

### 7.1 Local setup

```bash
cd dashboard
npm install
cp .env.local.example .env.local
```

Fill in `NEXT_PUBLIC_FIREBASE_*` values and optional:

- `NEXT_PUBLIC_AUTHORIZED_UIDS` — comma‑separated UIDs to restrict access.
- `NEXT_PUBLIC_TANK_LABEL` — custom tank label text.
- `NEXT_PUBLIC_FIREBASE_VAPID_KEY` — for push notifications (see Firebase Console → Cloud Messaging → Web push).

Verify build:

```bash
npm run build
```

### 7.2 Deploy to Vercel (or similar)

1. Push the repo to GitHub (ensure secrets and `secrets.h` are **not** committed).
2. In Vercel, create a new project from the repo.
3. Add all `NEXT_PUBLIC_*` env vars in the project settings.
4. Deploy and note the URL (e.g. `your-app.vercel.app`).
5. Add that URL to Firebase **Authorized domains**.
6. Sign‑in test with Google; ensure authorized UIDs can access the dashboard.

---

## 8. ESP32 Firmware

Follow `firmware/README.md` for full details. Summary:

1. Install Arduino IDE 2.x and ESP32 board support.
2. Copy `secrets.h.example` to `secrets.h` and fill Wi‑Fi + Firebase credentials.
3. Open the correct `.ino` (for Arduino or PlatformIO project).
4. Verify, then upload to the ESP32.
5. Use Serial Monitor to confirm Wi‑Fi and Firebase connectivity and status pushes.

---

## 9. Post‑Deploy Smoke Test

1. Open the dashboard and confirm:
   - Status updates every ~3 seconds.
   - Mode changes and run controls behave as expected.
   - Alerts trigger correctly for simulated dry‑run/overflow conditions.
2. Verify notifications (if enabled) for at least one event (e.g. dry‑run).
3. Confirm that non‑admin accounts cannot perform admin‑only actions.