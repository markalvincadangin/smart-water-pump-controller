#!/usr/bin/env node
/**
 * Fix water level sensor calibration in RTDB device config.
 * 
 * ROOT CAUSE: ESP32 was using hardcoded defaults (122/8 cm) instead of field calibration (120/30 cm)
 * because /pump_system/config/device was missing tank_empty_cm and tank_full_cm fields.
 * 
 * FIX: Set tank_empty_cm=120 (sensor mounted 120cm above tank bottom) and 
 *      tank_full_cm=30 (sensor reads 30cm when tank full).
 * 
 * Prerequisites:
 *   - Firebase project with Realtime Database
 *   - Service account key: set GOOGLE_APPLICATION_CREDENTIALS or pass --keyfile
 * 
 * Usage (run from scripts/ after: npm install):
 *   node fix-device-config-calibration.js [--keyfile path] [--database-url URL]
 * 
 * Example:
 *   node fix-device-config-calibration.js --keyfile ../serviceAccountKey.json --database-url "https://myproject-default-rtdb.asia-southeast1.firebasedatabase.app"
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
const deviceConfigRef = db.ref("pump_system/config/device");

(async () => {
  try {
    console.log("Reading current device config...");
    const snap = await deviceConfigRef.once("value");
    const currentConfig = snap.val() || {};
    
    console.log("Current config:", JSON.stringify(currentConfig, null, 2));
    
    // Apply calibration fix
    const updates = {
      tank_empty_cm: 120,    // Sensor mounted 120 cm above tank bottom (empty reference)
      tank_full_cm: 30       // Sensor reads 30 cm when tank is full
    };
    
    console.log("\nApplying calibration fix:");
    console.log("  tank_empty_cm: 122 → 120 (sensor reference height)");
    console.log("  tank_full_cm: 8 → 30 (sensor reading at full tank)");
    
    await deviceConfigRef.update(updates);
    
    console.log("\n✓ Device config updated successfully!");
    console.log("  ESP32 will read new calibration within 30 seconds.");
    console.log("  Level percent should now calculate correctly:");
    console.log("    - When DIST=70.5cm, level% ≈ 55% (was miscalculated as ~45%)");
    console.log("    - Formula: pct = 100 * (120 - 70.5) / (120 - 30) = 100 * 49.5 / 90 ≈ 55%");
    
  } catch (err) {
    console.error("\n✗ Update failed:", err.message);
    process.exit(1);
  }
  process.exit(0);
})();
