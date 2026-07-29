/**
 * Destructive, backup-first reset for the SmartFlow Firebase test environment.
 *
 * Default mode is a read-only plan. Applying requires an exact project,
 * database URL, and an explicit acknowledgement. This intentionally preserves
 * Firebase project configuration, deployed Functions, OAuth providers, and
 * local device secrets; it deletes only RTDB data and Firebase Auth users.
 *
 * Usage:
 *   npm run reset-test-env -- --project smartflow-fed87 --database-url https://...  # plan
 *   npm run reset-test-env -- --project smartflow-fed87 --database-url https://... --apply --acknowledge-delete-all-test-data
 */
import * as admin from "firebase-admin";
import { mkdir, writeFile } from "node:fs/promises";
import { join } from "node:path";

const APPLY_FLAG = "--apply";
const ACK_FLAG = "--acknowledge-delete-all-test-data";

type RegistrySeed = {
  deviceId: string;
  secretName: string;
};

function argument(name: string): string | undefined {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : undefined;
}

function requiredArgument(name: string): string {
  const value = argument(name);
  if (!value || value.startsWith("--")) throw new Error(`Missing ${name}`);
  return value;
}

function usage(): string {
  return [
    "Usage:",
    "  npm run reset-test-env -- --project <project-id> --database-url <https-url>",
    `  npm run reset-test-env -- --project <project-id> --database-url <https-url> ${APPLY_FLAG} ${ACK_FLAG}`,
    "    [--seed-device-id <SF-XXXXXX> --seed-secret-name <projects/.../secrets/...>]",
    "",
    "The default command creates a read-only reset plan.",
    "Apply creates an ignored RTDB backup, deletes all RTDB data, then deletes all Firebase Auth users.",
    "An optional seed recreates only one active non-secret device-registry record after the deletion.",
  ].join("\n");
}

function optionalRegistrySeed(projectId: string): RegistrySeed | undefined {
  const deviceId = argument("--seed-device-id");
  const secretName = argument("--seed-secret-name");
  if (!deviceId && !secretName) return undefined;
  if (!deviceId || !secretName) {
    throw new Error("--seed-device-id and --seed-secret-name must be provided together.");
  }
  if (!/^SF-[A-F0-9]{6}$/.test(deviceId)) throw new Error("Invalid --seed-device-id.");
  if (!secretName.startsWith(`projects/${projectId}/secrets/`) || secretName.includes("/versions/")) {
    throw new Error("--seed-secret-name must be a non-versioned Secret Manager resource in the selected project.");
  }
  return { deviceId, secretName };
}

function assertSafeTarget(projectId: string, databaseUrl: string): void {
  if (!/^[a-z][a-z0-9-]{4,62}$/.test(projectId)) throw new Error("Invalid Firebase project ID.");
  const url = new URL(databaseUrl);
  if (url.protocol !== "https:" || !url.hostname.endsWith("firebasedatabase.app")) {
    throw new Error("--database-url must be an HTTPS Firebase Realtime Database URL.");
  }
  if (projectId.includes("prod") || projectId.includes("production")) {
    throw new Error("Refusing to reset a production-named project.");
  }
}

async function listAllUsers(auth: admin.auth.Auth): Promise<admin.auth.UserRecord[]> {
  const users: admin.auth.UserRecord[] = [];
  let pageToken: string | undefined;
  do {
    const page = await auth.listUsers(1000, pageToken);
    users.push(...page.users);
    pageToken = page.pageToken;
  } while (pageToken);
  return users;
}

async function writeRtdbBackup(root: unknown, projectId: string): Promise<string> {
  const backupDir = join(process.cwd(), ".local", "reset-backups");
  const timestamp = new Date().toISOString().replace(/[:.]/g, "-");
  const filename = `${projectId}-rtdb-before-reset-${timestamp}.json`;
  await mkdir(backupDir, { recursive: true });
  const path = join(backupDir, filename);
  await writeFile(path, JSON.stringify(root, null, 2), { encoding: "utf8", mode: 0o600 });
  return path;
}

async function main(): Promise<void> {
  if (process.argv.includes("--help") || process.argv.includes("-h")) {
    console.log(usage());
    return;
  }

  const projectId = requiredArgument("--project");
  const databaseURL = requiredArgument("--database-url");
  assertSafeTarget(projectId, databaseURL);
  const apply = process.argv.includes(APPLY_FLAG);
  const acknowledged = process.argv.includes(ACK_FLAG);
  if (apply && !acknowledged) throw new Error(`Apply requires ${ACK_FLAG}.`);
  const registrySeed = optionalRegistrySeed(projectId);

  admin.initializeApp({ projectId, databaseURL });
  const auth = admin.auth();
  const db = admin.database();
  const [users, rootSnapshot] = await Promise.all([listAllUsers(auth), db.ref().get()]);
  const root = rootSnapshot.exists() ? rootSnapshot.val() : null;
  const topLevelPaths = root && typeof root === "object" ? Object.keys(root as Record<string, unknown>).sort() : [];
  const plan = {
    mode: apply ? "apply" : "dry-run",
    projectId,
    databaseURL,
    authUsers: users.length,
    rtdbTopLevelPaths: topLevelPaths,
    registrySeed: registrySeed ? { deviceId: registrySeed.deviceId, state: "active", secretName: registrySeed.secretName } : null,
  };
  console.log(JSON.stringify(plan, null, 2));

  if (!apply) {
    console.log("Dry run only. Re-run with --apply and --acknowledge-delete-all-test-data to reset.");
    return;
  }

  const backupPath = await writeRtdbBackup(root, projectId);
  console.log(`RTDB backup written: ${backupPath}`);
  await db.ref().remove();
  console.log("RTDB root deleted.");

  for (let index = 0; index < users.length; index += 1000) {
    const result = await auth.deleteUsers(users.slice(index, index + 1000).map((user) => user.uid));
    if (result.failureCount > 0) throw new Error(`Auth deletion failed for ${result.failureCount} user(s). RTDB backup remains at ${backupPath}.`);
  }
  if (registrySeed) {
    await db.ref(`deviceRegistry/${registrySeed.deviceId}`).set({
      state: "active",
      secretName: registrySeed.secretName,
      updatedAtMs: Date.now(),
      updatedBy: "reset-test-environment",
    });
    console.log(`Device registry seed restored for ${registrySeed.deviceId}.`);
  }
  console.log(JSON.stringify({
    status: "complete",
    deletedAuthUsers: users.length,
    backupPath,
    registrySeeded: registrySeed?.deviceId ?? null,
  }, null, 2));
}

main()
  .catch((error: unknown) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
  })
  .finally(async () => {
    if (admin.apps.length) await admin.app().delete();
  });
