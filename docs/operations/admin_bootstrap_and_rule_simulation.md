# Admin Bootstrap & Rule Simulation
## Security validation for dynamic database rules (v3.0)

---

## 1. Bootstrap first admin UID

After deploying `database.rules.json`, **no user can write to control until** at least one UID is set at `pump_system/config/admins/{uid} = true`. Use one of the methods below.

### Option A: Firebase CLI (manual one-off)

You need the **database URL** and **Authentication UID** of the user who should be admin.

1. Get the UID: Firebase Console → **Authentication** → **Users** → copy the **User UID** of the Google account you use for the dashboard.
2. Using the Firebase REST API with a credential that has admin access (e.g. a service account), or the **Realtime Database** tab:
   - Go to **Realtime Database** → **Data**.
   - Navigate to `pump_system` → `config` → `admins`.
   - If `admins` does not exist, click **+** to add a child `admins` (object).
   - Under `admins`, add a child with key = **your UID** and value = **true**.

### Option B: Node.js script (recommended for automation)

From repo root, with a **service account key** (JSON) for the Firebase project:

```bash
# One-time: install dependencies in scripts/
cd scripts
npm install

# Set path to service account key (download from Firebase Console → Project Settings → Service accounts)
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/serviceAccountKey.json"

# Bootstrap: replace YOUR_UID with the UID from Authentication → Users
node bootstrap-admin.js YOUR_UID
```

Or with explicit keyfile (from repo root):

```bash
cd scripts && node bootstrap-admin.js --keyfile /path/to/serviceAccountKey.json YOUR_UID
```

**Success:** Script prints `Admin bootstrap OK: pump_system/config/admins/YOUR_UID = true`.

### Option C: One-liner from functions directory

If you already have `functions/` with `firebase-admin` installed and a service account key at `functions/serviceAccountKey.json`:

```bash
cd functions
node -e "
const admin = require('firebase-admin');
admin.initializeApp({ credential: admin.credential.cert(require('./serviceAccountKey.json')) });
const uid = process.argv[1] || 'YOUR_UID';
admin.database().ref('pump_system/config/admins').child(uid).set(true).then(() => { console.log('Admin set:', uid); process.exit(0); }).catch(e => { console.error(e); process.exit(1); });
" YOUR_UID
```

Replace `YOUR_UID` with the UID from Firebase Console → Authentication → Users.

---

## 2. Rule simulation: non-admin write must be denied

To confirm that `pump_system/control/mode` is **denied** for an authenticated user who is **not** in `pump_system/config/admins`:

### Option A: Firebase Console Rules Playground

1. Firebase Console → **Realtime Database** → **Rules** tab → **Rules Playground** (or **Simulator**).
2. **Location:** `/pump_system/control/mode`
3. **Operation:** **Write**
4. **Type:** **Authenticated**; choose a **User** that is **not** listed under `pump_system/config/admins` (or use a test user that has no admin entry).
5. **Value:** `"AUTO"` (string).
6. Run the simulation.
7. **Expected:** **Denied** (simulation shows that the write would be rejected).

### Option B: Manual test with second Google account

1. Add a second Google user in Firebase Authentication (e.g. a test account).
2. Do **not** add that user’s UID to `pump_system/config/admins`.
3. In an incognito window, sign in to the dashboard with that second account.
4. Try to change mode (e.g. switch to FORCE_OFF or AUTO).
5. **Expected:** The write fails; UI may show an error or the mode does not change. In the browser dev tools (Network or Console), you should see a Firebase permission-denied error.

### Option C: REST API with ID token

With an ID token for a **non-admin** user (e.g. from the dashboard after signing in with a test account):

```bash
# Replace DATABASE_URL and ID_TOKEN
curl -X PUT -d '"AUTO"' \
  "https://YOUR_PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app/pump_system/control/mode.json?auth=ID_TOKEN"
```

**Expected:** `401` or `403` and a body indicating permission denied (e.g. `"Permission denied"`). A successful write would return `200` and the value; with dynamic rules and a non-admin UID, the request must be denied.

---

## 3. Verification checklist

| Step | Action | Expected |
|------|--------|----------|
| 1 | Deploy rules | `firebase deploy --only database` |
| 2 | Bootstrap admin | Set `pump_system/config/admins/{uid} = true` for at least one UID |
| 3 | Sign in as admin | Dashboard loads; mode change succeeds |
| 4 | Rules simulation | Write to `control/mode` as non-admin → **Denied** |
| 5 | ESP32 status write | ESP32 (Email/Password) can still write to `pump_system/status` |
