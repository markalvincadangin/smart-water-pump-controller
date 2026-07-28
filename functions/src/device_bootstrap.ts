import * as crypto from "crypto";
import * as admin from "firebase-admin";
import { SecretManagerServiceClient } from "@google-cloud/secret-manager";
import { HttpsError, onCall, onRequest } from "firebase-functions/v2/https";
import { logger } from "firebase-functions";

const REGION = "asia-southeast1";
const WINDOW_MS = 5 * 60 * 1000;
const OWNERSHIP_PAIRING_WINDOW_MS = 5 * 60 * 1000;
const RATE_WINDOW_MS = 60 * 1000;
const RATE_LIMIT = 5;
const DEVICE_ID = /^SF-[A-F0-9]{6,12}$/;
const NONCE = /^[A-Za-z0-9_-]{16,128}$/;
const secrets = new SecretManagerServiceClient();

type Registry = { state?: "active" | "revoked"; secretName?: string };
type DurableAuth = { uid: string; emailVerified: boolean; provider?: string; isAnonymous?: boolean; role?: string };
type PairingVerifier = {
  proofHash?: string;
  expiresAtMs?: number;
  purpose?: "claim" | "transfer" | "release";
  consumedAtMs?: number;
  recipientUid?: string;
};

/** Stable, client-visible codes for the ownership claim boundary. */
export const CLAIM_RESULT_CODES = {
  DURABLE_ACCOUNT_REQUIRED: "DURABLE_ACCOUNT_REQUIRED",
  EMAIL_VERIFICATION_REQUIRED: "EMAIL_VERIFICATION_REQUIRED",
  INVALID_DEVICE: "INVALID_DEVICE",
  INVALID_PAIRING_PROOF: "INVALID_PAIRING_PROOF",
  EXPIRED_PAIRING: "EXPIRED_PAIRING",
  ALREADY_CLAIMED: "ALREADY_CLAIMED",
  CLAIM_UNAVAILABLE: "CLAIM_UNAVAILABLE",
} as const;
export type ClaimResultCode = typeof CLAIM_RESULT_CODES[keyof typeof CLAIM_RESULT_CODES];

const pairingHash = (proof: string): string => crypto.createHash("sha256").update(proof, "utf8").digest("hex");

function requireDurableAccount(auth: DurableAuth | undefined): DurableAuth {
  if (!auth || !auth.uid || auth.isAnonymous || auth.role === "device") {
    throw new HttpsError("permission-denied", "DURABLE_ACCOUNT_REQUIRED");
  }
  if (auth.provider === "password" && !auth.emailVerified) {
    throw new HttpsError("failed-precondition", "EMAIL_VERIFICATION_REQUIRED");
  }
  return auth;
}

function callableDurableAuth(request: { auth?: { uid: string; token: Record<string, unknown> } }): DurableAuth {
  const token = request.auth?.token;
  return requireDurableAccount(request.auth ? {
    uid: request.auth.uid,
    emailVerified: token?.email_verified === true,
    provider: (token?.firebase as { sign_in_provider?: string } | undefined)?.sign_in_provider,
    isAnonymous: (token?.firebase as { sign_in_provider?: string } | undefined)?.sign_in_provider === "anonymous",
    role: typeof token?.role === "string" ? token.role : undefined,
  } : undefined);
}

function requirePairingProof(value: unknown): string {
  if (typeof value !== "string" || value.length < 24 || value.length > 256) {
    throw new HttpsError("invalid-argument", "INVALID_PAIRING_PROOF");
  }
  return value;
}

/** Internal seams used by local contract tests; not exposed as callable APIs. */
export const ownershipTestOnly = {
  pairingHash,
  requireDurableAccount,
  requirePairingProof,
};

export type LegacyOwnershipMigration = {
  outcome: "migrated" | "conflict" | "not_legacy";
  code?: "MISSING_LEGACY_OWNER" | "MISSING_LEGACY_INDEX" | "CONFLICTING_LEGACY_OWNER";
  root: Record<string, any>;
};

