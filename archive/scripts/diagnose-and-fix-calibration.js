#!/usr/bin/env node
/**
 * SMARTFLOW WATER LEVEL CALIBRATION DIAGNOSTIC & FIX
 * 
 * This script:
 * 1. Connects to Firebase RTDB
 * 2. Reads current device config
 * 3. Diagnoses calibration issues
 * 4. Applies fix if needed
 * 5. Verifies and reports results
 * 
 * Usage:
 *   FIREBASE_DATABASE_URL="https://..." GOOGLE_APPLICATION_CREDENTIALS="..." node diagnose-and-fix-calibration.js
 * 
 * Or with flags:
 *   node diagnose-and-fix-calibration.js --keyfile key.json --db-url "https://..."
 */

const admin = require("firebase-admin");
const path = require("path");
const fs = require("fs");

// Parse arguments
const args = process.argv.slice(2);
const keyfileIndex = args.findIndex((a) => a === "--keyfile");
const dbUrlIndex = args.findIndex((a) => a === "--db-url");
let keyfile = process.env.GOOGLE_APPLICATION_CREDENTIALS;
let databaseURL = process.env.FIREBASE_DATABASE_URL;
if (keyfileIndex >= 0 && args[keyfileIndex + 1]) keyfile = args[keyfileIndex + 1];
if (dbUrlIndex >= 0 && args[dbUrlIndex + 1]) databaseURL = args[dbUrlIndex + 1];

// Validate inputs
if (!keyfile) {
  console.error("❌ Error: GOOGLE_APPLICATION_CREDENTIALS not set");
  console.error("   Set env var or pass --keyfile path");
  process.exit(1);
}

if (!databaseURL) {
  console.error("❌ Error: FIREBASE_DATABASE_URL not set");
  console.error("   Set env var or pass --db-url URL");
  process.exit(1);
}

// Initialize Firebase
if (!admin.apps.length) {
  const keyPath = path.resolve(keyfile);
  if (!fs.existsSync(keyPath)) {
    console.error(`❌ Keyfile not found: ${keyPath}`);
    process.exit(1);
  }
  try {
    const key = JSON.parse(fs.readFileSync(keyPath, "utf8"));
    admin.initializeApp({
      credential: admin.credential.cert(key),
      databaseURL
    });
  } catch (err) {
    console.error(`❌ Failed to initialize Firebase: ${err.message}`);
    process.exit(1);
  }
}

const db = admin.database();

/**
 * DIAGNOSTIC LOGIC
 */
async function diagnoseAndFix() {
  const RED = "\x1b[31m";
  const GREEN = "\x1b[32m";
  const YELLOW = "\x1b[33m";
  const CYAN = "\x1b[36m";
  const RESET = "\x1b[0m";

  console.log("\n" + CYAN + "=== SmartFlow Water Level Calibration Diagnosis ===" + RESET);
  console.log("Checking RTDB device config...\n");

  // Read device config
  const configRef = db.ref("pump_system/config/device");
  const snap = await configRef.once("value");
  const currentConfig = snap.val() || {};

  // Check for calibration fields
  const hasEmpty = "tank_empty_cm" in currentConfig;
  const hasFull = "tank_full_cm" in currentConfig;
  const emptyVal = currentConfig.tank_empty_cm;
  const fullVal = currentConfig.tank_full_cm;

  console.log("Current device config:");
  console.log("  tank_empty_cm: " + (hasEmpty ? GREEN + emptyVal + RESET : RED + "NOT SET" + RESET));
  console.log("  tank_full_cm:  " + (hasFull ? GREEN + fullVal + RESET : RED + "NOT SET" + RESET));

  // Field calibration (correct values)
  const CORRECT_EMPTY = 120;
  const CORRECT_FULL = 30;

  // Diagnosis
  console.log("\nDiagnosis:");
  console.log(`  Field calibration: empty=${CORRECT_EMPTY}cm, full=${CORRECT_FULL}cm`);
  console.log(`  ESP32 hardcoded fallback: empty=122cm, full=8cm (WRONG if RTDB missing)`);

  let needsFix = false;
  const issues = [];

  if (!hasEmpty || emptyVal !== CORRECT_EMPTY) {
    issues.push(
      `tank_empty_cm=${hasEmpty ? emptyVal : "missing"} (should be ${CORRECT_EMPTY})`
    );
    needsFix = true;
  }

  if (!hasFull || fullVal !== CORRECT_FULL) {
    issues.push(
      `tank_full_cm=${hasFull ? fullVal : "missing"} (should be ${CORRECT_FULL})`
    );
    needsFix = true;
  }

  if (needsFix) {
    console.log("\n" + RED + "Issues found:" + RESET);
    issues.forEach((issue) => console.log("  ❌ " + issue));

    console.log("\n" + YELLOW + "Impact:" + RESET);
    console.log("  ESP32 is using WRONG calibration (122/8 cm)");
    console.log("  When sensor sends DIST=70.5cm:");
    console.log("    ❌ Calculated (wrong):  45% ← range 122-8=114cm");
    console.log("    ✓ Correct should be:   55% ← range 120-30=90cm");
    console.log("  Error: -10 percentage points");
  } else {
    console.log("\n" + GREEN + "✓ Config is correct!" + RESET);
    console.log("  tank_empty_cm: " + emptyVal);
    console.log("  tank_full_cm: " + fullVal);
    console.log("  ESP32 will calculate level% correctly");
    return;
  }

  // Apply fix
  console.log("\n" + CYAN + "Applying fix..." + RESET);
  const updates = {
    tank_empty_cm: CORRECT_EMPTY,
    tank_full_cm: CORRECT_FULL
  };

  try {
    await configRef.update(updates);
    console.log(GREEN + "✓ RTDB updated successfully!" + RESET);

    // Verify
    const verifySnap = await configRef.once("value");
    const verifiedConfig = verifySnap.val();
    console.log("\nVerification:");
    console.log("  tank_empty_cm: " + verifiedConfig.tank_empty_cm);
    console.log("  tank_full_cm: " + verifiedConfig.tank_full_cm);

    console.log("\n" + GREEN + "Fix applied successfully!" + RESET);
    console.log(YELLOW + "Timeline:" + RESET);
    console.log("  ✓ RTDB updated now");
    console.log("  ⏱ ESP32 reads new config in ~30 seconds");
    console.log("    (log: '[INFO] FIREBASE: Device config updated.')");
    console.log("  ⏱ Next level% calculation uses correct calibration");
    console.log("    (should show ~55% when DIST=70.5cm)");
    console.log("\n");
  } catch (err) {
    console.error(RED + "❌ Update failed: " + err.message + RESET);
    process.exit(1);
  }
}

// Run
(async () => {
  try {
    await diagnoseAndFix();
  } catch (err) {
    console.error("\n" + RED + "Fatal error: " + err.message + RESET);
    process.exit(1);
  }
  process.exit(0);
})();
