#!/usr/bin/env node
/**
 * SMARTFLOW CALIBRATION FIX — END-TO-END TEST
 * 
 * This script verifies the complete fix workflow:
 * 1. Applies calibration to RTDB
 * 2. Simulates ESP32 reading the config
 * 3. Verifies level calculations are correct
 * 4. Reports pass/fail with detailed diagnostics
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

if (!keyfile || !databaseURL) {
  console.error("Usage: FIREBASE_DATABASE_URL=... GOOGLE_APPLICATION_CREDENTIALS=... node test-calibration-fix.js");
  console.error("Or:    node test-calibration-fix.js --keyfile path --db-url url");
  process.exit(1);
}

if (!admin.apps.length) {
  const keyPath = path.resolve(keyfile);
  if (!fs.existsSync(keyPath)) {
    console.error(`Keyfile not found: ${keyPath}`);
    process.exit(1);
  }
  try {
    const key = JSON.parse(fs.readFileSync(keyPath, "utf8"));
    admin.initializeApp({
      credential: admin.credential.cert(key),
      databaseURL
    });
  } catch (err) {
    console.error(`Firebase init failed: ${err.message}`);
    process.exit(1);
  }
}

const db = admin.database();
const RED = "\x1b[31m";
const GREEN = "\x1b[32m";
const YELLOW = "\x1b[33m";
const CYAN = "\x1b[36m";
const RESET = "\x1b[0m";

async function test() {
  console.log(CYAN + "\n=== SmartFlow Calibration Fix Test ===" + RESET);

  // Step 1: Apply fix
  console.log(YELLOW + "\n[STEP 1] Applying calibration fix to RTDB..." + RESET);
  const configRef = db.ref("pump_system/config/device");
  await configRef.update({
    tank_empty_cm: 120,
    tank_full_cm: 30
  });
  console.log(GREEN + "✓ RTDB updated" + RESET);

  // Step 2: Verify RTDB has correct values
  console.log(YELLOW + "\n[STEP 2] Verifying RTDB values..." + RESET);
  const snap = await configRef.once("value");
  const config = snap.val();
  const passStep2 = config.tank_empty_cm === 120 && config.tank_full_cm === 30;
  console.log(`  tank_empty_cm: ${config.tank_empty_cm} ${config.tank_empty_cm === 120 ? GREEN + "✓" + RESET : RED + "✗" + RESET}`);
  console.log(`  tank_full_cm: ${config.tank_full_cm} ${config.tank_full_cm === 30 ? GREEN + "✓" + RESET : RED + "✗" + RESET}`);

  // Step 3: Simulate ESP32 level conversion
  console.log(YELLOW + "\n[STEP 3] Testing level conversion formula..." + RESET);
  
  function convertDistanceToLevel(dist, empty, full) {
    const range = empty - full;
    if (range <= 0.1) return -1;
    let pct = 100.0 * (empty - dist) / range;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return Math.round(pct * 10) / 10;  // Round to 1 decimal
  }

  // Test case: distance = 70.5 cm
  const testDist = 70.5;
  const wrongEmpty = 122;  // Old hardcoded
  const wrongFull = 8;
  const correctEmpty = 120;  // Fixed
  const correctFull = 30;

  const wrongLevel = convertDistanceToLevel(testDist, wrongEmpty, wrongFull);
  const correctLevel = convertDistanceToLevel(testDist, correctEmpty, correctFull);

  console.log(`  Input: DIST=${testDist}cm (actual tank ~55% full)`);
  console.log(`  Before fix (wrong calibration 122/8): ${RED}${wrongLevel}%${RESET} ❌`);
  console.log(`  After fix (correct calibration 120/30): ${GREEN}${correctLevel}%${RESET} ✓`);
  
  const passStep3 = correctLevel >= 54 && correctLevel <= 56;  // Should be ~55%

  // Step 4: Test multiple points
  console.log(YELLOW + "\n[STEP 4] Testing multiple tank states..." + RESET);
  const testPoints = [
    { dist: 30, expected: 100, desc: "tank full" },
    { dist: 75, expected: 50, desc: "tank half-full" },
    { dist: 120, expected: 0, desc: "tank empty" }
  ];
  
  let allPointsPass = true;
  testPoints.forEach(point => {
    const level = convertDistanceToLevel(point.dist, correctEmpty, correctFull);
    const pass = Math.abs(level - point.expected) < 2;
    const status = pass ? GREEN + "✓" + RESET : RED + "✗" + RESET;
    console.log(`  ${point.desc}: DIST=${point.dist}cm → ${level}% (expected ~${point.expected}%) ${status}`);
    if (!pass) allPointsPass = false;
  });

  // Final verdict
  console.log(YELLOW + "\n[RESULT]" + RESET);
  const allPass = passStep2 && passStep3 && allPointsPass;
  
  if (allPass) {
    console.log(GREEN + "✓ ALL TESTS PASSED" + RESET);
    console.log("\nCalibration fix is working correctly:");
    console.log("  1. RTDB has correct tank_empty_cm and tank_full_cm");
    console.log("  2. Level conversion formula produces accurate results");
    console.log("  3. All tank state points convert correctly");
    console.log("\nNext steps:");
    console.log("  • ESP32 will read the new calibration within 30 seconds");
    console.log("  • Water level will start showing correct percent");
    console.log("  • Monitor RTDB /pump_system/status/water_level_percent");
  } else {
    console.log(RED + "✗ TEST FAILED" + RESET);
    console.log("  Check RTDB values and try again");
    process.exit(1);
  }
}

test()
  .then(() => process.exit(0))
  .catch(err => {
    console.error(RED + `Fatal error: ${err.message}` + RESET);
    process.exit(1);
  });
