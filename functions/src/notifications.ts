/**
 * Notification throttle and record helpers — testable without Firebase Admin init.
 * Used by Cloud Function onStatusChange.
 */

import type { Database } from "firebase-admin/database";

export interface NotificationDndConfig {
  dndEnabled?: boolean;
  dndStartHour?: number;
  dndEndHour?: number;
  timezone?: string;
}

export const THROTTLE_SEC = 15 * 60; // 15 minutes

export interface LastSent {
  dryRun?: number;
  lowLevel?: number;
  pumpStarted?: number;
  maxRuntime?: number;
}

export type NotificationType = keyof LastSent;

/**
 * Returns true if we are allowed to send a notification of the given type for this user
 * (i.e. at least THROTTLE_SEC seconds since last send).
 */
export async function canSend(
  db: Database,
  uid: string,
  type: NotificationType
): Promise<boolean> {
  const lastRef = db.ref(`users/${uid}/notification_last_sent`);
  const snap = await lastRef.get();
  const last: LastSent = snap.val() || {};
  const now = Math.floor(Date.now() / 1000);
  const lastTime = last[type] ?? 0;
  if (now - lastTime < THROTTLE_SEC) return false;
  return true;
}

/**
 * Records that a notification was sent for the given type (updates last-sent timestamp).
 */
export async function recordSent(
  db: Database,
  uid: string,
  type: NotificationType
): Promise<void> {
  const lastRef = db.ref(`users/${uid}/notification_last_sent`);
  await lastRef.update({ [type]: Math.floor(Date.now() / 1000) });
}

/**
 * Atomically claims a throttle slot. Unlike canSend()+recordSent(), this
 * prevents concurrent function invocations from both passing the same check.
 */
export async function claimThrottle(
  db: Database,
  uid: string,
  type: NotificationType
): Promise<boolean> {
  const ref = db.ref(`users/${uid}/notification_last_sent/${type}`);
  const now = Math.floor(Date.now() / 1000);

  const result = await ref.transaction((current: unknown) => {
    const lastTime = typeof current === "number" ? current : 0;
    if (now - lastTime < THROTTLE_SEC) return current;
    return now;
  });

  return result.committed && result.snapshot.val() === now;
}

export async function claimEventDelivery(
  db: Database,
  uid: string,
  deviceId: string,
  eventId: string
): Promise<boolean> {
  if (!deviceId || !eventId) return false;

  const safeDeviceId = deviceId.replace(/[.#$]/g, "_").replace(/[\/\[\]]/g, "_");
  const safeEventId = eventId.replace(/[.#$]/g, "_").replace(/[\/\[\]]/g, "_");
  const ref = db.ref(`users/${uid}/notification_delivery/${safeDeviceId}/${safeEventId}`);
  const claimValue = Date.now();

  const result = await ref.transaction((current: unknown) => {
    if (current != null) return current;
    return { claimedAt: claimValue };
  });

  const snapshot = result.snapshot.val();
  return result.committed && snapshot?.claimedAt === claimValue;
}