/**
 * Pure, idempotent migration planner. It only accepts a legacy owner when the
 * deprecated metadata marker and exactly one user index agree. Ambiguous data
 * is frozen as a conflict and must be resolved by a trusted operator script.
 */
export function planLegacyOwnershipMigration(
  root: Record<string, any>,
  id: string,
  nowMs: number,
): LegacyOwnershipMigration {
  const next = structuredClone(root) as Record<string, any>;
  const device = next.devices?.[id];
  if (!device) throw new Error("Unknown device");
  const legacyOwnerUid = typeof device.metadata?.claimedByUid === "string" ? device.metadata.claimedByUid : undefined;
  const indexedOwners = Object.entries(next.users ?? {})
    .filter(([, user]) => (user as { devices?: Record<string, boolean> }).devices?.[id] === true)
    .map(([uid]) => uid);
  if (!legacyOwnerUid && indexedOwners.length === 0) return { outcome: "not_legacy", root: next };

  const ownership = device.ownership ?? {};
  if (ownership.migrationState === "migrated" || ownership.migrationState === "resolved") {
    return { outcome: "not_legacy", root: next };
  }
  if (legacyOwnerUid && indexedOwners.length === 1 && indexedOwners[0] === legacyOwnerUid) {
    device.ownership = {
      ...ownership,
      ownerUid: legacyOwnerUid,
      state: "claimed",
      migrationState: "migrated",
      migratedAtMs: nowMs,
    };
    device.ownershipAudit = {
      ...(device.ownershipAudit ?? {}),
      [`migration-${nowMs}`]: { type: "legacy_ownership_migrated", ownerUid: legacyOwnerUid, createdAtMs: nowMs },
    };
    return { outcome: "migrated", root: next };
  }
  const code = !legacyOwnerUid ? "MISSING_LEGACY_OWNER" :
    indexedOwners.length === 0 ? "MISSING_LEGACY_INDEX" : "CONFLICTING_LEGACY_OWNER";
  device.ownership = {
    ...ownership,
    migrationState: "conflict",
    migrationConflictCode: code,
    migrationDetectedAtMs: nowMs,
  };
  device.ownershipAudit = {
    ...(device.ownershipAudit ?? {}),
    [`migration-${nowMs}`]: { type: "legacy_ownership_conflict", code, createdAtMs: nowMs },
  };
  return { outcome: "conflict", code, root: next };
}

/**
 * Trusted conflict-resolution planner. Caller authentication is intentionally
 * outside this function so the only caller is the operator-admin script.
 */
export function planOwnershipMigrationResolution(
  root: Record<string, any>,
  id: string,
  ownerUid: string,
  operatorUid: string,
  evidence: string,
  nowMs: number,
): Record<string, any> | undefined {
  const next = structuredClone(root) as Record<string, any>;
  const device = next.devices?.[id];
  if (!device || device.ownership?.migrationState !== "conflict") return;
  for (const user of Object.values(next.users ?? {}) as Array<{ devices?: Record<string, boolean> }>) {
    if (user.devices) delete user.devices[id];
  }
  next.users = {
    ...(next.users ?? {}),
    [ownerUid]: {
      ...(next.users?.[ownerUid] ?? {}),
      devices: { ...(next.users?.[ownerUid]?.devices ?? {}), [id]: true },
    },
  };
  device.metadata = { ...(device.metadata ?? {}), claimedByUid: ownerUid };
  device.ownership = {
    ...device.ownership,
    ownerUid,
    state: "claimed",
    migrationState: "resolved",
    migrationResolvedAtMs: nowMs,
  };
  device.ownershipAudit = {
    ...(device.ownershipAudit ?? {}),
    [`migration-resolution-${nowMs}`]: {
      type: "migration_resolved", ownerUid, operatorUid, evidence, createdAtMs: nowMs,
    },
  };
  return next;
}

/**
 * Pure transaction planner for owner-authorized Wi-Fi recovery. Keeping this
 * authorization and all related writes inside one root transaction prevents a
 * stale convenience index or a concurrent ownership change from authorizing a
 * reset of another user's device.
 */
