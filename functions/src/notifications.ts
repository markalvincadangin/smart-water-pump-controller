/**
 * Notification throttle and record helpers — testable without Firebase Admin init.
 * Used by Cloud Function onStatusChange.
 */

import type { Database } from "firebase-admin/database";

export const THROTTLE_SEC = 15 * 60; // 15 minutes

export interface LastSent {
  dryRun?: number;
  lowLevel?: number;
  pumpStarted?: number;
  overflow?: number;
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
  const lastRef = db.ref(`pump_system/config/notification_last_sent/${uid}`);
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
  const lastRef = db.ref(`pump_system/config/notification_last_sent/${uid}`);
  await lastRef.update({ [type]: Math.floor(Date.now() / 1000) });
}
