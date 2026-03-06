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