export function planWifiReprovision(
  root: Record<string, any>,
  id: string,
  ownerUid: string,
  requestId: string,
  nonce: string,
  nowMs: number,
): Record<string, any> | undefined {
  const next = structuredClone(root) as Record<string, any>;
  const device = next.devices?.[id];
  if (!device || device.ownership?.ownerUid !== ownerUid || device.ownership?.state !== "claimed") return;

  const request = {
    action: "WIFI_REPROVISION",
    requestedByUid: ownerUid,
    issuedAtMs: nowMs,
    expiresAtMs: nowMs + WINDOW_MS,
    nonce,
    status: "pending",
  };
  device.maintenance = {
    ...(device.maintenance ?? {}),
    requests: { ...(device.maintenance?.requests ?? {}), [requestId]: request },
    audit: {
      ...(device.maintenance?.audit ?? {}),
      [requestId]: { action: request.action, actorUid: ownerUid, outcome: "requested", createdAtMs: nowMs, requestId },
    },
    activeWifiReprovisionRequestId: requestId,
  };
  return next;
}

/**
 * Pure atomic-transaction planner for an initial claim, release replacement,
 * or recipient-bound transfer. A second invocation against its returned root
 * fails because the verifier is already consumed.
 */
export function planDeviceClaim(
  root: Record<string, any>,
  id: string,
  callerUid: string,
  proof: string,
  auditId: string,
  nowMs: number,
): Record<string, any> | undefined {
  const next = structuredClone(root) as Record<string, any>;
  const device = next.devices?.[id];
  if (!device) return;
  const ownership = device.ownership ?? {};
  const verifier = device.pairing?.current as PairingVerifier | undefined;
  const initialClaim = !ownership.ownerUid && verifier?.purpose === "claim";
  const transferClaim = ownership.state === "transfer_pending" && ownership.ownerUid &&
    ownership.pendingRecipientUid === callerUid && verifier?.purpose === "transfer" && verifier.recipientUid === callerUid;
  const releaseClaim = ownership.state === "release_pending" && ownership.ownerUid && verifier?.purpose === "release";
  if ((!initialClaim && !transferClaim && !releaseClaim) || !verifier || verifier.consumedAtMs ||
      typeof verifier.expiresAtMs !== "number" || verifier.expiresAtMs <= nowMs ||
      verifier.proofHash !== pairingHash(proof)) return;

  const previousOwnerUid = typeof ownership.ownerUid === "string" ? ownership.ownerUid : undefined;
  device.ownership = {
    ...ownership,
    ownerUid: callerUid,
    state: "claimed",
    claimedAtMs: ownership.claimedAtMs ?? nowMs,
    updatedAtMs: nowMs,
    ownershipPairingRequestId: null,
    ownershipPairingExpiresAtMs: null,
    transferId: null,
    pendingRecipientUid: null,
    transferExpiresAtMs: null,
  };
  device.metadata = { ...(device.metadata ?? {}), claimedByUid: callerUid, updatedAtMs: nowMs };
  device.pairing = { ...(device.pairing ?? {}), current: { ...verifier, consumedAtMs: nowMs } };
  const auditType = initialClaim ? "claimed" : transferClaim ? "ownership_transferred" : "claimed_after_release";
  device.ownershipAudit = {
    ...(device.ownershipAudit ?? {}),
    [auditId]: { type: auditType, actorUid: callerUid, previousOwnerUid: previousOwnerUid ?? null, createdAtMs: nowMs },
  };
  next.devices = { ...(next.devices ?? {}), [id]: device };
  if (previousOwnerUid && previousOwnerUid !== callerUid) {
    const previousDevices = { ...(next.users?.[previousOwnerUid]?.devices ?? {}) };
    delete previousDevices[id];
    next.users = {
      ...(next.users ?? {}),
      [previousOwnerUid]: { ...(next.users?.[previousOwnerUid] ?? {}), devices: previousDevices },
    };
  }
  next.users = {
    ...(next.users ?? {}),
    [callerUid]: { ...(next.users?.[callerUid] ?? {}), devices: { ...(next.users?.[callerUid]?.devices ?? {}), [id]: true } },
  };
  return next;
}

