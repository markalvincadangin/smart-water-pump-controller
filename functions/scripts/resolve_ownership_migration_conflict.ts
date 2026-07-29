/** Trusted operator-only resolution for one frozen ownership migration conflict. */
import { planOwnershipMigrationResolution } from "../src/device_bootstrap";
import { hasFlag, initializeTrustedAdmin, requiredArgument, requireApplyAcknowledgement, shutdownTrustedAdmin } from "./admin_cli";

const ACK_FLAG = "--acknowledge-ownership-resolution";

async function main(): Promise<void> {
  if (hasFlag("--help") || hasFlag("-h")) {
    console.log("Usage: npm run resolve-ownership-conflict -- --project <project-id> --database-url <https-url> --device-id SF-XXXXXX --owner-uid <uid> --operator-uid <admin-uid> --evidence <case-reference> [--apply --acknowledge-ownership-resolution]");
    return;
  }
  const deviceId = requiredArgument("--device-id");
  const ownerUid = requiredArgument("--owner-uid");
  const operatorUid = requiredArgument("--operator-uid");
  const evidence = requiredArgument("--evidence");
  const apply = hasFlag("--apply");
  requireApplyAcknowledgement(ACK_FLAG);
  if (!/^SF-[A-F0-9]{6,12}$/.test(deviceId) || evidence.length > 160) throw new Error("Invalid device ID or evidence reference.");

  const { projectId, databaseURL, auth, db } = initializeTrustedAdmin();
  const operator = await auth.getUser(operatorUid);
  if (operator.customClaims?.admin !== true) throw new Error("Operator admin claim required.");
  const owner = await auth.getUser(ownerUid);
  if (owner.disabled || owner.providerData.length === 0 || owner.providerData.some((provider) => provider.providerId === "anonymous")) {
    throw new Error("Chosen owner is not a durable eligible user.");
  }
  if (!apply) {
    const device = (await db.ref(`devices/${deviceId}`).get()).val() as { ownership?: { migrationState?: string } } | null;
    console.log(JSON.stringify({ mode: "dry-run", projectId, databaseURL, deviceId, eligible: device?.ownership?.migrationState === "conflict" }, null, 2));
    return;
  }
  const result = await db.ref().transaction((root: Record<string, unknown> | null) => {
    if (!root) return;
    return planOwnershipMigrationResolution(root, deviceId, ownerUid, operatorUid, evidence, Date.now());
  });
  console.log(JSON.stringify({ mode: "apply", projectId, deviceId, committed: result.committed }, null, 2));
}

main()
  .catch((error: unknown) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
  })
  .finally(shutdownTrustedAdmin);
