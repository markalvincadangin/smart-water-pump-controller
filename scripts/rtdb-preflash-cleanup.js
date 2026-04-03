#!/usr/bin/env node
/*
  SmartFlow RTDB Pre-Flash Cleanup

  Purpose:
  - Reset safety-critical control flags before master flash.
  - Align device calibration/config values for safe startup.

  Usage:
  node rtdb-preflash-cleanup.js --apply --keyfile ../path/to/serviceAccountKey.json --database-url https://PROJECT-default-rtdb.REGION.firebasedatabase.app

  Optional:
  - --dry-run (default when --apply is not provided)
  - --use-adc (use Google ADC instead of keyfile)
*/

const admin = require("firebase-admin");
const fs = require("fs");
const path = require("path");

const args = process.argv.slice(2);
const has = (flag) => args.includes(flag);
const getArg = (flag) => {
  const i = args.findIndex((a) => a === flag);
  return i >= 0 ? args[i + 1] : undefined;
};

const apply = has("--apply");
const dryRun = !apply || has("--dry-run");
const useAdc = has("--use-adc");

let keyfile = getArg("--keyfile") || process.env.GOOGLE_APPLICATION_CREDENTIALS;
let databaseURL = getArg("--database-url") || process.env.FIREBASE_DATABASE_URL;

if (!databaseURL) {
  const envLocal = path.resolve(__dirname, "../dashboard/.env.local");
  if (fs.existsSync(envLocal)) {
    const content = fs.readFileSync(envLocal, "utf8");
    const m = content.match(/^NEXT_PUBLIC_FIREBASE_DATABASE_URL=(.+)$/m);
    if (m && m[1]) databaseURL = m[1].trim();
  }
}

if (!databaseURL) {
  console.error("Missing database URL. Set FIREBASE_DATABASE_URL or pass --database-url.");
  process.exit(1);
}

let credential;
if (useAdc) {
  credential = admin.credential.applicationDefault();
} else {
  if (!keyfile) {
    console.error("Missing admin credential. Pass --keyfile, set GOOGLE_APPLICATION_CREDENTIALS, or use --use-adc.");
    process.exit(1);
  }
  const keyPath = path.resolve(keyfile);
  if (!fs.existsSync(keyPath)) {
    console.error("Keyfile not found:", keyPath);
    process.exit(1);
  }
  const key = JSON.parse(fs.readFileSync(keyPath, "utf8"));
  credential = admin.credential.cert(key);
}

admin.initializeApp({ credential, databaseURL });
const db = admin.database();

const targetUpdates = {
  "/pump_system/control/emergency_stop": false,
  "/pump_system/control/countdown_stop": false,
  "/pump_system/control/bypass_level_sensor": false,
  "/pump_system/control/bypass_flow_sensor": false,
  "/pump_system/control/mode": "AUTO",
  "/pump_system/config/device/flow_calibration_factor": 7.5,
  "/pump_system/config/device/tank_full_cm": 30,
  "/pump_system/status/countdown_active": false,
};

function stable(v) {
  return JSON.stringify(v);
}

(async () => {
  try {
    const root = db.ref("/");
    const before = {};

    for (const p of Object.keys(targetUpdates)) {
      const snap = await root.child(p.replace(/^\//, "")).get();
      before[p] = snap.exists() ? snap.val() : null;
    }

    const changes = [];
    for (const [p, next] of Object.entries(targetUpdates)) {
      const prev = before[p];
      if (stable(prev) !== stable(next)) {
        changes.push({ path: p, before: prev, after: next });
      }
    }

    if (changes.length === 0) {
      console.log("No changes needed. RTDB already matches target pre-flash values.");
      process.exit(0);
    }

    console.log(dryRun ? "DRY RUN: pending updates" : "APPLY: writing updates");
    for (const c of changes) {
      console.log(`${c.path}: ${stable(c.before)} -> ${stable(c.after)}`);
    }

    if (!dryRun) {
      const payload = {};
      for (const c of changes) {
        payload[c.path.replace(/^\//, "")] = c.after;
      }
      await root.update(payload);
      console.log("RTDB cleanup applied successfully.");
    } else {
      console.log("Dry run complete. Re-run with --apply to write.");
    }

    process.exit(0);
  } catch (err) {
    console.error("Cleanup failed:", err && err.message ? err.message : err);
    process.exit(1);
  }
})();
