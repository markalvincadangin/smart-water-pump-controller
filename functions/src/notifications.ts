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


export function isDndActive(config: NotificationDndConfig, now = new Date()): boolean {
  if (!config.dndEnabled) return false;
  const start = config.dndStartHour ?? 22;
  const end = config.dndEndHour ?? 6;
  if (start < 0 || start > 23 || end < 0 || end > 23) return false;
  const timezone = config.timezone || "UTC";
  let hour: number;
  try {
    hour = Number(new Intl.DateTimeFormat("en-US", { timeZone: timezone, hour: "2-digit", hourCycle: "h23" }).format(now));
  } catch {
    hour = Number(new Intl.DateTimeFormat("en-US", { timeZone: "UTC", hour: "2-digit", hourCycle: "h23" }).format(now));
  }
  if (start === end) return true;
  return start < end ? hour >= start && hour < end : hour >= start || hour < end;
}
