#!/usr/bin/env node
/**
 * Bootstrap first admin UID into pump_system/config/admins/{uid}
 * Use after deploying database.rules.json (dynamic admin rules).
 *
 * Prerequisites:
 *   - Firebase project with Realtime Database
 *   - Service account key: set GOOGLE_APPLICATION_CREDENTIALS or pass --keyfile
 *   - UID from Firebase Console → Authentication → Users (copy the User UID)
 *
 * Usage (run from scripts/ after: npm install):
 *   node bootstrap-admin.js [--keyfile path] [--database-url URL] <FIREBASE_UID>
 *
 * Required: --database-url (or FIREBASE_DATABASE_URL env). Get it from Firebase Console → Realtime Database → URL at top.
 * Example: https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app
 *
 * Example:
 *   node bootstrap-admin.js --keyfile ../serviceAccountKey.json --database-url "https://myproject-default-rtdb.asia-southeast1.firebasedatabase.app" YOUR_UID
 */
const admin = require("firebase-admin");
const path = require("path");
const fs = require("fs");

const args = process.argv.slice(2);
const keyfileIndex = args.findIndex((a) => a === "--keyfile");
const dbUrlIndex = args.findIndex((a) => a === "--database-url");
let keyfile = process.env.GOOGLE_APPLICATION_CREDENTIALS;
let databaseURL = process.env.FIREBASE_DATABASE_URL;
if (keyfileIndex >= 0 && args[keyfileIndex + 1]) keyfile = args[keyfileIndex + 1];
if (dbUrlIndex >= 0 && args[dbUrlIndex + 1]) databaseURL = args[dbUrlIndex + 1];
const used = new Set([keyfileIndex, keyfileIndex + 1, dbUrlIndex, dbUrlIndex + 1].filter((i) => i >= 0));
const uid = args.find((_, i) => !used.has(i));

if (!uid) {
  console.error("Usage: node bootstrap-admin.js [--keyfile path] [--database-url URL] <FIREBASE_UID>");
  console.error("Get UID from Firebase Console → Authentication → Users");
  process.exit(1);
}

if (!keyfile) {
  console.error("Set GOOGLE_APPLICATION_CREDENTIALS or pass --keyfile path/to/serviceAccountKey.json");
  process.exit(1);
}

if (!databaseURL) {
  console.error("Set FIREBASE_DATABASE_URL or pass --database-url <your Realtime Database URL>");
  console.error("Find it in Firebase Console → Realtime Database (e.g. https://PROJECT-default-rtdb.REGION.firebasedatabase.app)");
  process.exit(1);
}

if (!admin.apps.length) {
  const keyPath = path.resolve(keyfile);
  if (!fs.existsSync(keyPath)) {
    console.error("Keyfile not found:", keyPath);
    console.error("Use the real path to your Firebase service account JSON (download from Firebase Console → Project settings → Service accounts).");
    process.exit(1);
  }
  const key = JSON.parse(fs.readFileSync(keyPath, "utf8"));
  admin.initializeApp({ credential: admin.credential.cert(key), databaseURL });
}

const db = admin.database();
const adminsRef = db.ref("pump_system/config/admins");

(async () => {
  try {
    await adminsRef.child(uid).set(true);
    console.log(`Admin bootstrap OK: pump_system/config/admins/${uid} = true`);
  } catch (err) {
    console.error("Bootstrap failed:", err.message);
    process.exit(1);
  }
  process.exit(0);
})();
