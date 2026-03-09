// lib/usePumpData.ts
"use client";

import { useEffect, useRef, useState, useCallback } from "react";
import { ref, onValue, set } from "firebase/database";
import { onAuthStateChanged } from "firebase/auth";
import { db, auth } from "./firebase";
import type { PumpStatus, PumpControl, PumpSnapshot, HistoryEntry } from "./types";
import { writeAuditEvent } from "@/lib/audit";

const STATUS_PATH = "/pump_system/status";
const CONTROL_PATH = "/pump_system/control";
const MAX_HISTORY = 60; // Keep last 60 data points (~3 min at 3s intervals)

const DEFAULT_STATUS: PumpStatus = {
  water_level_percent: 0,
  is_running: false,
  flow_rate_lpm: 0,
  is_error: false,
  is_sensor_error: false,
  is_overflow_error: false,
  wifi_rssi: 0,
  last_boot_reason: "",
  uptime_minutes: 0,
};

const DEFAULT_CONTROL: PumpControl = {
  mode: "AUTO",
  clear_error: false,
};

export function usePumpData() {
  const [snapshot, setSnapshot] = useState<PumpSnapshot | null>(null);
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [connected, setConnected] = useState(false);
  const [authChecked, setAuthChecked] = useState(false);
  const [authUser, setAuthUser] = useState<{ uid: string; email: string | null } | null>(null);
  const [error, setError] = useState<string | null>(null);

  // Keep refs so callbacks don't close over stale state
  const statusRef = useRef<PumpStatus>(DEFAULT_STATUS);
  const controlRef = useRef<PumpControl>(DEFAULT_CONTROL);

  // authReady = Firebase auth has been checked AND we have a signed-in user
  const authReady = authChecked && !!authUser;

  // Locale for timestamps — prefer browser setting when available
  const [timeLocale, setTimeLocale] = useState<string>("en-PH");

  // ── Wait for signed-in user (Google Auth via login page) ───────────────────
  useEffect(() => {
    const unsub = onAuthStateChanged(auth, (user) => {
      setAuthChecked(true);
      setAuthUser(user ? { uid: user.uid, email: user.email ?? null } : null);
    });
    return () => unsub();
  }, []);

  // Determine locale for chart timestamps (client-side only)
  useEffect(() => {
    if (typeof window === "undefined") return;
    const locale = window.navigator?.language || window.navigator?.languages?.[0];
    if (locale && typeof locale === "string") {
      setTimeLocale(locale);
    }
  }, []);

  // ── Real-time listeners ──────────────────────────────────────────────────
  useEffect(() => {
    if (!authReady) return;

    const statusDbRef = ref(db, STATUS_PATH);
    const controlDbRef = ref(db, CONTROL_PATH);

    // Listen to /pump_system/status
    const unsubStatus = onValue(
      statusDbRef,
      (snap) => {
        if (snap.exists()) {
          const data = snap.val() as PumpStatus;
          statusRef.current = data;
          setConnected(true);
          setError(null);

          // Append to rolling history
          const now = new Date();
          const timeLabel = now.toLocaleTimeString(timeLocale, { hour12: false });
          setHistory((prev) => {
            const next = [
              ...prev,
              {
                time: timeLabel,
                level: data.water_level_percent,
                flow: parseFloat(data.flow_rate_lpm.toFixed(2)),
              },
            ];
            return next.length > MAX_HISTORY ? next.slice(-MAX_HISTORY) : next;
          });

          setSnapshot(() => ({
            status: data,
            control: controlRef.current,
            updatedAt: Date.now(),
          }));
        }
      },
      (err) => {
        console.error("[RTDB]", err);
        setConnected(false);
        setError(err.message);
      }
    );

    // Listen to /pump_system/control
    const unsubControl = onValue(controlDbRef, (snap) => {
      if (snap.exists()) {
        const data = snap.val() as PumpControl;
        controlRef.current = data;
        setSnapshot((prev) =>
          prev ? { ...prev, control: data } : null
        );
      }
    });

    return () => {
      unsubStatus();
      unsubControl();
    };
  }, [authReady, timeLocale]);

  // ── Control writers ──────────────────────────────────────────────────────

  const setMode = useCallback(async (mode: PumpControl["mode"]) => {
    try {
      await set(ref(db, `${CONTROL_PATH}/mode`), mode);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.set_mode",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { mode },
        });
      }
    } catch (err) {
      console.error("[RTDB] setMode failed:", err);
    }
  }, [authUser?.email, authUser?.uid]);

  const acknowledgeError = useCallback(async () => {
    try {
      await set(ref(db, `${CONTROL_PATH}/clear_error`), true);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.ack_error",
          uid: authUser.uid,
          email: authUser.email ?? null,
        });
      }
    } catch (err) {
      console.error("[RTDB] acknowledgeError failed:", err);
    }
  }, [authUser?.email, authUser?.uid]);

  return {
    snapshot,
    history,
    connected,
    authReady,
    authChecked,
    authUser,
    error,
    setMode,
    acknowledgeError,
  };
}
