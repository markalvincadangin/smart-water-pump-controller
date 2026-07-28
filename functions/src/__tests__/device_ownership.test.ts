import { HttpsError } from "firebase-functions/v2/https";
import { ownershipTestOnly, planDeviceClaim, planLegacyOwnershipMigration, planOwnershipMigrationResolution, planWifiReprovision } from "../device_bootstrap";
import { DEVICE_ID, OTHER_UID, OWNER_UID, ownershipFixtures } from "./fixtures/device_ownership";

describe("ownership contract validation", () => {
  it("accepts a durable account and rejects an anonymous identity", () => {
    expect(ownershipTestOnly.requireDurableAccount(ownershipFixtures.durableUser)).toMatchObject({
      uid: ownershipFixtures.durableUser.uid,
    });
    expect(() => ownershipTestOnly.requireDurableAccount(ownershipFixtures.anonymousUser)).toThrow(HttpsError);
  });

  it("requires email verification for password accounts", () => {
    expect(() => ownershipTestOnly.requireDurableAccount({
      uid: "unverified-password-user", provider: "password", emailVerified: false,
    })).toThrow("EMAIL_VERIFICATION_REQUIRED");
  });

  it("validates a sufficiently random-looking proof and hashes it without retaining its raw value", () => {
    const proof = "e".repeat(64);
    expect(ownershipTestOnly.requirePairingProof(proof)).toBe(proof);
    expect(ownershipTestOnly.pairingHash(proof)).toHaveLength(64);
    expect(ownershipTestOnly.pairingHash(proof)).not.toContain(proof);
    expect(() => ownershipTestOnly.requirePairingProof("short")).toThrow("INVALID_PAIRING_PROOF");
  });

  it("migrates only a consistent legacy owner and freezes contradictory records", () => {
    const migrated = planLegacyOwnershipMigration(ownershipFixtures.consistentLegacyOwner, DEVICE_ID, 1_000);
    expect(migrated.outcome).toBe("migrated");
    expect(migrated.root.devices[DEVICE_ID].ownership).toMatchObject({
      ownerUid: OWNER_UID, state: "claimed", migrationState: "migrated",
    });

    const conflict = planLegacyOwnershipMigration(ownershipFixtures.conflictingLegacyOwner, DEVICE_ID, 2_000);
    expect(conflict).toMatchObject({ outcome: "conflict", code: "CONFLICTING_LEGACY_OWNER" });
    expect(conflict.root.devices[DEVICE_ID].ownership.migrationState).toBe("conflict");
  });

  it("resolves only an explicitly frozen conflict with an audit record", () => {
    const conflict = planLegacyOwnershipMigration(ownershipFixtures.conflictingLegacyOwner, DEVICE_ID, 2_000);
    const resolved = planOwnershipMigrationResolution(conflict.root, DEVICE_ID, OWNER_UID, "operator-1", "CASE-123", 3_000);
    expect(resolved?.devices[DEVICE_ID].ownership).toMatchObject({
      ownerUid: OWNER_UID, state: "claimed", migrationState: "resolved",
    });
    expect(resolved?.users[OWNER_UID].devices[DEVICE_ID]).toBe(true);
    expect(planOwnershipMigrationResolution(ownershipFixtures.claimed, DEVICE_ID, OWNER_UID, "operator-1", "CASE-123", 3_000)).toBeUndefined();
  });

  it("atomically claims an unclaimed device and consumes the proof", () => {
    const proof = "a".repeat(64);
    const root = structuredClone(ownershipFixtures.unclaimed) as Record<string, any>;
    root.devices[DEVICE_ID].pairing = {
      current: { proofHash: ownershipTestOnly.pairingHash(proof), purpose: "claim", expiresAtMs: 2_000 },
    };
    const claimed = planDeviceClaim(root, DEVICE_ID, OWNER_UID, proof, "audit-1", 1_000);
    expect(claimed?.devices[DEVICE_ID].ownership).toMatchObject({ ownerUid: OWNER_UID, state: "claimed" });
    expect(claimed?.users[OWNER_UID].devices[DEVICE_ID]).toBe(true);
    expect(claimed?.devices[DEVICE_ID].pairing.current.consumedAtMs).toBe(1_000);
    expect(planDeviceClaim(claimed!, DEVICE_ID, OTHER_UID, proof, "audit-2", 1_001)).toBeUndefined();
  });

  it("rejects invalid and expired pairing proofs without partial ownership writes", () => {
    const proof = "b".repeat(64);
    const root = structuredClone(ownershipFixtures.unclaimed) as Record<string, any>;
    root.devices[DEVICE_ID].pairing = {
      current: { proofHash: ownershipTestOnly.pairingHash(proof), purpose: "claim", expiresAtMs: 1_000 },
    };
    expect(planDeviceClaim(root, DEVICE_ID, OWNER_UID, "c".repeat(64), "audit-invalid", 999)).toBeUndefined();
    expect(planDeviceClaim(root, DEVICE_ID, OWNER_UID, proof, "audit-expired", 1_000)).toBeUndefined();
    expect(root.devices[DEVICE_ID].ownership).toBeUndefined();
  });

  it("creates an owner-authorized, bounded Wi-Fi recovery request and audit atomically", () => {
    const request = planWifiReprovision(
      ownershipFixtures.claimed, DEVICE_ID, OWNER_UID, "recovery-1", "n".repeat(32), 1_000,
    );
    expect(request?.devices[DEVICE_ID].maintenance).toMatchObject({
      activeWifiReprovisionRequestId: "recovery-1",
      requests: {
        "recovery-1": {
          action: "WIFI_REPROVISION", requestedByUid: OWNER_UID, issuedAtMs: 1_000,
          expiresAtMs: 301_000, status: "pending",
        },
      },
      audit: { "recovery-1": { outcome: "requested", actorUid: OWNER_UID } },
    });
    expect(request?.devices[DEVICE_ID].ownership).toEqual(ownershipFixtures.claimed.devices[DEVICE_ID].ownership);
  });

  it("rejects non-owners without creating a Wi-Fi recovery request", () => {
    expect(planWifiReprovision(
      ownershipFixtures.claimed, DEVICE_ID, OTHER_UID, "recovery-2", "n".repeat(32), 1_000,
    )).toBeUndefined();
    expect(ownershipFixtures.claimed.devices[DEVICE_ID]).not.toHaveProperty("maintenance");
  });
});
