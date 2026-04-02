// lib/usePumpData.ts
"use client";

import { useEffect, useRef, useState, useCallback } from "react";
import { ref, onValue, set, update } from "firebase/database";
import { onAuthStateChanged } from "firebase/auth";
import { db, auth } from "./firebase";
import type { PumpStatus, PumpControl, PumpSnapshot, HistoryEntry, HistoryEvent } from "./types";
import { writeAuditEvent } from "@/lib/audit";
import { formatPhtTime } from "@/lib/time";

const STATUS_PATH = "/pump_system/status";
const CONTROL_PATH = "/pump_system/control";
const MAX_HISTORY = 60; // Keep last 60 data points (~3 min at 3s intervals)

const DEFAULT_STATUS: PumpStatus = {
  is_running: false,
  flow_rate_lpm: 0,
  run_mode: "AUTO_STANDBY",
  pump_cooldown_remaining_sec: 0,
  is_error: false,
  is_sensor_error: false,
  is_flow_sensor_error: false,
  is_overflow_error: false,
  last_fault_code: "",
  last_fault_message: "",
  is_idle_mode: false,
  is_sleeping: false,
  emergency_stop_latched: false,
  manual_desired: false,
  bypass_level_sensor: false,
  bypass_flow_sensor: false,
  manual_runtime_warning: false,
  remote_sensor_stable: false,
  level_fresh: false,
  level_sensor_health_pct: 0,
  level_estimate_active: false,
  remote_level_discard_count: 0,
  countdown_remaining_sec: 0,
  flow_volume_added_l: 0,
  wifi_rssi: 0,
  uptime_minutes: 0,
  last_boot_reason: "",
  debug_log_level: 2,
  total_pump_cycles: 0,
  total_pump_run_min: 0,
  ultrasonic_cycles_ok: 0,
  ultrasonic_cycles_timeout: 0,
  ultrasonic_last_good_cm: 0,
  free_heap_bytes: 0,
  min_free_heap_observed_bytes: 0,
  firebase_consecutive_failures: 0,
  firebase_last_error: "",
};

const DEFAULT_CONTROL: PumpControl = {
  mode: "AUTO",
  manual_desired: false,
  emergency_stop: false,
  reset_stop: false,
  clear_error: false,
  countdown_start: false,
  countdown_duration_min: 10,
  countdown_add_time: false,
  countdown_add_min: 5,
  bypass_level_sensor: false,
  bypass_flow_sensor: false,
  reboot_request_id: 0,
};