async function beginOwnershipPairing(
  id: string,
  callerUid: string,
  purpose: "transfer" | "release",
  recipientUid?: string,
): Promise<{ requestId: string; expiresAtMs: number }> {
  if (purpose === "transfer" && (!recipientUid || recipientUid === callerUid)) {
    throw new HttpsError("invalid-argument", "INVALID_RECIPIENT");
  }
  if (recipientUid) {
    const recipient = await admin.auth().getUser(recipientUid).catch(() => null);
    if (!recipient || recipient.providerData.length === 0 || recipient.disabled ||
        (recipient.providerData.some((provider) => provider.providerId === "password") && !recipient.emailVerified)) {
      throw new HttpsError("failed-precondition", "RECIPIENT_NOT_ELIGIBLE");
    }
  }
  const requestId = admin.database().ref(`devices/${id}/maintenance/requests`).push().key;
  if (!requestId) throw new HttpsError("internal", "OWNERSHIP_PAIRING_UNAVAILABLE");
  const nowMs = Date.now();
  const expiresAtMs = nowMs + OWNERSHIP_PAIRING_WINDOW_MS;
  const result = await admin.database().ref().transaction((root: Record<string, any> | null) => {
    const next = root && typeof root === "object" ? structuredClone(root) as Record<string, any> : {};
    const device = next.devices?.[id];
    if (!device || device.ownership?.ownerUid !== callerUid) return;
    const activeRequestId = device.ownership?.ownershipPairingRequestId as string | undefined;
    const activeExpiresAtMs = (device.ownership?.ownershipPairingExpiresAtMs ?? device.ownership?.transferExpiresAtMs) as number | undefined;
    if ((device.ownership?.state === "transfer_pending" || device.ownership?.state === "release_pending") &&
        typeof activeExpiresAtMs === "number" && activeExpiresAtMs <= nowMs) {
      const activeRequest = activeRequestId ? device.maintenance?.requests?.[activeRequestId] : undefined;
      if (activeRequest && activeRequest.status === "pending") {
        device.maintenance.requests[activeRequestId!] = { ...activeRequest, status: "expired", completedAtMs: nowMs };
      }
      device.maintenance.audit = {
        ...(device.maintenance?.audit ?? {}),
        [`${activeRequestId ?? "ownership"}-expired`]: {
          action: "OWNERSHIP_PAIRING", actorUid: callerUid, outcome: "expired", createdAtMs: nowMs, requestId: activeRequestId ?? null,
        },
      };
      device.ownershipAudit = {
        ...(device.ownershipAudit ?? {}),
        [`${activeRequestId ?? "ownership"}-expired`]: { type: "ownership_pairing_expired", actorUid: callerUid, createdAtMs: nowMs },
      };
      device.ownership = {
        ...device.ownership,
        state: "claimed",
        ownershipPairingRequestId: null,
        ownershipPairingExpiresAtMs: null,
        transferId: null,
        pendingRecipientUid: null,
        transferExpiresAtMs: null,
        updatedAtMs: nowMs,
      };
    }
    if (device.ownership?.state !== "claimed") return;
    const request = {
      action: "OWNERSHIP_PAIRING",
      purpose,
      requestedByUid: callerUid,
      recipientUid: recipientUid ?? null,
      issuedAtMs: nowMs,
      expiresAtMs,
      nonce: crypto.randomBytes(24).toString("base64url"),
      status: "pending",
    };
    device.ownership = {
      ...device.ownership,
      state: purpose === "transfer" ? "transfer_pending" : "release_pending",
      ownershipPairingRequestId: requestId,
      ownershipPairingExpiresAtMs: expiresAtMs,
      ...(purpose === "transfer" ? { transferId: requestId, pendingRecipientUid: recipientUid, transferExpiresAtMs: expiresAtMs } : {}),
      updatedAtMs: nowMs,
    };
    device.maintenance = {
      ...(device.maintenance ?? {}),
      requests: { ...(device.maintenance?.requests ?? {}), [requestId]: request },
      audit: { ...(device.maintenance?.audit ?? {}), [requestId]: { action: request.action, purpose, actorUid: callerUid, outcome: "requested", createdAtMs: nowMs, requestId } },
    };
    device.ownershipAudit = { ...(device.ownershipAudit ?? {}), [requestId]: { type: `${purpose}_started`, actorUid: callerUid, recipientUid: recipientUid ?? null, createdAtMs: nowMs } };
    next.devices = { ...(next.devices ?? {}), [id]: device };
    return next;
  });
  if (!result.committed) throw new HttpsError("failed-precondition", "OWNERSHIP_PAIRING_UNAVAILABLE");
  return { requestId, expiresAtMs };
}

