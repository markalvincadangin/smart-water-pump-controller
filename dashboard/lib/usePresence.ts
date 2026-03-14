"use client";

/**
 * Presence was removed per FIRMWARE_DASHBOARD_DESIGN_v2 §2:
 * /pump_system/presence is no longer used. This hook is a no-op and returns
 * onlineCount 0 so existing UI that passes it to StatusBar simply shows no "X users" badge.
 */
export interface PresenceInfo {
  onlineCount: number;
}

export function usePresence(
  _uid?: string | null,
  _email?: string | null
): PresenceInfo {
  void _uid;
  void _email;
  return { onlineCount: 0 };
}

