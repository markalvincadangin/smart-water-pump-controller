import * as admin from "firebase-admin";

export function hasFlag(flag: string): boolean {
  return process.argv.includes(flag);
}

export function requiredArgument(name: string): string {
  const index = process.argv.indexOf(name);
  const value = index >= 0 ? process.argv[index + 1] : undefined;
  if (!value || value.startsWith("--")) throw new Error(`Missing ${name}`);
  return value;
}

export function initializeTrustedAdmin(): { projectId: string; databaseURL: string; db: admin.database.Database; auth: admin.auth.Auth } {
  const projectId = requiredArgument("--project");
  const databaseURL = requiredArgument("--database-url");
  if (!/^[a-z][a-z0-9-]{4,62}$/.test(projectId)) throw new Error("Invalid Firebase project ID.");
  const url = new URL(databaseURL);
  if (url.protocol !== "https:" || !url.hostname.endsWith("firebasedatabase.app")) {
    throw new Error("--database-url must be an HTTPS Firebase Realtime Database URL.");
  }
  if (projectId.includes("prod") || projectId.includes("production")) {
    throw new Error("Refusing to run a trusted administration script against a production-named project.");
  }
  if (!admin.apps.length) admin.initializeApp({ projectId, databaseURL });
  return { projectId, databaseURL, db: admin.database(), auth: admin.auth() };
}

export function requireApplyAcknowledgement(acknowledgement: string): void {
  if (!hasFlag("--apply")) return;
  if (!hasFlag(acknowledgement)) throw new Error(`--apply requires ${acknowledgement}.`);
}

export async function shutdownTrustedAdmin(): Promise<void> {
  if (admin.apps.length) await admin.app().delete();
}
