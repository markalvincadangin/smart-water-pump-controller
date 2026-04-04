# Notifications Setup

The Smart Water Pump System can send alerts via **email** and/or **push notifications** (directly to your phone or browser, like YouTube or Facebook) for high-risk events:

| Event | Description |
|-------|-------------|
| **Dry-Run Lockout** | No flow detected (configurable, default 30s) while pump was running |
| **Low Tank Level** | Water level drops to your threshold (default 20%) |
| **Pump Started** | Pump transitions from off to on |
| **Overflow Protection** | Max runtime exceeded — pump ran beyond limit without reaching stop level |

---

## Prerequisites

- Firebase project with **Blaze (pay-as-you-go)** plan (required for Cloud Functions)
- [Resend](https://resend.com) account (free tier: 100 emails/day)

---

## 1. Create Resend Account

1. Sign up at [resend.com](https://resend.com)
2. Verify your email
3. Go to **API Keys** → Create API Key
4. Copy the key (starts with `re_`)

**Optional:** Add and verify your domain for a custom "From" address. Without this, emails send from `onboarding@resend.dev`.

---

## 2. Deploy Cloud Functions

From the **project root**:

```bash
cd functions
npm install
npm run build

# Set Resend API key (you'll be prompted to enter it)
firebase functions:secrets:set RESEND_API_KEY

# Optional: custom "from" address — create functions/.env (do not commit) with:
# RESEND_FROM_EMAIL=Smart Water Pump <alerts@yourdomain.com>

# Deploy
cd ..
firebase deploy --only functions
```

When prompted for `RESEND_API_KEY`, paste your Resend API key. It is stored in Google Cloud Secret Manager.

---

## 3. Deploy Database Rules

Database rules must allow the paths used by the Cloud Function and dashboard:

```bash
firebase deploy --only database
```

Ensure `database.rules.json` includes:

- `pump_system/config/notifications_by_user/$uid` — read/write for `auth.uid === $uid`
- `pump_system/config/notification_last_sent` — no client read/write (Functions only)
- `pump_system/audit/events` and `pump_system/presence` — optional but recommended for multi-user activity/presence indicators (dashboard writes best-effort)

See `docs/archive/releases/v2.0/deploy.md` (historical) for full rules and UID setup.

---

## 4. Configure in Dashboard

1. Open the dashboard and sign in with your Google account
2. Click the **bell icon** (🔔) in the header
3. Enable notifications and enter **your** email (for email alerts)
4. **Push notifications (optional):** Click "Enable push on this device" — allow notifications when prompted — then **Save**. Alerts will appear on your phone/browser even when the app is closed.
5. Choose which alerts you want (dry-run, low level, pump started, overflow)
6. Set low-level threshold (default 20%)
7. Click **Save**

Each signed-in user has their **own** notification settings stored under:

```
pump_system/config/notifications_by_user/{uid}
```

Alerts are sent to every user who has:

- `enabled: true`
- At least one of: non-empty `email`, or `fcmTokens` with at least one device

---

## 4b. Push Notifications (FCM) — Optional

Push notifications are sent directly to your phone or browser. **Requirements:** HTTPS, supported browser (Chrome, Firefox, Safari 16+, Edge), and `NEXT_PUBLIC_FIREBASE_VAPID_KEY` configured.

### Setup

1. **Firebase Console** → Project Settings → **Cloud Messaging** → **Web Push certificates**
2. If no key pair exists, click **Generate key pair** — copy the VAPID key
3. Add to `dashboard/.env.local`:
   ```env
   NEXT_PUBLIC_FIREBASE_VAPID_KEY=your_vapid_key_here
   ```
4. Add the same variable in Vercel (or your host) if deployed
5. Rebuild and redeploy the dashboard
6. Deploy Cloud Functions: `firebase deploy --only functions` (Functions send push alongside email)

### Enable on Device

1. Open the dashboard in a supported browser (HTTPS required)
2. Bell icon → **Enable push on this device** → allow notifications when prompted
3. Click **Save**

Push works best when the dashboard is **installed as an app** (Add to Home Screen). See `docs/archive/releases/v2.0/deploy.md` Phase 5 step 7 (historical) for PWA install.

---

## Testing

The Cloud Function runs when `/pump_system/status` is written. You can trigger each alert by writing test data.

### Option A: Firebase Console (quickest)

1. Open [Firebase Console](https://console.firebase.google.com) → your project → **Realtime Database**
2. Navigate to `pump_system/status`
3. **Pump Started** — Set `is_running` to `false`, then after a moment set it to `true`. The function fires on the off→on transition.
4. **Low Tank** — Set `water_level_percent` to a value at or below your threshold (e.g. `15` if threshold is 20%)
5. **Dry-Run** — Set `is_error` to `true` (with notifications enabled and Dry-Run alert checked)
6. **Overflow** — Set `is_overflow_error` to `true` (with notifications enabled and Overflow alert checked)

> **Throttling:** Each alert type sends at most once per 15 minutes. Wait between tests or use different alert types.

### Option B: Real hardware (ESP32)

- **Pump Started** — Use the dashboard or ESP32 to turn the pump on; email should arrive within a few seconds
- **Low Tank** — Let the tank drain below your threshold (or temporarily lower the threshold in settings)
- **Dry-Run** — Run the pump with no flow (e.g. closed valve) for 30+ seconds so dry-run protection trips

### Verify

- **Resend Dashboard** → Emails — check delivery status
- **Firebase Console** → Functions → Logs — confirm `onStatusChange` runs
- If no email: confirm notifications are enabled in the dashboard, your email is set, and the relevant alert type is checked

---

## Throttling

To avoid spam, each alert type is limited to **once per 15 minutes** per user. For example, if the tank stays below 20%, you get one low-level email and then none for 15 minutes.

---

## Troubleshooting

| Issue | Check |
|-------|--------|
| No emails received | Resend dashboard delivery status; Cloud Function logs in Firebase Console |
| "RESEND_API_KEY not set" in logs | Run `firebase functions:secrets:set RESEND_API_KEY` and redeploy with `firebase deploy --only functions` |
| Sign-in fails on Vercel | Ensure `NEXT_PUBLIC_AUTHORIZED_UIDS` is set (or empty to allow any signed-in user) and Firebase Authorized domains includes your dashboard host |
| Push not working | `NEXT_PUBLIC_FIREBASE_VAPID_KEY` set; dashboard on HTTPS; browser allows notifications; check DevTools → Application → Service Workers |
| "Enable push" greyed out or missing | Browser may not support FCM (HTTPS + Push API); VAPID key not set |

---

*See `docs/archive/releases/v2.0/deploy.md` (historical) for full deployment order including Functions and dashboard.*