export function usePumpData() {
  const [snapshot, setSnapshot] = useState<PumpSnapshot | null>(null);
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [historyEvents, setHistoryEvents] = useState<HistoryEvent[]>([]);
  const [connected, setConnected] = useState(false);
  const [authChecked, setAuthChecked] = useState(false);
  const [authUser, setAuthUser] = useState<{ uid: string; email: string | null } | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [isAddingCountdownTime, setIsAddingCountdownTime] = useState(false);
  const [lastUpdateAtMs, setLastUpdateAtMs] = useState<number | null>(null);

  // Keep refs so callbacks don't close over stale state
  const statusRef = useRef<PumpStatus>(DEFAULT_STATUS);
  const controlRef = useRef<PumpControl>(DEFAULT_CONTROL);
  const lastRunModeRef = useRef<string | null>(null);
  const lastIsRunningRef = useRef<boolean | null>(null);
  const lastFaultCodeRef = useRef<string | null>(null);

  // authReady = Firebase auth has been checked AND we have a signed-in user
  const authReady = authChecked && !!authUser;


  // ── Wait for signed-in user (Google Auth via login page) ───────────────────
  useEffect(() => {
    const unsub = onAuthStateChanged(auth, (user) => {
      setAuthChecked(true);
      setAuthUser(user ? { uid: user.uid, email: user.email ?? null } : null);
    });
    return () => unsub();
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
        if (!snap.exists()) {
          setConnected(false);
          return;
        }
        const raw = snap.val() as (Partial<PumpStatus> & {
          is_level_sensor_error?: boolean;
          min_free_heap_bytes?: number;
        }) | null;
        if (!raw) return;
        // REFACTOR [N-06/N-07]: normalize firmware canonical keys into dashboard model.
        const data: PumpStatus = {
          ...DEFAULT_STATUS,
          ...raw,
          is_sensor_error: raw.is_sensor_error ?? raw.is_level_sensor_error ?? false,
          min_free_heap_observed_bytes:
            raw.min_free_heap_observed_bytes ?? raw.min_free_heap_bytes ?? 0,
        };
        statusRef.current = data;
        setConnected(true);
        setError(null);
        setLastUpdateAtMs(Date.now());

        // Append to rolling history
        const timeLabel = formatPhtTime(Date.now());

        // Append to rolling history (level + flow)
          setHistory((prev) => {
            const next: HistoryEntry[] = [
              ...prev,
              {
                time: timeLabel,
                level: data.water_level_percent ?? 0,
                flow: parseFloat(data.flow_rate_lpm.toFixed(2)),
              },
            ];
            return next.length > MAX_HISTORY ? next.slice(-MAX_HISTORY) : next;
          });

        // Derive lightweight event markers for the history chart
        const runMode = (data.run_mode ?? "") as string;
        const isRunning = !!data.is_running;
        const faultCode = (data.last_fault_code ?? "") as string;

        setHistoryEvents((prev) => {
          const events: HistoryEvent[] = [];

          const lastRunMode = lastRunModeRef.current;
          const lastIsRunning = lastIsRunningRef.current;
          const lastFaultCode = lastFaultCodeRef.current;

          if (lastRunMode && runMode && runMode !== lastRunMode) {
            events.push({
              time: timeLabel,
              type: "mode_change",
              runMode,
              prevRunMode: lastRunMode,
            });
          }

          if (lastIsRunning === false && isRunning === true) {
            events.push({
              time: timeLabel,
              type: "run_start",
              runMode,
            });
          } else if (lastIsRunning === true && isRunning === false) {
            events.push({
              time: timeLabel,
              type: "run_stop",
              runMode,
            });
          }

          if (faultCode && faultCode !== lastFaultCode && faultCode !== "") {
            events.push({
              time: timeLabel,
              type: "fault",
              runMode,
              faultCode,
            });
          }

          lastRunModeRef.current = runMode || lastRunMode || null;
          lastIsRunningRef.current = isRunning;
          lastFaultCodeRef.current = faultCode || lastFaultCode || null;

          if (events.length === 0) return prev;
          const next = [...prev, ...events];
          return next.length > MAX_HISTORY ? next.slice(-MAX_HISTORY) : next;
        });

        setSnapshot((prev: PumpSnapshot | null) => ({
          status: data,
          control: prev?.control ?? controlRef.current,
          updatedAt: Date.now(),
        }));
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
        const data = snap.val() as PumpControl | null;
        if (!data) return;
        controlRef.current = data;
        setSnapshot((prev: PumpSnapshot | null) =>
          prev ? { ...prev, control: data } : null
        );
        setLastUpdateAtMs(Date.now());
      }
    });

    return () => {
      unsubStatus();
      unsubControl();
    };
  }, [authReady]);

  // ── Control writers ──────────────────────────────────────────────────────

  const setMode = useCallback(async (newMode: PumpControl["mode"]) => {
    try {
      const prevMode = controlRef.current?.mode ?? "AUTO";

      // Atomically write new mode + safety-reset stale flags from previous mode
      const updates: Record<string, unknown> = { mode: newMode };

      // Leaving MANUAL: clear persistent pump intent so re-entering MANUAL doesn't auto-start
      if (prevMode === "MANUAL" && newMode !== "MANUAL") {
        updates.manual_desired = false;
      }
      // Leaving COUNTDOWN: reset all one-shot countdown flags
      if (prevMode === "COUNTDOWN" && newMode !== "COUNTDOWN") {
        updates.countdown_start = false;
        updates.countdown_add_time = false;
        updates.countdown_stop = false;
        updates.countdown_add_min = 0;
      }

      await update(ref(db, CONTROL_PATH), updates);

      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.set_mode",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { mode: newMode },
          detail: `Mode changed from ${prevMode} to ${newMode}`,
        });
      }
    } catch (err) {
      console.error("[RTDB] setMode failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const setManualDesired = useCallback(async (on: boolean) => {
    try {
      // Use update so we atomically set both fields (mode only if not already MANUAL)
      const updates: Record<string, unknown> = { manual_desired: on };
      if (controlRef.current?.mode !== "MANUAL") updates.mode = "MANUAL";
      await update(ref(db, CONTROL_PATH), updates);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.manual_desired",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { manual_desired: on },
          detail: on ? "Manual pump START requested" : "Manual pump STOP requested",
        });
      }
    } catch (err) {
      console.error("[RTDB] setManualDesired failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const triggerEmergencyStop = useCallback(async () => {
    try {
      const updates = {
        emergency_stop: true,
        mode: "AUTO",
        manual_desired: false,
        countdown_start: false,
        countdown_stop: true
      };
      await update(ref(db, CONTROL_PATH), updates);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.emergency_stop",
          uid: authUser.uid,
          email: authUser.email ?? null,
          detail: "Emergency stop requested",
        });
      }
    } catch (err) {
      console.error("[RTDB] triggerEmergencyStop failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const resetEmergencyStop = useCallback(async () => {
    try {
      await set(ref(db, `${CONTROL_PATH}/reset_stop`), true);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.reset_stop",
          uid: authUser.uid,
          email: authUser.email ?? null,
          detail: "Reset stop requested",
        });
      }
    } catch (err) {
      console.error("[RTDB] resetEmergencyStop failed:", err);
      throw err;
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
          detail: "Error acknowledged and cleared",
        });
      }
    } catch (err) {
      console.error("[RTDB] acknowledgeError failed:", err);
      throw err;
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
          detail: "Controller reboot requested",
        });
      }
    } catch (err) {
      console.error("[RTDB] requestReboot failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const startCountdown = useCallback(async (durationMin: number) => {
    try {
      const safeMin = Math.max(1, Math.min(120, Math.floor(durationMin)));
      // Atomic update: set start and explicitly clear stop to prevent race conditions
      const updates = {
        countdown_duration_min: safeMin,
        mode: "COUNTDOWN",
        countdown_start: true,
        countdown_stop: false
      };
      await update(ref(db, CONTROL_PATH), updates);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.run_countdown_start",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { durationMin: safeMin },
          detail: `Countdown started: ${safeMin} min`,
        });
      }
    } catch (err) {
      console.error("[RTDB] startCountdown failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const addCountdownTime = useCallback(async (minutes: number = 5) => {
    try {
      const addMin = Math.max(1, Math.min(120, Math.floor(minutes)));
      setIsAddingCountdownTime(true);
      await set(ref(db, `${CONTROL_PATH}/countdown_add_min`), addMin);
      await set(ref(db, `${CONTROL_PATH}/countdown_add_time`), true);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.run_countdown_add_time",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { addMin },
          detail: `${addMin} min added to countdown`,
        });
      }
    } catch (err) {
      console.error("[RTDB] addCountdownTime failed:", err);
      setIsAddingCountdownTime(false);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  // Clear isAddingCountdownTime when firmware resets countdown_add_time to false (Firebase roundtrip)
  useEffect(() => {
    if (snapshot?.control?.countdown_add_time === false && isAddingCountdownTime) {
      setIsAddingCountdownTime(false);
    }
  }, [snapshot?.control?.countdown_add_time, isAddingCountdownTime]);

  const stopCountdown = useCallback(async () => {
    try {
      // Atomic update: set stop and explicitly clear start to prevent race conditions
      const updates = {
        countdown_stop: true,
        countdown_start: false
      };
      await update(ref(db, CONTROL_PATH), updates);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.countdown_stop",
          uid: authUser.uid,
          email: authUser.email ?? null,
          detail: "Countdown stopped (mode remains COUNTDOWN)",
        });
      }
    } catch (err) {
      console.error("[RTDB] stopCountdown failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const setBypassLevelSensor = useCallback(async (value: boolean) => {
    try {
      await set(ref(db, `${CONTROL_PATH}/bypass_level_sensor`), value);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.bypass_level_sensor",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { bypass_level_sensor: value },
          detail: value ? "Level sensor bypass enabled (maintenance mode)" : "Level sensor bypass disabled",
        });
      }
    } catch (err) {
      console.error("[RTDB] setBypassLevelSensor failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const setBypassFlowSensor = useCallback(async (value: boolean) => {
    try {
      await set(ref(db, `${CONTROL_PATH}/bypass_flow_sensor`), value);
      if (authUser?.uid) {
        await writeAuditEvent({
          action: "control.bypass_flow_sensor",
          uid: authUser.uid,
          email: authUser.email ?? null,
          meta: { bypass_flow_sensor: value },
          detail: value ? "Flow sensor bypass enabled (maintenance mode)" : "Flow sensor bypass disabled",
        });
      }
    } catch (err) {
      console.error("[RTDB] setBypassFlowSensor failed:", err);
      throw err;
    }
  }, [authUser?.email, authUser?.uid]);

  const status: PumpStatus | null = snapshot?.status ?? null;
  const control: PumpControl | null = snapshot?.control ?? null;
  const degraded: boolean =
    !!lastUpdateAtMs && Date.now() - lastUpdateAtMs > 30000 && Date.now() - lastUpdateAtMs <= 60000;

  return {
    // Preferred public fields
    status,
    control,
    history,
    historyEvents,
    connected,
    degraded,
    authReady,
    authChecked,
    authUser,
    error,
    // Back-compat combined snapshot (used by some call sites)
    snapshot,
    // Writers and helpers
    setMode,
    acknowledgeError,
    requestReboot,
    setManualDesired,
    startCountdown,
    addCountdownTime,
    isAddingCountdownTime,
    stopCountdown,
    triggerEmergencyStop,
    resetEmergencyStop,
    setBypassLevelSensor,
    setBypassFlowSensor,
  };
}
