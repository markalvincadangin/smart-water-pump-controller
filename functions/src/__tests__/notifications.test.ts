/**
 * Gold Standard: canSend / recordSent notification throttling logic.
 */
import { canSend, claimEventDelivery, claimThrottle, isDndActive, recordSent, releaseEventDelivery, releaseThrottle, THROTTLE_SEC, type LastSent } from "../notifications";
import type { Database } from "firebase-admin/database";

function mockDb(initialLastSent: LastSent | null): Database {
  let store: LastSent = initialLastSent ? { ...initialLastSent } : {};
  return {
    ref: () => ({
      get: async () => ({
        val: () => (Object.keys(store).length ? store : null),
      }),
      update: async (data: Partial<LastSent>) => {
        store = { ...store, ...data };
      },
    }),
  } as unknown as Database;
}

describe("notifications", () => {
  describe("canSend", () => {
    it("returns true when no previous send for that type", async () => {
      const db = mockDb(null);
      expect(await canSend(db, "user1", "dryRun")).toBe(true);
      expect(await canSend(db, "user1", "lowLevel")).toBe(true);
    });

    it("supports the canonical maxRuntime throttle key", async () => {
      const now = Math.floor(Date.now() / 1000);
      const db = mockDb({ maxRuntime: now - 60 });
      expect(await canSend(db, "user1", "maxRuntime")).toBe(false);
    });

    it("returns false when last send was within THROTTLE_SEC", async () => {
      const now = Math.floor(Date.now() / 1000);
      const db = mockDb({ dryRun: now - 60 }); // 1 min ago
      expect(await canSend(db, "user1", "dryRun")).toBe(false);
    });

    it("returns true when last send was at or beyond THROTTLE_SEC", async () => {
      const now = Math.floor(Date.now() / 1000);
      const db = mockDb({ dryRun: now - THROTTLE_SEC - 1 });
      expect(await canSend(db, "user1", "dryRun")).toBe(true);
    });

    it("allows different types to be sent independently", async () => {
      const now = Math.floor(Date.now() / 1000);
      const db = mockDb({ dryRun: now - 60 });
      expect(await canSend(db, "user1", "dryRun")).toBe(false);
      expect(await canSend(db, "user1", "lowLevel")).toBe(true);
    });
  });

  describe("recordSent", () => {
    it("updates last-sent for the given type without throwing", async () => {
      let store: LastSent = {};
      const db = {
        ref: () => ({
          update: async (data: Partial<LastSent>) => {
            store = { ...store, ...data };
          },
        }),
      } as unknown as Database;
      await recordSent(db, "user1", "pumpStarted");
      expect(store.pumpStarted).toBeDefined();
      expect(typeof store.pumpStarted).toBe("number");
    });
  });

  describe("THROTTLE_SEC", () => {
    it("is 15 minutes (900 seconds)", () => {
      expect(THROTTLE_SEC).toBe(15 * 60);
    });
  });
});


describe("DND", () => {
  it("handles a cross-midnight window in the configured timezone", () => {
    const config = { dndEnabled: true, dndStartHour: 22, dndEndHour: 6, timezone: "UTC" };
    expect(isDndActive(config, new Date("2026-10-04T23:00:00Z"))).toBe(true);
    expect(isDndActive(config, new Date("2026-10-04T05:59:00Z"))).toBe(true);
    expect(isDndActive(config, new Date("2026-10-04T12:00:00Z"))).toBe(false);
  });
  it("treats equal start/end as an all-day DND window", () => {
    expect(isDndActive({ dndEnabled: true, dndStartHour: 0, dndEndHour: 0, timezone: "UTC" }, new Date("2026-10-04T12:00:00Z"))).toBe(true);
  });
  it("falls back to UTC for an invalid timezone", () => {
    expect(isDndActive({ dndEnabled: true, dndStartHour: 22, dndEndHour: 6, timezone: "Not/AZone" }, new Date("2026-10-04T23:00:00Z"))).toBe(true);
  });
});


describe("atomic delivery claims", () => {
  function transactionalDb() {
    const values = new Map<string, unknown>();
    return {
      ref: (path: string) => ({
        transaction: async (update: (current: unknown) => unknown) => {
          const current = values.get(path);
          const next = update(current);
          values.set(path, next);
          return { committed: true, snapshot: { val: () => next } };
        },
        update: async (data: Record<string, unknown>) => {
          for (const [key, value] of Object.entries(data)) values.set(`${path}/${key}`, value);
        },
        remove: async () => values.delete(path),
        get: async () => ({ val: () => values.get(path) ?? null }),
      }),
    } as unknown as Database;
  }

  it("atomically claims an available throttle slot", async () => {
    const db = transactionalDb();
    expect(await claimThrottle(db, "user1", "dryRun")).toBe(true);
    expect(await claimThrottle(db, "user1", "dryRun")).toBe(false);
  });

  it("deduplicates the same authoritative event", async () => {
    const db = transactionalDb();
    expect(await claimEventDelivery(db, "user1", "-Oevent123")).toBe(true);
    expect(await claimEventDelivery(db, "user1", "-Oevent123")).toBe(false);
  });

  it("allows a failed event delivery to be retried after releasing its claim", async () => {
    const db = transactionalDb();
    expect(await claimEventDelivery(db, "user1", "-Oevent123")).toBe(true);
    await releaseEventDelivery(db, "user1", "-Oevent123");
    expect(await claimEventDelivery(db, "user1", "-Oevent123")).toBe(true);
  });

  it("releases a failed throttle claim", async () => {
    const db = transactionalDb();
    expect(await claimThrottle(db, "user1", "dryRun")).toBe(true);
    await releaseThrottle(db, "user1", "dryRun");
    expect(await claimThrottle(db, "user1", "dryRun")).toBe(true);
  });
});
