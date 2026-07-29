/** Trusted, dry-run-first migration: consistent owners migrate; ambiguity freezes. */
import { planLegacyOwnershipMigration } from "../src/device_bootstrap";
import { hasFlag, initializeTrustedAdmin, requireApplyAcknowledgement, shutdownTrustedAdmin } from "./admin_cli";

const ACK_FLAG = "--acknowledge-ownership-migration";

function summarize(root: Record<string, unknown> | null): { migrated: number; conflicts: number; skipped: number } {
  if (!root?.devices || typeof root.devices !== "object") return { migrated: 0, conflicts: 0, skipped: 0 };
  let next = structuredClone(root);
  const summary = { migrated: 0, conflicts: 0, skipped: 0 };
  for (const deviceId of Object.keys(next.devices as Record<string, unknown>)) {
    const result = planLegacyOwnershipMigration(next, deviceId, Date.now());
    next = result.root;
    if (result.outcome === "migrated") summary.migrated++;
    else if (result.outcome === "conflict") summary.conflicts++;
    else summary.skipped++;
  }
  return summary;
}

async function main(): Promise<void> {
  if (hasFlag("--help") || hasFlag("-h")) {
    console.log("Usage: npm run migrate-legacy-ownership -- --project <project-id> --database-url <https-url> [--apply --acknowledge-ownership-migration]");
    return;
  }
  const apply = hasFlag("--apply");
  requireApplyAcknowledgement(ACK_FLAG);
  const { projectId, databaseURL, db } = initializeTrustedAdmin();
  const before = (await db.ref().get()).val() as Record<string, unknown> | null;
  console.log(JSON.stringify({ mode: apply ? "apply" : "dry-run", projectId, databaseURL, ...summarize(before) }, null, 2));
  if (!apply) return;

  let applied = { migrated: 0, conflicts: 0, skipped: 0 };
  const result = await db.ref().transaction((current: Record<string, unknown> | null) => {
    if (!current) return current;
    applied = summarize(current);
    let next = structuredClone(current);
    for (const deviceId of Object.keys((next.devices ?? {}) as Record<string, unknown>)) {
      next = planLegacyOwnershipMigration(next, deviceId, Date.now()).root;
    }
    return next;
  });
  console.log(JSON.stringify({ mode: "apply", committed: result.committed, ...applied }, null, 2));
}

main()
  .catch((error: unknown) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
  })
  .finally(shutdownTrustedAdmin);
