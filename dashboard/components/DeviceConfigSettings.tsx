// components/DeviceConfigSettings.tsx
"use client";

import { useState, useEffect } from "react";
import { Settings, X } from "lucide-react";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
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
    if (form.flow_calibration_factor < 0.1 || form.flow_calibration_factor > 10) return "Flow factor: 0.1–10.";
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
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50 overscroll-contain pt-[env(safe-area-inset-top)] pb-[env(safe-area-inset-bottom)] pl-[max(1rem,env(safe-area-inset-left))] pr-[max(1rem,env(safe-area-inset-right))]">
      <div className="card card-glow-cyan p-6 max-w-md w-full max-h-[90vh] overflow-y-auto min-w-0">
        <div className="flex items-center justify-between mb-6">
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

        <div className="space-y-4">
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Tank empty (cm)</label>
              <input type="number" min={5} max={200} value={form.tank_empty_cm} onChange={(e) => setForm((f) => ({ ...f, tank_empty_cm: parseInt(e.target.value, 10) || 0 }))}
                className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
            </div>
            <div>
              <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Tank full (cm)</label>
              <input type="number" min={1} max={199} value={form.tank_full_cm} onChange={(e) => setForm((f) => ({ ...f, tank_full_cm: parseInt(e.target.value, 10) || 0 }))}
                className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
            </div>
          </div>
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Pump start (%)</label>
              <input type="number" min={0} max={100} value={form.pump_start_level} onChange={(e) => setForm((f) => ({ ...f, pump_start_level: parseInt(e.target.value, 10) || 0 }))}
                className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
            </div>
            <div>
              <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Pump stop (%)</label>
              <input type="number" min={0} max={100} value={form.pump_stop_level} onChange={(e) => setForm((f) => ({ ...f, pump_stop_level: parseInt(e.target.value, 10) || 0 }))}
                className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
            </div>
          </div>
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Dry-run threshold (LPM)</label>
              <input type="number" step={0.1} min={0.1} max={10} value={form.dry_run_threshold_lpm} onChange={(e) => setForm((f) => ({ ...f, dry_run_threshold_lpm: parseFloat(e.target.value) || 0 }))}
                className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
            </div>
            <div>
              <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Dry-run timeout (sec)</label>
              <input type="number" min={10} max={300} value={form.dry_run_timeout_sec} onChange={(e) => setForm((f) => ({ ...f, dry_run_timeout_sec: parseInt(e.target.value, 10) || 0 }))}
                className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
            </div>
          </div>
          <div>
            <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1">Flow calibration factor</label>
            <input type="number" step={0.1} min={0.1} max={10} value={form.flow_calibration_factor} onChange={(e) => setForm((f) => ({ ...f, flow_calibration_factor: parseFloat(e.target.value) || 0 }))}
              className="w-full px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm" />
          </div>
          <p className="text-[10px] font-mono text-text-muted">ESP32 reads this every 3s when online. When offline it uses last-saved values from NVS. No reflash needed.</p>
        </div>

        {saveError && (
          <p className="mt-4 p-3 rounded-lg bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs font-mono">{saveError}</p>
        )}

        <div className="flex flex-col gap-2 mt-6">
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
