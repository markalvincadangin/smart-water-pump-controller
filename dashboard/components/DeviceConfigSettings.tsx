// components/DeviceConfigSettings.tsx
"use client";

import { useState, useEffect } from "react";
import { Settings, X } from "lucide-react";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
import InfoTooltip from "./InfoTooltip";
import type { DeviceConfig } from "@/lib/types";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import { toast } from "@/lib/toast";
import { writeAuditEvent } from "@/lib/audit";

interface DeviceConfigSettingsProps {
  onClose: () => void;
  isAdmin?: boolean;
  actorUid?: string | null;
  actorEmail?: string | null;
}

export default function DeviceConfigSettings({ onClose, isAdmin = false, actorUid = null, actorEmail = null }: DeviceConfigSettingsProps) {
  const { config, loading, saveConfig, seedDefaultsIfEmpty } = useDeviceConfig();
  const [form, setForm] = useState<DeviceConfig>({ ...DEFAULT_DEVICE_CONFIG });
  const [saving, setSaving] = useState(false);
  const [seeding, setSeeding] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [saveSuccess, setSaveSuccess] = useState(false);
  const [showAdvanced, setShowAdvanced] = useState(false);

  const sectionTitleClass =
    "text-sm font-display font-semibold text-text-primary";
  const sectionSubtitleClass =
    "text-[11px] font-mono text-text-muted mt-0.5";
  const fieldLabelClass =
    "flex flex-wrap items-center gap-x-2 gap-y-1 text-[11px] font-mono text-text-secondary mb-1";
  const helperTextClass =
    "text-[10px] font-mono text-text-muted mt-1";

  useEffect(() => {
    if (config) setForm(config);
  }, [config]);

  function validate(): string | null {
    if (form.tank_full_cm >= form.tank_empty_cm) return "Full (cm) must be less than Empty (cm).";
    if (form.pump_start_level >= form.pump_stop_level) return "Start % must be less than Stop %.";
    if (form.tank_empty_cm < 5 || form.tank_empty_cm > 200) return "Empty: enter 5–200 cm.";
    if (form.tank_full_cm < 1 || form.tank_full_cm >= form.tank_empty_cm) return "Full: enter 1 to (Empty − 1) cm.";
    if (form.pump_start_level < 0 || form.pump_start_level > 100) return "Pump start: 0–100%.";
    if (form.pump_stop_level < 0 || form.pump_stop_level > 100) return "Pump stop: 0–100%.";
    if (form.dry_run_threshold_lpm < 0.1 || form.dry_run_threshold_lpm > 10) return "No-flow threshold: 0.1–10 L/min.";
    if (form.dry_run_timeout_sec < 10 || form.dry_run_timeout_sec > 300) return "Shutdown delay: 10–300 sec.";
    if (form.flow_calibration_factor < 0.1 || form.flow_calibration_factor > 20) return "Flow factor: 0.1–20.";
    if (form.max_pump_runtime_min < 30 || form.max_pump_runtime_min > 480) return "Max runtime: 30–480 minutes.";
    if (form.sleep_start_hour < 0 || form.sleep_start_hour > 23) return "Sleep start: 0–23.";
    if (form.sleep_end_hour < 0 || form.sleep_end_hour > 23) return "Sleep end: 0–23.";
    if (form.sleep_emergency_level < 0 || form.sleep_emergency_level > 100) return "Emergency level: 0–100%.";
    if (form.sensor_failure_threshold < 3 || form.sensor_failure_threshold > 20) return "Sensor error threshold: 3–20.";
    if (form.idle_sensor_interval_ms < 5000 || form.idle_sensor_interval_ms > 60000) return "Level check: 5000–60000 ms.";
    if (form.idle_firebase_interval_ms < 10000 || form.idle_firebase_interval_ms > 120000) return "Sync interval: 10000–120000 ms.";
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
      toast({ kind: "success", title: "Device settings saved", detail: "Changes will sync when the controller is online." });
      if (actorUid) {
        await writeAuditEvent({
          action: "config.device.save",
          uid: actorUid,
          email: actorEmail,
        });
      }
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
              <h2 className="font-display font-semibold text-text-primary">Device settings</h2>
              <p className="text-xs font-mono text-text-muted">Tank size, pump levels, and safety</p>
            </div>
          </div>
          <button onClick={onClose} className="p-2 rounded-lg text-text-muted hover:text-text-primary hover:bg-surface-3 transition-colors" aria-label="Close">
            <X size={20} />
          </button>
        </div>

        {/* Scrollable content */}
        <div className="flex-1 overflow-y-auto min-h-0 px-4 sm:px-6 py-4">
          <div className="flex items-center justify-between gap-2 mb-4">
            <div className="inline-flex rounded-xl border border-surface-4 bg-surface-2 p-1">
              <button
                type="button"
                onClick={() => setShowAdvanced(false)}
                className={`px-3 py-2 rounded-lg text-xs font-mono transition-colors ${!showAdvanced ? "bg-surface-3 text-text-primary" : "text-text-muted hover:text-text-primary"}`}
              >
                Basic
              </button>
              <button
                type="button"
                onClick={() => isAdmin && setShowAdvanced(true)}
                disabled={!isAdmin}
                title={isAdmin ? "Advanced settings" : "Admin only"}
                className={`px-3 py-2 rounded-lg text-xs font-mono transition-colors ${showAdvanced ? "bg-surface-3 text-text-primary" : "text-text-muted hover:text-text-primary"} disabled:opacity-50 disabled:cursor-not-allowed`}
              >
                Advanced
              </button>
            </div>
            {!isAdmin && (
              <span className="text-[10px] font-mono text-text-muted">
                Advanced settings are admin-only
              </span>
            )}
          </div>

          <div className="space-y-4">
          {/* Tank Calibration */}
          <div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <div>
                <p className={sectionTitleClass}>Tank size</p>
                <p className={sectionSubtitleClass}>Calibrate empty/full distances for accurate level.</p>
              </div>
              <InfoTooltip content="Measure with a tape measure. Enter the distance from the sensor to the water when empty and when full. This sets how we show the water level." />
            </div>
            <div className="grid grid-cols-2 gap-2 sm:gap-3">
              <div>
                <label className={fieldLabelClass}>
                  Empty (cm)
                  <InfoTooltip content="Distance from sensor to water when the tank is empty. Taller tanks use a larger number." side="right" />
                </label>
                <input type="number" min={5} max={200} value={form.tank_empty_cm} onChange={(e) => setForm((f) => ({ ...f, tank_empty_cm: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className={helperTextClass}>Typical: 50–150 cm (depends on tank height).</p>
              </div>
              <div>
                <label className={fieldLabelClass}>
                  Full (cm)
                  <InfoTooltip content="Distance from sensor to water when the tank is completely full. Must be less than empty." side="right" />
                </label>
                <input type="number" min={1} max={199} value={form.tank_full_cm} onChange={(e) => setForm((f) => ({ ...f, tank_full_cm: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className={helperTextClass}>Must be less than Empty.</p>
              </div>
            </div>
          </div>
          {/* Pump Thresholds */}
          <div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <div>
                <p className={sectionTitleClass}>When to run the pump</p>
                <p className={sectionSubtitleClass}>AUTO mode turns ON at Start and OFF at Stop.</p>
              </div>
              <InfoTooltip content="In automatic mode, the pump turns on when water drops to Start % and off when it reaches Stop %. Example: Start 30%, Stop 100% fills the tank completely." />
            </div>
            <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 sm:gap-3">
              <div>
                <label className={fieldLabelClass}>
                  Start at (%)
                  <InfoTooltip content="Pump turns on when water reaches this level or below. Lower means waiting longer before refilling." side="right" />
                </label>
                <input type="number" min={0} max={100} value={form.pump_start_level} onChange={(e) => setForm((f) => ({ ...f, pump_start_level: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className={fieldLabelClass}>
                  Stop at (%)
                  <InfoTooltip content="Pump turns off when water reaches this level. Usually 100% for a full tank." side="right" />
                </label>
                <input type="number" min={0} max={100} value={form.pump_stop_level} onChange={(e) => setForm((f) => ({ ...f, pump_stop_level: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className={fieldLabelClass}>
                  Max runtime (min)
                  <InfoTooltip content="Safety limit: pump stops if it runs longer than this without filling. Helps prevent overflow if something goes wrong." side="right" />
                </label>
                <input type="number" min={30} max={480} value={form.max_pump_runtime_min} onChange={(e) => setForm((f) => ({ ...f, max_pump_runtime_min: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                <p className={helperTextClass}>Overflow protection.</p>
              </div>
            </div>
          </div>
          {/* Basic safety */}
          <div>
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
              <div>
                <p className={sectionTitleClass}>Pump protection</p>
                <p className={sectionSubtitleClass}>Dry-run protection based on low flow.</p>
              </div>
              <InfoTooltip content="Settings to protect the pump from running without water." />
            </div>
            <div className="grid grid-cols-2 gap-2 sm:gap-3">
              <div>
                <label className={fieldLabelClass}>
                  No-flow threshold (L/min)
                  <InfoTooltip content="If water flow stays below this while the pump is on, it will shut off. Protects the pump from running dry." side="right" />
                </label>
                <input type="number" step={0.1} min={0.1} max={10} value={form.dry_run_threshold_lpm} onChange={(e) => setForm((f) => ({ ...f, dry_run_threshold_lpm: parseFloat(e.target.value) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
              <div>
                <label className={fieldLabelClass}>
                  Shutdown delay (sec)
                  <InfoTooltip content="How long low flow must continue before the pump shuts off. 30 seconds is typical. Shorter = faster protection." side="right" />
                </label>
                <input type="number" min={10} max={300} value={form.dry_run_timeout_sec} onChange={(e) => setForm((f) => ({ ...f, dry_run_timeout_sec: parseInt(e.target.value, 10) || 0 }))}
                  className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
              </div>
            </div>
          </div>

          {/* Advanced-only sections */}
          {showAdvanced && isAdmin && (
            <>
              <div className="border-t border-surface-3 pt-4">
                <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
                  <div>
                    <p className={sectionTitleClass}>Calibration & diagnostics</p>
                    <p className={sectionSubtitleClass}>Admin-only. Adjust only if you know what you’re doing.</p>
                  </div>
                  <InfoTooltip content="Advanced calibration values. Change only if you know what you’re doing." />
                </div>
                <div className="grid grid-cols-2 gap-2 sm:gap-3">
                  <div>
                    <label className={fieldLabelClass}>
                      Flow meter adjustment
                      <InfoTooltip content="Matches the flow meter to your setup. Check the sensor manual for the value (often 7.5 or 4.8). Test with a bucket to verify." side="right" />
                    </label>
                    <input type="number" step={0.1} min={0.1} max={20} value={form.flow_calibration_factor} onChange={(e) => setForm((f) => ({ ...f, flow_calibration_factor: parseFloat(e.target.value) || 0 }))}
                      className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                    <p className={helperTextClass}>Check sensor manual; verify with bucket test.</p>
                  </div>
                  <div>
                    <label className={fieldLabelClass}>
                      Sensor error threshold
                      <InfoTooltip content="After this many failed readings in a row, we show a sensor error. Default is 5." side="right" />
                    </label>
                    <input type="number" min={3} max={20} value={form.sensor_failure_threshold} onChange={(e) => setForm((f) => ({ ...f, sensor_failure_threshold: parseInt(e.target.value, 10) || 5 }))}
                      className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                    <p className={helperTextClass}>Consecutive failed readings before error.</p>
                  </div>
                </div>
              </div>

              <div className="border-t border-surface-3 pt-4">
                <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
                  <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Quiet hours</p>
                  <InfoTooltip content="Pause automatic pumping during set hours. Manual control and emergency fill still work. Useful to avoid pump noise at night." />
                </div>
                <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
                  <input type="checkbox" id="sleep_enabled" checked={form.sleep_enabled}
                    onChange={(e) => setForm((f) => ({ ...f, sleep_enabled: e.target.checked }))}
                    className="w-4 h-4 rounded border-surface-4 text-accent-cyan focus:ring-accent-cyan/50" />
                  <label htmlFor="sleep_enabled" className="text-sm font-mono text-text-secondary">Enable quiet hours</label>
                </div>
                <p className="text-[9px] font-mono text-text-muted mb-2">During quiet hours: auto mode is paused. Manual control and low-water override still work.</p>
                <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 sm:gap-3">
                  <div>
                    <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                      Start (hour)
                      <InfoTooltip content="When quiet hours begin. Example: 23 = 11 PM." side="right" />
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
                      <InfoTooltip content="When quiet hours end. Example: 5 = 5 AM." side="right" />
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
                      Low-water override (%)
                      <InfoTooltip content="If water drops to this level during quiet hours, the pump runs anyway. Prevents running out overnight." side="right" />
                    </label>
                    <input type="number" min={0} max={100} value={form.sleep_emergency_level}
                      onChange={(e) => setForm((f) => ({ ...f, sleep_emergency_level: parseInt(e.target.value, 10) || 0 }))}
                      className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                    <p className="text-[9px] font-mono text-text-muted mt-0.5">Pump runs regardless of quiet hours when this low</p>
                  </div>
                </div>
              </div>

              <div className="border-t border-surface-3 pt-4">
                <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
                  <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Power saving</p>
                  <InfoTooltip content="When the tank is full and the pump has been off for 5+ minutes, we check the water level less often to save power." />
                </div>
                <p className="text-[9px] font-mono text-text-muted mb-2">Used when tank is 90%+ full and pump is off for 5+ min.</p>
                <div className="grid grid-cols-2 gap-2 sm:gap-3">
                  <div>
                    <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                      Level check (ms)
                      <InfoTooltip content="How often we measure the water level when idle. 10000 = 10 seconds. Higher = less power, slower updates." side="right" />
                    </label>
                    <input type="number" min={5000} max={60000} step={1000} value={form.idle_sensor_interval_ms}
                      onChange={(e) => setForm((f) => ({ ...f, idle_sensor_interval_ms: parseInt(e.target.value, 10) || 10000 }))}
                      className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                  </div>
                  <div>
                    <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1">
                      Sync interval (ms)
                      <InfoTooltip content="How often we update the dashboard when idle. 30000 = 30 seconds. Higher = less data usage." side="right" />
                    </label>
                    <input type="number" min={10000} max={120000} step={1000} value={form.idle_firebase_interval_ms}
                      onChange={(e) => setForm((f) => ({ ...f, idle_firebase_interval_ms: parseInt(e.target.value, 10) || 30000 }))}
                      className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
                  </div>
                </div>
              </div>
            </>
          )}
          <p className="text-[10px] font-mono text-text-muted">Settings sync to your controller when it&apos;s online. When offline, it uses the last saved values—no restart needed.</p>
          </div>

          {saveError && (
            <p className="mt-4 p-3 rounded-lg bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs font-mono">{saveError}</p>
          )}
        </div>

        {/* Footer - fixed, no overlap */}
        <div className="shrink-0 flex flex-col gap-2 p-4 sm:p-6 pt-4 border-t border-surface-3 bg-surface-1">
          <button onClick={handleSeedDefaults} disabled={seeding} className="w-full px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm hover:bg-surface-3 disabled:opacity-50">
            {seeding ? "Setting up…" : "Reset to defaults (if empty)"}
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
