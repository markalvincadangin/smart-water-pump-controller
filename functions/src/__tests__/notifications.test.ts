/**
 * Gold Standard: canSend / recordSent notification throttling logic.
 */
import { canSend, recordSent, THROTTLE_SEC, type LastSent } from "../notifications";
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
