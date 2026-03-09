"use client";

import { useEffect, useMemo, useState } from "react";
import { onValue, onDisconnect, ref, remove, serverTimestamp, set } from "firebase/database";
import { db } from "@/lib/firebase";
import { getDashboardDeviceId } from "@/lib/deviceId";

export interface PresenceInfo {
  onlineCount: number;
}

const PRESENCE_ROOT = "/pump_system/presence";

export function usePresence(uid: string | null, email: string | null) {
  const [onlineCount, setOnlineCount] = useState(0);
  const deviceId = useMemo(() => getDashboardDeviceId(), []);

  useEffect(() => {
    if (!uid) return;

    const myRef = ref(db, `${PRESENCE_ROOT}/${uid}_${deviceId}`);
    const allRef = ref(db, PRESENCE_ROOT);

    const unsub = onValue(
      allRef,
      (snap) => {
        const val = snap.val() as Record<string, unknown> | null;
        setOnlineCount(val ? Object.keys(val).length : 0);
      },
      () => {
        // ignore
      }
    );

    // Best-effort presence. If rules deny it, it should fail silently.
    Promise.resolve()
      .then(async () => {
        await set(myRef, { uid, email: email ?? null, at: serverTimestamp() });
        await onDisconnect(myRef).remove();
      })
      .catch(() => {});

    return () => {
      unsub();
      remove(myRef).catch(() => {});
    };
  }, [deviceId, email, uid]);

  return { onlineCount } satisfies PresenceInfo;
}

