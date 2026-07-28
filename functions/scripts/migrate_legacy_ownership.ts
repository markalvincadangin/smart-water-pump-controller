/**
 * Trusted, dry-run-first legacy ownership migration.
 * Build Functions first, then run with a service-account authenticated shell:
 *   npx ts-node scripts/migrate_legacy_ownership.ts --apply
 * This command never guesses an owner: inconsistent records are frozen.
 */
import * as admin from "firebase-admin";
import { planLegacyOwnershipMigration } from "../src/device_bootstrap";

const apply = process.argv.includes("--apply");

async function main(): Promise<void> {
  admin.initializeApp();
  const db = admin.database();
  const root = (await db.ref().get()).val() as Record<string, any> | null;
  if (!root?.devices) throw new Error("No devices found");
  let next = structuredClone(root) as Record<string, any>;
  const summary = { migrated: 0, conflicts: 0, skipped: 0 };
  for (const id of Object.keys(next.devices)) {
    const result = planLegacyOwnershipMigration(next, id, Date.now());
    next = result.root;
    if (result.outcome === "migrated") summary.migrated++;
    else if (result.outcome === "conflict") summary.conflicts++;
    else summary.skipped++;
  }
  console.log(JSON.stringify({ mode: apply ? "apply" : "dry-run", ...summary }));
  if (apply) await db.ref().set(next);
}

main().catch((error) => { console.error(error); process.exitCode = 1; });