function deviceId(value: unknown): string {
  if (typeof value !== "string" || !DEVICE_ID.test(value)) throw new HttpsError("invalid-argument", "INVALID_DEVICE");
  return value;
}

function projectId(): string {
  const value = process.env.GCLOUD_PROJECT || process.env.GOOGLE_CLOUD_PROJECT;
  if (!value) throw new Error("Missing Google Cloud project ID");
  return value;
}

async function rateAllowed(id: string, nowMs: number): Promise<boolean> {
  const result = await admin.database().ref(`deviceBootstrapRate/${id}`).transaction((current: { start?: number; count?: number } | null) => {
    if (!current || nowMs - (current.start ?? 0) >= RATE_WINDOW_MS) return { start: nowMs, count: 1 };
    if ((current.count ?? 0) >= RATE_LIMIT) return;
    return { start: current.start, count: (current.count ?? 0) + 1 };
  });
  return result.committed;
}

async function nonceUnused(id: string, nonce: string, expiresAtMs: number): Promise<boolean> {
  const result = await admin.database().ref(`deviceBootstrapNonces/${id}/${nonce}`).transaction((current) => {
    if (current !== null) return;
    return { expiresAtMs };
  });
  return result.committed;
}

async function bootstrapSecret(id: string, registry: Registry): Promise<string> {
  const reference = registry.secretName || `projects/${projectId()}/secrets/smartflow-bootstrap-${id.toLowerCase()}`;
  if (!reference.startsWith(`projects/${projectId()}/secrets/`)) throw new Error("Invalid secret reference");
  const name = reference.includes("/versions/") ? reference : `${reference}/versions/latest`;
  const [version] = await secrets.accessSecretVersion({ name });
  const value = version.payload?.data ? Buffer.from(version.payload.data).toString("utf8").trim() : "";
  if (!value) throw new Error("Empty bootstrap secret");
  return value;
}

function proofValid(secret: string, id: string, timestampMs: number, nonce: string, proof: string): boolean {
  if (!/^[a-fA-F0-9]{64}$/.test(proof)) return false;
  const expected = crypto.createHmac("sha256", secret).update(`${id}.${timestampMs}.${nonce}`).digest("hex");
  return crypto.timingSafeEqual(Buffer.from(expected, "hex"), Buffer.from(proof, "hex"));
}

