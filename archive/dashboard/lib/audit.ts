"use client";

import {
  Activity,
  Bell,
  RotateCw,
  Settings,
  ShieldCheck,
  Square,
  Timer,
  Zap,
  type LucideIcon,
} from "lucide-react";
import { push, ref, serverTimestamp, set } from "firebase/database";
import { db } from "@/lib/firebase";
import { getDashboardDeviceId } from "@/lib/deviceId";

export type AuditAction =
  | "control.set_mode"
  | "control.ack_error"
  | "control.request_reboot"
  | "control.manual_desired"
  | "control.emergency_stop"
  | "control.reset_stop"
  | "control.run_countdown_start"
  | "control.run_countdown_add_time"
  | "control.countdown_stop"
  | "control.bypass_level_sensor"
  | "control.bypass_flow_sensor"
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
  /** Optional human-readable description (v2 §10); prefer when present in ActivityPanel. */
  detail?: string;
}

const AUDIT_PATH = "/pump_system/audit/events";

export async function writeAuditEvent(input: Omit<AuditEvent, "deviceId" | "at">) {
  try {
    const deviceId = getDashboardDeviceId();
    const eventsRef = ref(db, AUDIT_PATH);
    const evtRef = push(eventsRef);
    const payload: AuditEvent = {
      ...input,
      deviceId,
      at: serverTimestamp(),
      at_ms: Date.now(),
    };
    if (input.detail != null) payload.detail = input.detail;
    await set(evtRef, payload);
  } catch {
    // Best-effort; ignore audit failures so UI controls still work.
  }
}

/**
 * Get human-readable description for an audit action.
 */
export function getAuditActionLabel(action: AuditAction): string {
  const labels: Record<AuditAction, string> = {
    "control.set_mode": "Changed mode",
    "control.ack_error": "Acknowledged error",
    "control.request_reboot": "Requested reboot",
    "control.manual_desired": "Manual intent changed",
    "control.emergency_stop": "Emergency stop",
    "control.reset_stop": "Reset stop",
    "control.run_countdown_start": "Started countdown",
    "control.run_countdown_add_time": "Added 5 min to countdown",
    "control.countdown_stop": "Stopped countdown",
    "control.bypass_level_sensor": "Level sensor bypass",
    "control.bypass_flow_sensor": "Flow sensor bypass",
    "config.device.save": "Saved device settings",
    "config.notifications.save": "Saved alert settings",
  };
  return labels[action] ?? "Activity";
}

/**
 * Get the icon for an audit action.
 */
export function getAuditActionIcon(action: AuditAction): LucideIcon {
  const icons: Record<AuditAction, LucideIcon> = {
    "control.set_mode": Zap,
    "control.ack_error": ShieldCheck,
    "control.request_reboot": RotateCw,
    "control.manual_desired": Activity,
    "control.emergency_stop": Square,
    "control.reset_stop": RotateCw,
    "control.run_countdown_start": Timer,
    "control.run_countdown_add_time": Timer,
    "control.countdown_stop": Square,
    "control.bypass_level_sensor": Settings,
    "control.bypass_flow_sensor": Settings,
    "config.device.save": Settings,
    "config.notifications.save": Bell,
  };
  return icons[action] ?? Activity;
}

