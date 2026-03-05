// lib/useNotificationConfig.ts
"use client";

import { useEffect, useState, useCallback } from "react";
import { ref, onValue, set } from "firebase/database";
import { db } from "./firebase";
import type { NotificationConfig } from "./types";
import { DEFAULT_NOTIFICATION_CONFIG } from "./types";

const CONFIG_PATH = "/pump_system/config/notifications";

export function useNotificationConfig() {
  const [config, setConfig] = useState<NotificationConfig | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const configRef = ref(db, CONFIG_PATH);
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
  }, []);

  const saveConfig = useCallback(async (next: Partial<NotificationConfig>) => {
    const configRef = ref(db, CONFIG_PATH);
    const merged = { ...DEFAULT_NOTIFICATION_CONFIG, ...config, ...next };
    await set(configRef, merged);
  }, [config]);

  return { config, loading, saveConfig };
}
