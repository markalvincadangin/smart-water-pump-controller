# Email Notifications Setup

The Smart Water Pump System can send email alerts for high-risk events:

| Event | Description |
|-------|-------------|
| **Dry-Run Lockout** | No flow detected for 30s while pump was running |
| **Low Tank Level** | Water level drops to your threshold (default 20%) |
| **Pump Started** | Pump transitions from off to on |

---

## Prerequisites

- Firebase project with Blaze (pay-as-you-go) plan (required for Cloud Functions)
- Resend account (free tier: 100 emails/day) — [resend.com](https://resend.com)

---

## 1. Create Resend Account

1. Sign up at [resend.com](https://resend.com)
2. Verify your email
3. Go to **API Keys** → Create API Key
4. Copy the key (starts with `re_`)

**Optional:** Add and verify your domain for custom "From" address. Without this, emails send from `onboarding@resend.dev`.

---

## 2. Deploy Cloud Functions

```bash
# From project root
cd functions
npm install
npm run build

# Set Resend API key via Secret Manager (you'll be prompted to enter it)
firebase functions:secrets:set RESEND_API_KEY

# Optional: custom "from" address (create .env in functions/ before deploy)
# RESEND_FROM_EMAIL=Smart Water Pump <alerts@yourdomain.com>

# Deploy
cd ..
firebase deploy --only functions
```

When you run `firebase functions:secrets:set RESEND_API_KEY`, paste your Resend API key when prompted. It is stored securely in Google Cloud Secret Manager.

---

## 3. Deploy Database Rules

```bash
firebase deploy --only database
```

> ⚠ Before deploying: In `database.rules.json`, replace `YOUR_GOOGLE_UID` with your actual UID from Firebase Console → Authentication → Users. For multiple UIDs, use `auth.uid === 'uid1' || auth.uid === 'uid2'`.

---

## 4. Configure in Dashboard

1. Open the dashboard
2. Click the **bell icon** (🔔) in the header
3. Enable notifications and enter your email
4. Choose which alerts to receive
5. Set low-level threshold (default 20%)
6. Click **Save**

---

## Testing

The Cloud Function runs when `/pump_system/status` is written. You can trigger each alert by writing test data.

### Option A: Firebase Console (quickest)

1. Open [Firebase Console](https://console.firebase.google.com) → your project → **Realtime Database**.
2. Navigate to `pump_system/status`.
3. **Pump Started** – Change `is_running` from `false` to `true`. The function fires when it detects this transition, so you may need to first set it to `false`, wait a moment, then set it to `true`.
4. **Low Tank** – Set `water_level_percent` to a value at or below your threshold (e.g. `15` if threshold is 20%).
5. **Dry-Run** – Set `is_error` to `true` (and ensure notifications are enabled with Dry-Run alert checked).

> Throttling: each alert type sends at most once per 15 minutes. Wait between tests or use different alert types.

### Option B: Script with Firebase Admin (Node.js)

From project root:

```bash
cd functions
node -e "
const admin = require('firebase-admin');
const serviceAccount = require('./serviceAccountKey.json'); // download from Firebase Console
admin.initializeApp({ credential: admin.credential.cert(serviceAccount) });
const db = admin.database();

// Simulate pump started (off → on)
db.ref('pump_system/status').set({
  water_level_percent: 50,
  is_running: true,
  flow_rate_lpm: 12.5,
  is_error: false
});
console.log('Wrote status — pump started alert may fire');
"
```

To test low tank, set `water_level_percent: 15` (or below your threshold). To test dry-run, set `is_error: true`.

### Option C: Real hardware (ESP32)

- **Pump Started** – Use the dashboard or ESP32 to turn the pump on. An email should arrive within a few seconds.
- **Low Tank** – Let the tank drain below your threshold (or temporarily lower the threshold in settings).
- **Dry-Run** – Run the pump with no flow (e.g. closed valve) for 30+ seconds so dry-run protection trips.

### Verify

- Check **Resend Dashboard** → Emails for delivery status.
- Check **Firebase Console** → Functions → Logs for `onStatusChange` execution.
- If no email: confirm notifications are enabled in the dashboard, your email is set, and the relevant alert type is checked.

---

## Throttling

To avoid spam, each alert type is limited to **once per 15 minutes**. For example, if the tank stays below 20%, you’ll get one low-level email and then no more for 15 minutes.

---

## Troubleshooting

**No emails received**
- Check Resend dashboard for delivery status
- Confirm secret exists: `firebase functions:secrets:access RESEND_API_KEY`
- Check Cloud Functions logs: Firebase Console → Functions → Logs

**"RESEND_API_KEY not set" in logs**
- Run `firebase functions:secrets:set RESEND_API_KEY` and enter your key when prompted
- Redeploy: `firebase deploy --only functions`
