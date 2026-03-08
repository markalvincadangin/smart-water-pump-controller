"use client";

import { useEffect, useState, useCallback } from "react";
import { ref, onValue, set, get } from "firebase/database";
import { db } from "./firebase";
import type { DeviceConfig } from "./types";
import { DEFAULT_DEVICE_CONFIG } from "./types";

const DEVICE_CONFIG_PATH = "/pump_system/config/device";

export function useDeviceConfig() {
  const [config, setConfig] = useState<DeviceConfig | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const configRef = ref(db, DEVICE_CONFIG_PATH);
    const unsub = onValue(
      configRef,
      (snap) => {
        if (snap.exists()) {
          const val = snap.val();
          setConfig({
            ...DEFAULT_DEVICE_CONFIG,
            tank_empty_cm: val.tank_empty_cm ?? DEFAULT_DEVICE_CONFIG.tank_empty_cm,
            tank_full_cm: val.tank_full_cm ?? DEFAULT_DEVICE_CONFIG.tank_full_cm,
            pump_start_level: val.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level,
            pump_stop_level: val.pump_stop_level ?? DEFAULT_DEVICE_CONFIG.pump_stop_level,
            dry_run_threshold_lpm: val.dry_run_threshold_lpm ?? DEFAULT_DEVICE_CONFIG.dry_run_threshold_lpm,
            dry_run_timeout_sec: val.dry_run_timeout_sec ?? DEFAULT_DEVICE_CONFIG.dry_run_timeout_sec,
            flow_calibration_factor: val.flow_calibration_factor ?? DEFAULT_DEVICE_CONFIG.flow_calibration_factor,
            max_pump_runtime_min: val.max_pump_runtime_min ?? DEFAULT_DEVICE_CONFIG.max_pump_runtime_min,
            sleep_enabled: val.sleep_enabled ?? DEFAULT_DEVICE_CONFIG.sleep_enabled,
            sleep_start_hour: val.sleep_start_hour ?? DEFAULT_DEVICE_CONFIG.sleep_start_hour,
            sleep_end_hour: val.sleep_end_hour ?? DEFAULT_DEVICE_CONFIG.sleep_end_hour,
            sleep_emergency_level: val.sleep_emergency_level ?? DEFAULT_DEVICE_CONFIG.sleep_emergency_level,
            sensor_failure_threshold: val.sensor_failure_threshold ?? DEFAULT_DEVICE_CONFIG.sensor_failure_threshold,
            idle_sensor_interval_ms: val.idle_sensor_interval_ms ?? DEFAULT_DEVICE_CONFIG.idle_sensor_interval_ms,
            idle_firebase_interval_ms: val.idle_firebase_interval_ms ?? DEFAULT_DEVICE_CONFIG.idle_firebase_interval_ms,
          });
        } else {
          setConfig({ ...DEFAULT_DEVICE_CONFIG });
        }
        setLoading(false);
      },
      (err) => {
        console.error("[DeviceConfig] Read failed:", err);
        setConfig({ ...DEFAULT_DEVICE_CONFIG });
        setLoading(false);
      }
    );
    const t = setTimeout(() => setLoading(false), 5000);
    return () => {
      unsub();
      clearTimeout(t);
    };
  }, []);

  const saveConfig = useCallback(async (next: Partial<DeviceConfig>) => {
    const configRef = ref(db, DEVICE_CONFIG_PATH);
    const merged: DeviceConfig = {
      ...DEFAULT_DEVICE_CONFIG,
      ...config,
      ...next,
    };
    await set(configRef, merged);
    // Optimistic update so StatCard labels and other UI reflect changes immediately
    setConfig(merged);
  }, [config]);

  /** One-time seed: write default config if path is empty (so ESP32 can read it). */
  const seedDefaultsIfEmpty = useCallback(async () => {
    const configRef = ref(db, DEVICE_CONFIG_PATH);
    const snap = await get(configRef);
    if (!snap.exists()) {
      await set(configRef, DEFAULT_DEVICE_CONFIG);
    }
  }, []);

  return { config, loading, saveConfig, seedDefaultsIfEmpty };
}
