"use client";

import { push, ref, serverTimestamp, set } from "firebase/database";
import { db } from "@/lib/firebase";
import { getDashboardDeviceId } from "@/lib/deviceId";

export type AuditAction =
  | "control.set_mode"
  | "control.ack_error"
  | "config.device.save"
  | "config.notifications.save";

export interface AuditEvent {
  action: AuditAction;
  uid: string;
  email?: string | null;
  deviceId: string;
  at: unknown; // serverTimestamp()
  at_ms?: number; // client timestamp for reliable UI sorting/relative time
  meta?: Record<string, unknown>;
}

const AUDIT_PATH = "/pump_system/audit/events";

export async function writeAuditEvent(input: Omit<AuditEvent, "deviceId" | "at">) {
  try {
    const deviceId = getDashboardDeviceId();
    const eventsRef = ref(db, AUDIT_PATH);
    const evtRef = push(eventsRef);
    await set(evtRef, {
      ...input,
      deviceId,
      at: serverTimestamp(),
      at_ms: Date.now(),
    } satisfies AuditEvent);
  } catch {
    // Best-effort; ignore audit failures so UI controls still work.
  }
}

