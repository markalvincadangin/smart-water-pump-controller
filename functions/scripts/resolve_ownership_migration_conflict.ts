/**
 * Trusted per-device resolution for a previously frozen migration conflict.
 * Required: --device-id SF-XXXXXX --owner-uid UID --operator-uid UID
 *           --evidence CASE-123 [--apply]
 * It validates the operator's admin claim and is deliberately not a callable.
 */
import * as admin from "firebase-admin";
import { planOwnershipMigrationResolution } from "../src/device_bootstrap";

function argument(name: string): string {
  const index = process.argv.indexOf(name);
  const value = index >= 0 ? process.argv[index + 1] : undefined;
  if (!value || value.startsWith("--")) throw new Error(`Missing ${name}`);
  return value;
}

async function main(): Promise<void> {
  const deviceId = argument("--device-id");
  const ownerUid = argument("--owner-uid");
  const operatorUid = argument("--operator-uid");
  const evidence = argument("--evidence");
  const apply = process.argv.includes("--apply");
  if (!/^SF-[A-F0-9]{6,12}$/.test(deviceId) || evidence.length > 160) throw new Error("Invalid device ID or evidence reference");
  admin.initializeApp();
  const operator = await admin.auth().getUser(operatorUid);
  if (operator.customClaims?.admin !== true) throw new Error("Operator admin claim required");
  const owner = await admin.auth().getUser(ownerUid);
  if (owner.disabled || owner.providerData.length === 0) throw new Error("Chosen owner is not eligible");
  const db = admin.database();
  if (!apply) {
    const device = (await db.ref(`devices/${deviceId}`).get()).val() as { ownership?: { migrationState?: string } } | null;
    console.log(JSON.stringify({ mode: "dry-run", eligible: device?.ownership?.migrationState === "conflict", deviceId }));
    return;
  }
  const result = await db.ref().transaction((root: Record<string, any> | null) => {
    if (!root) return;
    return planOwnershipMigrationResolution(root, deviceId, ownerUid, operatorUid, evidence, Date.now());
  });
  console.log(JSON.stringify({ mode: "apply", committed: result.committed, deviceId }));
}

main().catch((error) => { console.error(error); process.exitCode = 1; });
