// lib/useNotificationConfig.ts
"use client";

import { useEffect, useState, useCallback } from "react";
import { ref, onValue, set } from "firebase/database";
import { db } from "./firebase";
import type { NotificationConfig } from "./types";
import { DEFAULT_NOTIFICATION_CONFIG } from "./types";

export function useNotificationConfig(uid: string | null) {
  const [config, setConfig] = useState<NotificationConfig | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    if (!uid) {
      setConfig({ ...DEFAULT_NOTIFICATION_CONFIG });
      setLoading(false);
      return;
    }

    const configRef = ref(db, `/pump_system/config/notifications_by_user/${uid}`);
    const unsub = onValue(
      configRef,
      (snap) => {
        if (snap.exists()) {
          setConfig({ ...DEFAULT_NOTIFICATION_CONFIG, ...snap.val() });
        } else {
          setConfig({ ...DEFAULT_NOTIFICATION_CONFIG });
        }
        setLoading(false);
      },
      (err) => {
        console.error("[Notifications] Read failed:", err);
        setConfig({ ...DEFAULT_NOTIFICATION_CONFIG });
        setLoading(false);
      }
    );
    // Fallback: stop loading after 5s in case callback never fires
    const t = setTimeout(() => setLoading(false), 5000);
    return () => {
      unsub();
      clearTimeout(t);
    };
  }, [uid]);

  const saveConfig = useCallback(async (next: Partial<NotificationConfig>) => {
    if (!uid) {
      throw new Error("Cannot save notification config without user ID");
    }

    const configRef = ref(db, `/pump_system/config/notifications_by_user/${uid}`);
    const merged = {
      ...DEFAULT_NOTIFICATION_CONFIG,
      ...config,
      ...next,
      // Preserve fcmTokens when merging (they're device-specific)
      fcmTokens: next.fcmTokens ?? config?.fcmTokens ?? {},
    };
    await set(configRef, merged);
    // Optimistic update so UI reflects changes immediately
    setConfig(merged);
  }, [config, uid]);

  return { config, loading, saveConfig };
}