export const bootstrapDevice = onRequest({ region: REGION, cors: false }, async (request, response) => {
  if (request.method !== "POST") {
    response.status(405).json({ code: "METHOD_NOT_ALLOWED" });
    return;
  }
  try {
    const body = request.body as { deviceId?: unknown; timestampMs?: unknown; nonce?: unknown; proof?: unknown };
    const id = deviceId(body.deviceId);
    const timestampMs = typeof body.timestampMs === "number" ? body.timestampMs : Number.NaN;
    const nonce = typeof body.nonce === "string" ? body.nonce : "";
    const proof = typeof body.proof === "string" ? body.proof : "";
    const nowMs = Date.now();
    if (!Number.isFinite(timestampMs) || Math.abs(nowMs - timestampMs) > WINDOW_MS) {
      response.status(400).json({ code: "EXPIRED_REQUEST" }); return;
    }
    if (!NONCE.test(nonce) || !(await rateAllowed(id, nowMs))) {
      response.status(429).json({ code: "RATE_LIMITED" }); return;
    }
    const registry = (await admin.database().ref(`deviceRegistry/${id}`).get()).val() as Registry | null;
    if (!registry) { response.status(404).json({ code: "INVALID_DEVICE" }); return; }
    if (registry.state !== "active") { response.status(403).json({ code: "REVOKED_DEVICE" }); return; }
    if (!proofValid(await bootstrapSecret(id, registry), id, timestampMs, nonce, proof)) {
      response.status(403).json({ code: "INVALID_PROOF" }); return;
    }
    if (!(await nonceUnused(id, nonce, nowMs + WINDOW_MS))) {
      response.status(409).json({ code: "REPLAYED_NONCE" }); return;
    }
    const deviceUid = `device:${id}`;
    const customToken = await admin.auth().createCustomToken(deviceUid, { role: "device", deviceId: id });
    await admin.database().ref(`devices/${id}/metadata`).update({ deviceAuthUid: deviceUid, updatedAtMs: nowMs });
    response.status(200).json({ customToken, deviceUid });
  } catch (error) {
    logger.error("Device bootstrap failed", error);
    response.status(500).json({ code: "BOOTSTRAP_UNAVAILABLE" });
  }
});

export const setDeviceBootstrapState = onCall({ region: REGION }, async (request) => {
  if (request.auth?.token.admin !== true) throw new HttpsError("permission-denied", "Operator role required");
  const data = request.data as { deviceId?: unknown; state?: unknown; reason?: unknown; secretName?: unknown };
  const id = deviceId(data.deviceId);
  if (data.state !== "active" && data.state !== "revoked") throw new HttpsError("invalid-argument", "Invalid bootstrap state");
  const secretName = typeof data.secretName === "string" ? data.secretName : undefined;
  if (secretName && !secretName.startsWith(`projects/${projectId()}/secrets/`)) {
    throw new HttpsError("invalid-argument", "Invalid Secret Manager reference");
  }
  await admin.database().ref(`deviceRegistry/${id}`).update({
    state: data.state,
    ...(secretName ? { secretName } : {}),
    updatedAtMs: Date.now(),
    updatedByUid: request.auth.uid,
    reason: typeof data.reason === "string" ? data.reason.slice(0, 160) : null,
  });
  return { deviceId: id, state: data.state };
});

export const requestWifiReprovision = onCall({ region: REGION }, async (request) => {
  const caller = callableDurableAuth(request);
  const id = deviceId((request.data as { deviceId?: unknown }).deviceId);
  const requestId = admin.database().ref(`devices/${id}/maintenance/requests`).push().key;
  if (!requestId) throw new HttpsError("internal", "Unable to create request ID");
  const nowMs = Date.now();
  const nonce = crypto.randomBytes(24).toString("base64url");
  const result = await admin.database().ref().transaction((root: Record<string, any> | null) => {
    if (!root || typeof root !== "object") return;
    return planWifiReprovision(root, id, caller.uid, requestId, nonce, nowMs);
  });
  if (!result.committed) throw new HttpsError("permission-denied", "Device ownership required");
  return { requestId, expiresAtMs: nowMs + WINDOW_MS };
});

