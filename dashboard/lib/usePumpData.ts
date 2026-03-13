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

  const requestReboot = useCallback(async () => {
    try {
      const id = Math.max(1, Math.floor(Date.now() / 1000));
      await set(ref(db, `${CONTROL_PATH}/reboot_request_id`), id);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.request_reboot",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { reboot_request_id: id },
        });
      }
    } catch (err) {
      console.error("[RTDB] requestReboot failed:", err);
    }
  }, [authUser?.email, authUser?.uid]);

  // ── Phase 7: Smart manual/timed run writers ───────────────────────────────

  const startManualRun = useCallback(async () => {
    try {
      // If the policy mode is FORCE_OFF, switch to AUTO first, otherwise the firmware will stop runs immediately.
      if (controlRef.current?.mode === "FORCE_OFF") {
        await set(ref(db, `${CONTROL_PATH}/mode`), "AUTO");
      }
      // One-shot: toggle true then reset to false so firmware edge-detects reliably.
      await set(ref(db, `${CONTROL_PATH}/manual_start`), true);
      window.setTimeout(() => {
        void set(ref(db, `${CONTROL_PATH}/manual_start`), false);
      }, 15000); // keep high longer so ESP32 can see it even with weak WiFi
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.run_manual_start",
          uid: authUser.uid,
          email: authUser.email ?? null,
        });
      }
    } catch (err) {
      console.error("[RTDB] startManualRun failed:", err);
    }
  }, [authUser?.email, authUser?.uid]);

  const startTimedRun = useCallback(async (durationSec: number) => {
    try {
      const safeSec = Math.max(1, Math.floor(durationSec));
      // If the policy mode is FORCE_OFF, switch to AUTO first, otherwise the firmware will stop runs immediately.
      if (controlRef.current?.mode === "FORCE_OFF") {
        await set(ref(db, `${CONTROL_PATH}/mode`), "AUTO");
      }
      // One-shot: write duration then reset to 0 so firmware can detect future runs.
      await set(ref(db, `${CONTROL_PATH}/timed_start_sec`), safeSec);
      window.setTimeout(() => {
        void set(ref(db, `${CONTROL_PATH}/timed_start_sec`), 0);
      }, 15000); // keep non-zero longer so ESP32 can see it even with weak WiFi
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.run_timed_start",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { durationSec: safeSec },
        });
      }
    } catch (err) {
      console.error("[RTDB] startTimedRun failed:", err);
    }
  }, [authUser?.email, authUser?.uid]);

  const stopRun = useCallback(async () => {
    try {
      // One-shot: toggle true then reset to false so repeated stop works.
      await set(ref(db, `${CONTROL_PATH}/manual_stop`), true);
      window.setTimeout(() => {
        void set(ref(db, `${CONTROL_PATH}/manual_stop`), false);
        }, 5000); // keep high a bit longer to survive timeouts
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.run_stop",
          uid: authUser.uid,
          email: authUser.email ?? null,
        });
      }
    } catch (err) {
      console.error("[RTDB] stopRun failed:", err);
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
    requestReboot,
    startManualRun,
    startTimedRun,
    stopRun,
  };
}
