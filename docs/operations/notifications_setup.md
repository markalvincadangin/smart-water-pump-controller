# Notifications Setup

SmartFlow supports email and web push notifications for critical pump events.

## Supported alert types

- Dry-run lockout
- Low tank level
- Pump started
- Overflow protection event

## Prerequisites

- Firebase project with Realtime Database, Auth, and Cloud Functions enabled
- Blaze plan for production Cloud Functions usage
- Resend account and API key for email delivery
- Dashboard deployed with valid Firebase client environment variables

## 1. Configure email provider (Resend)

1. Create Resend API key.
2. Set secret for Functions:

```bash
cd functions
firebase functions:secrets:set RESEND_API_KEY
```

3. Optional: set custom sender in `functions/.env`:

```env
RESEND_FROM_EMAIL=SmartFlow <alerts@yourdomain.com>
```

## 2. Build and deploy Functions

```bash
cd functions
npm install
npm run build
cd ..
firebase deploy --only functions
```

## 3. Deploy rules

```bash
firebase deploy --only database
```

Required paths:

- `pump_system/config/notifications_by_user/{uid}` user read/write
- `pump_system/config/notification_last_sent` server-only

## 4. Configure user preferences in dashboard

1. Sign in to dashboard.
2. Open notifications panel.
3. Enable notifications.
4. Set email address and alert toggles.
5. Save.

Preferences are stored per user at:

```text
pump_system/config/notifications_by_user/{uid}
```

## 5. Optional push setup (FCM)

1. Generate Web Push VAPID key in Firebase Cloud Messaging settings.
2. Set dashboard env var:

```env
NEXT_PUBLIC_FIREBASE_VAPID_KEY=your_vapid_key
```

3. Redeploy dashboard.
4. Enable push in notification settings and allow browser prompt.

## 6. Validation test

Trigger a safe test event by writing a test status transition in RTDB.

Examples:

- Pump started: toggle `is_running` false -> true
- Low level: set `water_level_percent` below configured threshold
- Dry-run: set `is_error` true in a controlled test context
- Overflow: set `is_overflow_error` true in a controlled test context

Verify:

- Function invocation appears in Functions logs
- Email/push is delivered
- Throttle behavior is respected

## Throttling behavior

Alerts are rate-limited per user and alert type to reduce notification spam.

## Troubleshooting

- Missing email: verify `RESEND_API_KEY` secret and delivery logs.
- Missing push: verify VAPID key, HTTPS origin, browser permission.
- Permission denied: verify rules for `notifications_by_user` path.