/** Atomically binds an unclaimed device to the durable user holding the active BLE proof. */
export const claimDevice = onCall({ region: REGION }, async (request) => {
  const caller = callableDurableAuth(request);
  const data = request.data as { deviceId?: unknown; pairingProof?: unknown };
  const id = deviceId(data.deviceId);
  const proof = requirePairingProof(data.pairingProof);
  const nowMs = Date.now();
  const auditId = admin.database().ref(`devices/${id}/ownershipAudit`).push().key;
  if (!auditId) throw new HttpsError("internal", "CLAIM_UNAVAILABLE");

  const result = await admin.database().ref().transaction((root: Record<string, any> | null) => {
    if (!root) return;
    return planDeviceClaim(root, id, caller.uid, proof, auditId, nowMs);
  });
  if (!result.committed) {
    const device = (await admin.database().ref(`devices/${id}`).get()).val() as { ownership?: { ownerUid?: string; state?: string }; pairing?: { current?: PairingVerifier } } | null;
    if (device?.ownership?.ownerUid && device.ownership?.state === "claimed") throw new HttpsError("already-exists", "ALREADY_CLAIMED");
    const verifier = device?.pairing?.current;
    if (verifier?.expiresAtMs && verifier.expiresAtMs <= nowMs) throw new HttpsError("deadline-exceeded", "EXPIRED_PAIRING");
    throw new HttpsError("failed-precondition", "CLAIM_UNAVAILABLE");
  }
  return { deviceId: id, status: "claimed", auditId };
});

export const startOwnershipTransfer = onCall({ region: REGION }, async (request) => {
  const caller = callableDurableAuth(request);
  const data = request.data as { deviceId?: unknown; recipientUid?: unknown };
  const id = deviceId(data.deviceId);
  if (typeof data.recipientUid !== "string") throw new HttpsError("invalid-argument", "INVALID_RECIPIENT");
  return beginOwnershipPairing(id, caller.uid, "transfer", data.recipientUid);
});

export const releaseDevice = onCall({ region: REGION }, async (request) => {
  const caller = callableDurableAuth(request);
  const id = deviceId((request.data as { deviceId?: unknown }).deviceId);
  return beginOwnershipPairing(id, caller.uid, "release");
});

export const cancelOwnershipPairing = onCall({ region: REGION }, async (request) => {
  const caller = callableDurableAuth(request);
  const id = deviceId((request.data as { deviceId?: unknown }).deviceId);
  const nowMs = Date.now();
  const result = await admin.database().ref().transaction((root: Record<string, any> | null) => {
    const next = root && typeof root === "object" ? structuredClone(root) as Record<string, any> : {};
    const device = next.devices?.[id];
    const requestId = device?.ownership?.ownershipPairingRequestId as string | undefined;
    if (!device || !requestId || device.ownership?.ownerUid !== caller.uid ||
        (device.ownership?.state !== "transfer_pending" && device.ownership?.state !== "release_pending")) return;
    const request = device.maintenance?.requests?.[requestId];
    if (!request || request.status !== "pending") return;
    device.maintenance.requests[requestId] = { ...request, status: "cancelled", completedAtMs: nowMs };
    device.maintenance.audit = { ...(device.maintenance.audit ?? {}), [`${requestId}-cancelled`]: { action: "OWNERSHIP_PAIRING", actorUid: caller.uid, outcome: "cancelled", createdAtMs: nowMs, requestId } };
    device.ownership = { ...device.ownership, state: "claimed", ownershipPairingRequestId: null, ownershipPairingExpiresAtMs: null, transferId: null, pendingRecipientUid: null, transferExpiresAtMs: null, updatedAtMs: nowMs };
    device.ownershipAudit = { ...(device.ownershipAudit ?? {}), [`${requestId}-cancelled`]: { type: "ownership_pairing_cancelled", actorUid: caller.uid, createdAtMs: nowMs } };
    next.devices = { ...(next.devices ?? {}), [id]: device };
    return next;
  });
  if (!result.committed) throw new HttpsError("failed-precondition", "NO_ACTIVE_OWNERSHIP_PAIRING");
  return { deviceId: id, status: "cancelled" };
});

/**
 * The app must call this before invoking Firebase Auth account deletion. It
 * checks authoritative owner markers rather than the convenience user index.
 */
export const checkAccountDeletionEligibility = onCall({ region: REGION }, async (request) => {
  const caller = callableDurableAuth(request);
  const devices = (await admin.database().ref("devices").get()).val() as Record<string, { ownership?: { ownerUid?: string } }> | null;
  const ownedDeviceCount = Object.values(devices ?? {}).filter((device) => device.ownership?.ownerUid === caller.uid).length;
  return { eligible: ownedDeviceCount === 0, ownedDeviceCount };
});
