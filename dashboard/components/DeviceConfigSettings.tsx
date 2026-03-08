// components/DeviceConfigSettings.tsx
"use client";

import { useState, useEffect } from "react";
import { Settings, X } from "lucide-react";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
import InfoTooltip from "./InfoTooltip";
import type { DeviceConfig } from "@/lib/types";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";

interface DeviceConfigSettingsProps {
  onClose: () => void;
}

export default function DeviceConfigSettings({ onClose }: DeviceConfigSettingsProps) {
  const { config, loading, saveConfig, seedDefaultsIfEmpty } = useDeviceConfig();
  const [form, setForm] = useState<DeviceConfig>({ ...DEFAULT_DEVICE_CONFIG });
  const [saving, setSaving] = useState(false);
  const [seeding, setSeeding] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [saveSuccess, setSaveSuccess] = useState(false);

  useEffect(() => {
    if (config) setForm(config);
  }, [config]);

  function validate(): string | null {
    if (form.tank_full_cm >= form.tank_empty_cm) return "Tank full (cm) must be less than tank empty (cm).";
    if (form.pump_start_level >= form.pump_stop_level) return "Pump start % must be less than pump stop %.";
    if (form.tank_empty_cm < 5 || form.tank_empty_cm > 200) return "Tank empty: 5–200 cm.";
    if (form.tank_full_cm < 1 || form.tank_full_cm >= form.tank_empty_cm) return "Tank full: 1 to (tank empty − 1) cm.";
    if (form.pump_start_level < 0 || form.pump_start_level > 100) return "Pump start: 0–100%.";
    if (form.pump_stop_level < 0 || form.pump_stop_level > 100) return "Pump stop: 0–100%.";
    if (form.dry_run_threshold_lpm < 0.1 || form.dry_run_threshold_lpm > 10) return "Dry-run threshold: 0.1–10 LPM.";
    if (form.dry_run_timeout_sec < 10 || form.dry_run_timeout_sec > 300) return "Dry-run timeout: 10–300 sec.";
    if (form.flow_calibration_factor < 0.1 || form.flow_calibration_factor > 20) return "Flow factor: 0.1–20.";
    if (form.max_pump_runtime_min < 30 || form.max_pump_runtime_min > 480) return "Max runtime: 30–480 minutes.";
    if (form.sleep_start_hour < 0 || form.sleep_start_hour > 23) return "Sleep start: 0–23.";
    if (form.sleep_end_hour < 0 || form.sleep_end_hour > 23) return "Sleep end: 0–23.";
    if (form.sleep_emergency_level < 0 || form.sleep_emergency_level > 100) return "Emergency level: 0–100%.";
    if (form.sensor_failure_threshold < 3 || form.sensor_failure_threshold > 20) return "Sensor failure threshold: 3–20.";
    if (form.idle_sensor_interval_ms < 5000 || form.idle_sensor_interval_ms > 60000) return "Idle sensor interval: 5000–60000 ms.";
    if (form.idle_firebase_interval_ms < 10000 || form.idle_firebase_interval_ms > 120000) return "Idle Firebase interval: 10000–120000 ms.";
    return null;
  }

  async function handleSave() {
    setSaveError(null);
    const err = validate();
    if (err) {
      setSaveError(err);
      return;
    }
    setSaving(true);
    try {
      await saveConfig(form);
      setSaveSuccess(true);
      setTimeout(() => onClose(), 800);
    } catch (e: unknown) {
      let msg = "Failed to save. Check your connection.";
      if (e && typeof e === "object" && "code" in e) {
        const code = (e as { code?: string }).code;
        if (code === "PERMISSION_DENIED") msg = "Only authorized dashboard users can change device config. Check database rules.";
        else if (code === "UNAVAILABLE") msg = "Database unavailable.";
      } else if (e instanceof Error) msg = e.message;
      setSaveError(msg);
    } finally {
      setSaving(false);
    }
  }

  async function handleSeedDefaults() {
    setSaveError(null);
    setSeeding(true);
    try {
      await seedDefaultsIfEmpty();
      if (config) setForm(config);
      else setForm({ ...DEFAULT_DEVICE_CONFIG });
    } catch (e: unknown) {
      setSaveError(e instanceof Error ? e.message : "Failed to seed defaults.");
    } finally {
      setSeeding(false);
    }
  }

  if (loading) {
    return (
      <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50">
        <div className="card p-8 max-w-md w-full mx-4">
          <div className="animate-pulse text-text-muted text-sm font-mono">Loading…</div>
        </div>
      </div>
    );
  }

  return (
    <div className="fixed inset-0 z-50 flex items-end sm:items-center justify-center p-0 sm:p-4 bg-black/50 overscroll-contain pt-[env(safe-area-inset-top)] pb-[env(safe-area-inset-bottom)] pl-[max(0.5rem,env(safe-area-inset-left))] pr-[max(0.5rem,env(safe-area-inset-right))]">
      <div className="card card-glow-cyan max-w-md w-full max-h-[95dvh] sm:max-h-[90vh] min-w-0 rounded-t-2xl sm:rounded-2xl flex flex-col overflow-hidden">
        {/* Header - fixed */}
        <div className="flex items-center justify-between p-4 sm:p-6 pb-0 shrink-0">
          <div className="flex items-center gap-3">
            <div className="p-2 rounded-lg bg-accent-cyan/10">
              <Settings size={20} className="text-accent-cyan" />
            </div>
            <div>
              <h2 className="font-display font-semibold text-text-primary">Device config</h2>
              <p className="text-xs font-mono text-text-muted">Calibration & thresholds (ESP32 reads from DB)</p>
            </div>
          </div>
          <button onClick={onClose} className="p-2 rounded-lg text-text-muted hover:text-text-primary hover:bg-surface-3 transition-colors" aria-label="Close">
            <X size={20} />
          </button>
        </div>

        {/* Scrollable content */}
        <div className="flex-1 overflow-y-auto min-h-0 px-4 sm:px-6 py-4">
          <div className="space-y-4">
          {/* Tank Calibration */}
          <div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Tank Calibration</p>
              <InfoTooltip content="Measure your tank with a tape measure. Tank empty = distance from sensor to water surface when tank is empty. Tank full = distance when completely full. The ESP32 uses these to calculate water level %." />
            </div>
            <div className="grid grid-cols-2 gap-2 sm:gap-3">
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Tank empty (cm)
                  <InfoTooltip content="Distance from ultrasonic sensor to water surface when the tank is empty. Higher number = taller tank." side="right" />
                </label>
                <input type="number" min={5} max={200} value={form.tank_empty_cm} onChange={(e) => setForm((f) => ({ ...f, tank_empty_cm: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Tank full (cm)
                  <InfoTooltip content="Distance from sensor to water surface when the tank is completely full. Must be less than tank empty." side="right" />
                </label>
                <input type="number" min={1} max={199} value={form.tank_full_cm} onChange={(e) => setForm((f) => ({ ...f, tank_full_cm: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
            </div>
          </div>
          {/* Pump Thresholds */}
          <div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Pump Thresholds</p>
              <InfoTooltip content="In AUTO mode, the pump starts when tank level drops to Start % and stops when it reaches Stop %. Example: Start 30%, Stop 100% = pump fills tank to full." />
            </div>
            <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 sm:gap-3">
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Start (%)
                  <InfoTooltip content="Pump turns ON when tank level ≤ this %. Lower = wait longer before refilling." side="right" />
                </label>
                <input type="number" min={0} max={100} value={form.pump_start_level} onChange={(e) => setForm((f) => ({ ...f, pump_start_level: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Stop (%)
                  <InfoTooltip content="Pump turns OFF when tank level ≥ this %. Usually 100% for full tank." side="right" />
                </label>
                <input type="number" min={0} max={100} value={form.pump_stop_level} onChange={(e) => setForm((f) => ({ ...f, pump_stop_level: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Max runtime (min)
                  <InfoTooltip content="Safety cutoff: pump stops if running longer than this without reaching Stop %. Prevents overflow from stuck sensor or fill valve." side="right" />
                </label>
                <input type="number" min={30} max={480} value={form.max_pump_runtime_min} onChange={(e) => setForm((f) => ({ ...f, max_pump_runtime_min: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className="text-[9px] font-mono text-text-muted mt-0.5">Overflow protection</p>
              </div>
            </div>
          </div>
          {/* Safety */}
          <div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Safety</p>
              <InfoTooltip content="These settings protect the pump motor from running dry (no water flow) and calibrate flow sensor accuracy." />
            </div>
            <div className="grid grid-cols-2 gap-2 sm:gap-3">
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Dry-run threshold (LPM)
                  <InfoTooltip content="If flow stays below this while pump is ON for the timeout period, pump shuts down. Protects motor from running dry." side="right" />
                </label>
                <input type="number" step={0.1} min={0.1} max={10} value={form.dry_run_threshold_lpm} onChange={(e) => setForm((f) => ({ ...f, dry_run_threshold_lpm: parseFloat(e.target.value) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Dry-run timeout (sec)
                  <InfoTooltip content="How long low flow must persist before shutdown. 30s typical. Shorter = faster protection, may false-trigger on slow fills." side="right" />
                </label>
                <input type="number" min={10} max={300} value={form.dry_run_timeout_sec} onChange={(e) => setForm((f) => ({ ...f, dry_run_timeout_sec: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Flow calibration factor
                  <InfoTooltip content="Sensor-specific. YF-G1 1-inch: ~7.5, ½-inch: ~4.8. Verify with bucket test: fill known volume, compare to displayed flow." side="right" />
                </label>
                <input type="number" step={0.1} min={0.1} max={20} value={form.flow_calibration_factor} onChange={(e) => setForm((f) => ({ ...f, flow_calibration_factor: parseFloat(e.target.value) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className="text-[9px] font-mono text-text-muted mt-0.5">YF-G1: 7.5 or 4.8. Verify with bucket test.</p>
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Sensor failure threshold
                  <InfoTooltip content="After this many consecutive ultrasonic read failures, system reports sensor error. 5 is default." side="right" />
                </label>
                <input type="number" min={3} max={20} value={form.sensor_failure_threshold} onChange={(e) => setForm((f) => ({ ...f, sensor_failure_threshold: parseInt(e.target.value, 10) || 5 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className="text-[9px] font-mono text-text-muted mt-0.5">Consecutive ultrasonic timeouts before error</p>
              </div>
            </div>
          </div>
          {/* Sleep Schedule */}
          <div className="border-t border-surface-3 pt-4">
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Sleep Schedule</p>
              <InfoTooltip content="Quiet hours: AUTO mode is suppressed. FORCE_ON and emergency override still work. Good for avoiding pump noise at night." />
            </div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <input type="checkbox" id="sleep_enabled" checked={form.sleep_enabled}
                onChange={(e) => setForm((f) => ({ ...f, sleep_enabled: e.target.checked }))}
                className="w-4 h-4 rounded border-surface-4 text-accent-cyan focus:ring-accent-cyan/50" />
              <label htmlFor="sleep_enabled" className="text-sm font-mono text-text-secondary">Enable sleep mode</label>
            </div>
            <p className="text-[9px] font-mono text-text-muted mb-2">During sleep: AUTO suppressed, {Math.round((form.idle_sensor_interval_ms ?? 10000) / 1000)}s poll. FORCE_ON and emergency override still work.</p>
            <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 sm:gap-3">
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Start (hour)
                  <InfoTooltip content="Sleep mode begins at this hour (0–23). Example: 23 = 11 PM." side="right" />
                </label>
                <select value={form.sleep_start_hour} onChange={(e) => setForm((f) => ({ ...f, sleep_start_hour: parseInt(e.target.value, 10) }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm">
                  {Array.from({ length: 24 }, (_, i) => (
                    <option key={i} value={i}>{i === 0 ? "12 AM" : i < 12 ? `${i} AM` : i === 12 ? "12 PM" : `${i - 12} PM`}</option>
                  ))}
                </select>
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  End (hour)
                  <InfoTooltip content="Sleep mode ends at this hour. Example: 5 = 5 AM." side="right" />
                </label>
                <select value={form.sleep_end_hour} onChange={(e) => setForm((f) => ({ ...f, sleep_end_hour: parseInt(e.target.value, 10) }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm">
                  {Array.from({ length: 24 }, (_, i) => (
                    <option key={i} value={i}>{i === 0 ? "12 AM" : i < 12 ? `${i} AM` : i === 12 ? "12 PM" : `${i - 12} PM`}</option>
                  ))}
                </select>
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Emergency %
                  <InfoTooltip content="If tank level ≤ this %, pump runs even during sleep. Prevents running out of water overnight." side="right" />
                </label>
                <input type="number" min={0} max={100} value={form.sleep_emergency_level}
                  onChange={(e) => setForm((f) => ({ ...f, sleep_emergency_level: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className="text-[9px] font-mono text-text-muted mt-0.5">Bypass sleep if level ≤ this %</p>
              </div>
            </div>
          </div>
          {/* Advanced */}
          <div className="border-t border-surface-3 pt-4">
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Advanced</p>
              <InfoTooltip content="When tank is full and pump has been off 5+ min, the ESP32 slows down sensor and Firebase updates to save power and reduce traffic." />
            </div>
            <p className="text-[9px] font-mono text-text-muted mb-2">Slow-poll intervals when tank ≥90% and pump OFF for 5 min.</p>
            <div className="grid grid-cols-2 gap-2 sm:gap-3">
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Idle sensor interval (ms)
                  <InfoTooltip content="How often to read ultrasonic sensor when idle. 10000 = 10 seconds. Higher = less power, slower response." side="right" />
                </label>
                <input type="number" min={5000} max={60000} step={1000} value={form.idle_sensor_interval_ms}
                  onChange={(e) => setForm((f) => ({ ...f, idle_sensor_interval_ms: parseInt(e.target.value, 10) || 10000 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                  Idle Firebase interval (ms)
                  <InfoTooltip content="How often to push status to cloud when idle. 30000 = 30 seconds. Saves bandwidth and Firebase reads." side="right" />
                </label>
                <input type="number" min={10000} max={120000} step={1000} value={form.idle_firebase_interval_ms}
                  onChange={(e) => setForm((f) => ({ ...f, idle_firebase_interval_ms: parseInt(e.target.value, 10) || 30000 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
            </div>
          </div>
          <p className="text-[10px] font-mono text-text-muted">ESP32 reads this every 30s when online. Offline: last-saved NVS values. No reflash needed.</p>
          </div>

          {saveError && (
            <p className="mt-4 p-3 rounded-lg bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs font-mono">{saveError}</p>
          )}
        </div>

        {/* Footer - fixed, no overlap */}
        <div className="shrink-0 flex flex-col gap-2 p-4 sm:p-6 pt-4 border-t border-surface-3 bg-surface-1">
          <button onClick={handleSeedDefaults} disabled={seeding} className="w-full px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm hover:bg-surface-3 disabled:opacity-50">
            {seeding ? "Seeding…" : "Seed defaults (if empty)"}
          </button>
          <div className="flex gap-3">
            <button onClick={onClose} disabled={saving} className="flex-1 px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm hover:bg-surface-3 disabled:opacity-50">
              Cancel
            </button>
            <button onClick={handleSave} disabled={saving} className="flex-1 px-4 py-2.5 rounded-xl bg-accent-cyan/20 border border-accent-cyan/40 text-accent-cyan font-mono text-sm font-semibold hover:bg-accent-cyan/30 disabled:opacity-50">
              {saving ? "Saving…" : saveSuccess ? "Saved" : "Save"}
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
