// components/DeviceConfigSettings.tsx
"use client";

import { useState, useEffect, useMemo } from "react";
import { Settings, X, RotateCw, AlertTriangle, ShieldAlert, Cpu, Droplets, Info } from "lucide-react";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
import { validateDeviceConfig, validateDeviceConfigFields, type FieldErrors } from "@/lib/validation";
import {
   DEVICE_CONFIG_ULTRASONIC_MAX_CM,
   DEVICE_CONFIG_ULTRASONIC_MIN_CM,
} from "@/lib/constants";
import InfoTooltip from "./InfoTooltip";
import type { DeviceConfig } from "@/lib/types";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import { toast } from "@/lib/toast";
import { writeAuditEvent } from "@/lib/audit";
import clsx from "clsx";

interface DeviceConfigSettingsProps {
  onClose: () => void;
  isAdmin?: boolean;
  actorUid?: string | null;
  actorEmail?: string | null;
  esp32Online?: boolean;
  onRequestReboot?: () => Promise<void>;
  /** Current bypass state from controller status (maintenance mode). */
  bypassLevelSensor?: boolean;
  /** Current flow bypass state from controller status (maintenance mode). */
  bypassFlowSensor?: boolean;
  /** Set level sensor bypass via Firebase control (admin only). */
  onSetBypassLevelSensor?: (value: boolean) => Promise<void>;
  /** Set flow sensor bypass via Firebase control (admin only). */
  onSetBypassFlowSensor?: (value: boolean) => Promise<void>;
}

/**
 * REFACTOR [D5.1]: DeviceConfigSettings
 * Modernized with SF design tokens, high-fidelity inputs, and clear safety semantics.
 */
export default function DeviceConfigSettings({ 
  onClose, 
  isAdmin = false, 
  actorUid = null, 
  actorEmail = null, 
  esp32Online = false, 
  onRequestReboot, 
  bypassLevelSensor = false, 
  bypassFlowSensor = false, 
  onSetBypassLevelSensor, 
  onSetBypassFlowSensor 
}: DeviceConfigSettingsProps) {
  const { config, loading, saveConfig, seedDefaultsIfEmpty } = useDeviceConfig();
  const [form, setForm] = useState<DeviceConfig>({ ...DEFAULT_DEVICE_CONFIG });
  const [saving, setSaving] = useState(false);
  const [seeding, setSeeding] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [fieldErrors, setFieldErrors] = useState<FieldErrors>({});
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [rebootBusy, setRebootBusy] = useState(false);
  const [bypassBusy, setBypassBusy] = useState(false);
  // Optimistic local state — shows new value immediately, syncs from hardware status
  const [bypassLevelLocal, setBypassLevelLocal] = useState(bypassLevelSensor);
  const [bypassFlowLocal, setBypassFlowLocal] = useState(bypassFlowSensor);

  // SF Styles
  const sectionTitleClass = "text-sm font-display font-semibold text-text-primary mb-3 flex items-center gap-2";
  const fieldGroupClass = "space-y-4 p-4 rounded-xl bg-surface-2 border border-surface-3";
  const fieldLabelClass = "flex flex-wrap items-center gap-x-2 gap-y-1 text-[11px] font-mono text-text-secondary mb-1.5";
  const inputClass = "w-full px-4 py-2.5 rounded-xl bg-surface-1 border border-surface-4 text-text-primary font-mono text-sm focus:outline-none focus:ring-2 focus:ring-accent-blue/50 focus:border-transparent transition-all disabled:opacity-50";
  const helperTextClass = "text-[10px] font-mono text-text-muted mt-1.5 leading-relaxed";

  useEffect(() => {
    if (config) setForm(config);
  }, [config]);

  useEffect(() => {
    setFieldErrors(validateDeviceConfigFields(form));
  }, [form]);

  // Sync bypass local state when hardware status changes (ESP32 round-trip)
  useEffect(() => { setBypassLevelLocal(bypassLevelSensor); }, [bypassLevelSensor]);
  useEffect(() => { setBypassFlowLocal(bypassFlowSensor); }, [bypassFlowSensor]);

  // B8: Dirty detection logic (including new FW fields)
  const isDirty = useMemo(() => {
    if (!config) return false;
    const a = form;
    const b = config;
    const levelThreshA = a.level_sensor_failure_threshold ?? a.sensor_failure_threshold;
    const levelThreshB = b.level_sensor_failure_threshold ?? b.sensor_failure_threshold;
    
    return (
      a.tank_empty_cm !== b.tank_empty_cm ||
      a.tank_full_cm !== b.tank_full_cm ||
      a.pump_start_level !== b.pump_start_level ||
      a.pump_stop_level !== b.pump_stop_level ||
      a.dry_run_threshold_lpm !== b.dry_run_threshold_lpm ||
      a.dry_run_timeout_sec !== b.dry_run_timeout_sec ||
      a.flow_calibration_factor !== b.flow_calibration_factor ||
      a.max_pump_runtime_min !== b.max_pump_runtime_min ||
      a.sleep_enabled !== b.sleep_enabled ||
      a.sleep_start_hour !== b.sleep_start_hour ||
      a.sleep_end_hour !== b.sleep_end_hour ||
      a.sleep_emergency_level !== b.sleep_emergency_level ||
      levelThreshA !== levelThreshB ||
      a.idle_sensor_interval_ms !== b.idle_sensor_interval_ms ||
      a.idle_firebase_interval_ms !== b.idle_firebase_interval_ms ||
      (a.auto_bypass_on_sensor_fail ?? false) !== (b.auto_bypass_on_sensor_fail ?? false) ||
      (a.auto_bypass_delay_sec ?? 60) !== (b.auto_bypass_delay_sec ?? 60)
    );
  }, [form, config]);

  const [showDiscardConfirm, setShowDiscardConfirm] = useState(false);

  function handleCloseAttempt() {
    if (isDirty) setShowDiscardConfirm(true);
    else onClose();
  }

  function handleDiscard() {
    if (config) setForm(config);
    setShowDiscardConfirm(false);
    setSaveError(null);
    onClose();
  }

  async function handleSave() {
    setSaveError(null);
    const nextFieldErrors = validateDeviceConfigFields(form);
    setFieldErrors(nextFieldErrors);
    const { isValid, error } = validateDeviceConfig(form);
    if (!isValid) {
      setSaveError(error);
      return;
    }
    setSaving(true);
    try {
      await saveConfig(form);
      toast({ kind: "success", title: "Settings saved", detail: "Firmware will apply within ~30 seconds." });
      if (actorUid) {
        await writeAuditEvent({
          action: "config.device.save",
          uid: actorUid,
          email: actorEmail,
          detail: "Config updated via dashboard UI",
        });
      }
      setTimeout(() => onClose(), 800);
      } catch (e: unknown) {
         setSaveError(e instanceof Error ? e.message : "Failed to save config.");
    } finally {
      setSaving(false);
    }
  }

  const errorText = (key: keyof FieldErrors) =>
    fieldErrors[key] ? (
      <p className="mt-1 text-[10px] font-mono text-accent-red">{fieldErrors[key]}</p>
    ) : null;

  async function handleSeedDefaults() {
    setSaveError(null);
    setSeeding(true);
    try {
      await seedDefaultsIfEmpty();
      if (config) setForm(config);
      } catch (e: unknown) {
         setSaveError(e instanceof Error ? e.message : "Failed to seed defaults.");
    } finally {
      setSeeding(false);
    }
  }

   async function handleRequestRebootClick() {
      if (!onRequestReboot || rebootBusy) return;
      setRebootBusy(true);
      try {
         await onRequestReboot();
      } finally {
         setRebootBusy(false);
      }
   }

   async function handleSetBypassLevelSensor(checked: boolean) {
      if (!onSetBypassLevelSensor || bypassBusy) return;
      setBypassLevelLocal(checked); // Optimistic update
      setBypassBusy(true);
      try {
         await onSetBypassLevelSensor(checked);
         toast({ kind: "success", title: `Level sensor bypass ${checked ? "enabled" : "disabled"}` });
      } catch (e: unknown) {
         setBypassLevelLocal(!checked); // Revert on failure
         toast({ kind: "error", title: "Bypass change failed", detail: e instanceof Error ? e.message : "Permission denied" });
      } finally {
         setBypassBusy(false);
      }
   }

   async function handleSetBypassFlowSensor(checked: boolean) {
      if (!onSetBypassFlowSensor || bypassBusy) return;
      setBypassFlowLocal(checked); // Optimistic update
      setBypassBusy(true);
      try {
         await onSetBypassFlowSensor(checked);
         toast({ kind: "success", title: `Flow sensor bypass ${checked ? "enabled" : "disabled"}` });
      } catch (e: unknown) {
         setBypassFlowLocal(!checked); // Revert on failure
         toast({ kind: "error", title: "Bypass change failed", detail: e instanceof Error ? e.message : "Permission denied" });
      } finally {
         setBypassBusy(false);
      }
   }

  if (loading) {
    return (
      <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 backdrop-blur-sm animate-fade-in">
        <div className="w-10 h-10 border-2 border-accent-blue/30 border-t-accent-blue rounded-full animate-spin" />
      </div>
    );
  }

  return (
    <div 
      className="fixed inset-0 z-50 flex items-end sm:items-center justify-center p-0 sm:p-4 bg-black/50 overscroll-contain pt-[env(safe-area-inset-top)] pb-[env(safe-area-inset-bottom)] pl-[max(0.5rem,env(safe-area-inset-left))] pr-[max(0.5rem,env(safe-area-inset-right))]"
      onClick={(e) => e.target === e.currentTarget && handleCloseAttempt()}
      role="dialog"
      aria-modal="true"
      aria-labelledby="settings-title"
    >
      <div className="card max-w-md w-full max-h-[95dvh] sm:max-h-[90vh] min-w-0 rounded-t-2xl sm:rounded-2xl flex flex-col overflow-hidden shadow-2xl animate-slide-up">
        
        {/* Header (Fixed) */}
        <div className="flex items-center justify-between p-4 sm:p-6 pb-0 shrink-0">
          <div className="flex items-center gap-3">
             <div className="p-2 flex items-center justify-center rounded-lg bg-accent-blue/10 text-accent-blue">
                <Settings size={20} className={saving ? "animate-spin-slow" : ""} />
             </div>
             <div>
                <h2 id="settings-title" className="font-display font-semibold text-text-primary">Device Settings</h2>
                <div 
                  className={clsx(
                    "text-xs font-mono",
                    esp32Online ? "text-accent-green" : "text-accent-amber"
                  )}
                >
                  {esp32Online ? "ONLINE \u2014 Sync Active" : "OFFLINE \u2014 Sync Pending"}
                </div>
             </div>
          </div>
          <button onClick={handleCloseAttempt} className="p-2 rounded-lg text-text-muted hover:text-text-primary hover:bg-surface-3 transition-colors" aria-label="Close">
            <X size={20} />
          </button>
        </div>

        {/* Scrollable Form Content */}
        <div className="flex-1 overflow-y-auto px-6 py-6 space-y-8 scrollbar-hide">
          
          {/* Admin Mode Toggle */}
          {isAdmin && (
             <div className="flex items-center justify-between gap-2 mb-4">
               <div className="inline-flex rounded-xl border border-surface-4 bg-surface-2 p-1">
                  <button 
                     onClick={() => setShowAdvanced(false)}
                     className={clsx(
                       "px-3 py-2 rounded-lg text-xs font-mono transition-colors",
                       !showAdvanced ? "bg-surface-3 text-text-primary" : "text-text-muted hover:text-text-primary"
                     )}
                  >
                     Basic
                  </button>
                  <button 
                     onClick={() => setShowAdvanced(true)}
                     className={clsx(
                       "px-3 py-2 rounded-lg text-xs font-mono transition-colors",
                       showAdvanced ? "bg-surface-3 text-text-primary" : "text-text-muted hover:text-text-primary"
                     )}
                  >
                     Advanced
                  </button>
               </div>
             </div>
          )}

          {/* Maintenance Overrides — always visible to admins, independent of Basic/Advanced toggle */}
          {showAdvanced && (onSetBypassLevelSensor || onSetBypassFlowSensor) && (
             <section className="space-y-4">
                <div className={sectionTitleClass}>
                   <AlertTriangle size={14} /> Maintenance Overrides
                   <span className="ml-auto text-[10px] font-mono font-normal text-accent-amber bg-accent-amber/10 border border-accent-amber/20 px-2 py-0.5 rounded-full">Saves immediately</span>
                </div>
                <div className="space-y-3">
                   {onSetBypassLevelSensor && (
                      <div className="p-4 rounded-xl bg-accent-amber/10 border border-accent-amber/30">
                         <label className="flex items-center gap-3 cursor-pointer">
                            <input 
                               type="checkbox" 
                               checked={bypassLevelLocal} 
                               onChange={(e) => handleSetBypassLevelSensor(e.target.checked)}
                               disabled={bypassBusy}
                               className="w-4 h-4 rounded border-accent-amber/30 text-accent-amber focus:ring-accent-amber"
                            />
                            <span className="text-xs font-bold text-accent-amber">Bypass Level Sensor</span>
                         </label>
                         <p className={helperTextClass}>Enables pump operation regardless of tank level telemetry.</p>
                      </div>
                   )}
                   {onSetBypassFlowSensor && (
                      <div className="p-4 rounded-xl bg-accent-red/10 border border-accent-red/30">
                         <label className="flex items-center gap-3 cursor-pointer">
                            <input 
                               type="checkbox" 
                               checked={bypassFlowLocal} 
                               onChange={(e) => handleSetBypassFlowSensor(e.target.checked)}
                               disabled={bypassBusy}
                               className="w-4 h-4 rounded border-accent-red/30 text-accent-red focus:ring-accent-red"
                            />
                            <span className="text-xs font-bold text-accent-red">Bypass Flow Guard</span>
                         </label>
                         <p className={helperTextClass}>Disables dry-run protection. <span className="font-bold underline">EXTREME CAUTION REQUIRED.</span></p>
                      </div>
                   )}
                </div>
             </section>
          )}

          {/* SECTION: TANK CALIBRATION */}
          <section className="space-y-4">
             <div className={sectionTitleClass}><Droplets size={14} /> Tank Calibration</div>
             <div className={fieldGroupClass}>
                <div className="grid grid-cols-2 gap-4">
                   <div>
                      <label className={fieldLabelClass}>
                         Empty (cm)
                         <InfoTooltip content="Distance from sensor to water when tank is empty" />
                      </label>
                      <input 
                         type="number" 
                         min={DEVICE_CONFIG_ULTRASONIC_MIN_CM}
                         max={DEVICE_CONFIG_ULTRASONIC_MAX_CM}
                         value={form.tank_empty_cm} 
                         onChange={(e) => setForm(f => ({...f, tank_empty_cm: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                         aria-invalid={!!fieldErrors.tank_empty_cm}
                      />
                      {errorText("tank_empty_cm")}
                   </div>
                   <div>
                      <label className={fieldLabelClass}>
                         Full (cm)
                         <InfoTooltip content="Distance from sensor to water when tank is full" />
                      </label>
                      <input 
                         type="number" 
                         min={DEVICE_CONFIG_ULTRASONIC_MIN_CM}
                         max={DEVICE_CONFIG_ULTRASONIC_MAX_CM}
                         value={form.tank_full_cm} 
                         onChange={(e) => setForm(f => ({...f, tank_full_cm: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                         aria-invalid={!!fieldErrors.tank_full_cm}
                      />
                      {errorText("tank_full_cm")}
                   </div>
                </div>
                        <p className={helperTextClass}>
                           These values define 0% and 100% level and must stay within ultrasonic range ({DEVICE_CONFIG_ULTRASONIC_MIN_CM}-{DEVICE_CONFIG_ULTRASONIC_MAX_CM} cm).
                        </p>
             </div>
          </section>

          {/* SECTION: AUTOMATION LIMITS */}
          <section className="space-y-4">
             <div className={sectionTitleClass}><Cpu size={14} /> Pump Thresholds</div>
             <div className={fieldGroupClass}>
                <div className="grid grid-cols-2 gap-4">
                   <div>
                      <label className={fieldLabelClass}>Start AT (%)</label>
                      <input 
                         type="number" 
                         value={form.pump_start_level} 
                         onChange={(e) => setForm(f => ({...f, pump_start_level: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                         aria-invalid={!!fieldErrors.pump_start_level}
                      />
                      {errorText("pump_start_level")}
                   </div>
                   <div>
                      <label className={fieldLabelClass}>Stop AT (%)</label>
                      <input 
                         type="number" 
                         value={form.pump_stop_level} 
                         onChange={(e) => setForm(f => ({...f, pump_stop_level: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                         aria-invalid={!!fieldErrors.pump_stop_level}
                      />
                      {errorText("pump_stop_level")}
                   </div>
                </div>
                <div>
                   <label className={fieldLabelClass}>Max Runtime (MIN)</label>
                   <input 
                      type="number" 
                      value={form.max_pump_runtime_min} 
                      onChange={(e) => setForm(f => ({...f, max_pump_runtime_min: parseInt(e.target.value) || 0}))} 
                      className={inputClass}
                      aria-invalid={!!fieldErrors.max_pump_runtime_min}
                   />
                   {errorText("max_pump_runtime_min")}
                   <p className={helperTextClass}>Safety cutoff to prevent continuous pumping if dry-run check fails.</p>
                </div>
             </div>
          </section>

          {/* SECTION: ADVANCED (Admin Only) */}
          {showAdvanced && isAdmin && (
             <div className="space-y-8 pt-4 animate-fade-in">
                
                {/* Safety Monitoring */}
                <section className="space-y-4">
                   <div className={sectionTitleClass}><ShieldAlert size={14} /> Safety Guard</div>
                   <div className={fieldGroupClass}>
                      <div className="grid grid-cols-2 gap-4">
                         <div>
                            <label className={fieldLabelClass}>No-Flow (L/MIN)</label>
                            <input 
                               type="number" 
                               step="0.1"
                               value={form.dry_run_threshold_lpm} 
                               onChange={(e) => setForm(f => ({...f, dry_run_threshold_lpm: parseFloat(e.target.value) || 0}))} 
                               className={inputClass}
                               aria-invalid={!!fieldErrors.dry_run_threshold_lpm}
                            />
                            {errorText("dry_run_threshold_lpm")}
                         </div>
                         <div>
                            <label className={fieldLabelClass}>Cutoff (SEC)</label>
                            <input 
                               type="number" 
                               value={form.dry_run_timeout_sec} 
                               onChange={(e) => setForm(f => ({...f, dry_run_timeout_sec: parseInt(e.target.value) || 0}))} 
                               className={inputClass}
                               aria-invalid={!!fieldErrors.dry_run_timeout_sec}
                            />
                            {errorText("dry_run_timeout_sec")}
                         </div>
                      </div>
                      <div className="pt-2">
                        <label className="flex items-center gap-3 cursor-pointer group">
                           <input 
                              type="checkbox" 
                              checked={form.auto_bypass_on_sensor_fail ?? false} 
                              onChange={(e) => setForm(f => ({...f, auto_bypass_on_sensor_fail: e.target.checked}))}
                              className="w-4 h-4 rounded border-surface-4 text-accent-blue focus:ring-accent-blue"
                           />
                           <span className="text-xs font-bold text-text-primary group-hover:text-accent-blue transition-colors">
                             Auto-bypass on sensor fail
                           </span>
                        </label>
                        {form.auto_bypass_on_sensor_fail && (
                           <div className="mt-4 pl-7 animate-slide-up">
                              <label className={fieldLabelClass}>Bypass Delay (SEC)</label>
                              <input 
                                 type="number" 
                                 value={form.auto_bypass_delay_sec ?? 60} 
                                 onChange={(e) => setForm(f => ({...f, auto_bypass_delay_sec: parseInt(e.target.value) || 60}))} 
                                 className={inputClass}
                                 aria-invalid={!!fieldErrors.auto_bypass_delay_sec}
                              />
                              {errorText("auto_bypass_delay_sec")}
                           </div>
                        )}
                      </div>
                   </div>
                </section>

                {/* Quiet Hours */}
                <section className="space-y-4">
                   <div className={sectionTitleClass}><ShieldAlert size={14} /> Quiet Hours</div>
                   <div className={fieldGroupClass}>
                      <label className="flex items-center gap-3 cursor-pointer">
                         <input 
                            type="checkbox" 
                            checked={form.sleep_enabled} 
                            onChange={(e) => setForm(f => ({...f, sleep_enabled: e.target.checked}))}
                            className="w-4 h-4 rounded border-surface-4 text-accent-blue focus:ring-accent-blue"
                         />
                         <span className="text-xs font-bold text-text-primary font-mono">Enable Silent Period</span>
                      </label>
                      {form.sleep_enabled && (
                        <div className="grid grid-cols-2 gap-4 pt-2 animate-slide-up">
                           <div>
                              <label className={fieldLabelClass}>Start HOUR</label>
                              <select 
                                 value={form.sleep_start_hour}
                                 onChange={(e) => setForm(f => ({...f, sleep_start_hour: parseInt(e.target.value)}))}
                                 className={inputClass}
                              >
                                 {Array.from({length: 24}).map((_, i) => (
                                   <option key={i} value={i}>{i.toString().padStart(2, '0')}:00</option>
                                 ))}
                              </select>
                           </div>
                           <div>
                              <label className={fieldLabelClass}>End HOUR</label>
                              <select 
                                 value={form.sleep_end_hour}
                                 onChange={(e) => setForm(f => ({...f, sleep_end_hour: parseInt(e.target.value)}))}
                                 className={inputClass}
                              >
                                 {Array.from({length: 24}).map((_, i) => (
                                   <option key={i} value={i}>{i.toString().padStart(2, '0')}:00</option>
                                 ))}
                              </select>
                           </div>
                        </div>
                      )}
                   </div>
                </section>

                {/* System Diagnostics */}
                <section className="space-y-4">
                   <div className={sectionTitleClass}><Info size={14} /> Controller System</div>
                   <div className="space-y-4">
                      {onRequestReboot && (
                         <button 
                            onClick={handleRequestRebootClick}
                            disabled={rebootBusy || !esp32Online}
                            className="w-full flex items-center justify-center gap-2 py-3 border border-surface-4 rounded-xl text-text-secondary font-bold hover:bg-surface-3 disabled:opacity-50 transition-colors"
                         >
                            <RotateCw size={16} className={rebootBusy ? "animate-spin" : ""} />
                            {rebootBusy ? "Restarting..." : "Restart Device"}
                         </button>
                      )}
                      
                      <button 
                         onClick={handleSeedDefaults}
                         disabled={seeding}
                         className="w-full py-3 text-[10px] font-bold text-text-muted uppercase tracking-widest hover:text-accent-blue transition-colors"
                      >
                         {seeding ? "Syncing..." : "Seed Default Constants"}
                      </button>
                   </div>
                </section>
             </div>
          )}

          {saveError && (
             <div className="p-4 rounded-xl bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs font-mono animate-fade-in">
                {saveError}
             </div>
          )}
        </div>

        {/* Action Footer (Sticky) */}
        <div className="shrink-0 flex gap-3 p-4 sm:p-6 pt-4 border-t border-surface-3 bg-surface-1">
           {isDirty && (
              <div className="flex items-center gap-2 text-accent-amber animate-pulse mr-auto">
                <Info size={14} />
                <span className="text-[10px] font-bold uppercase tracking-wider font-mono">Unsaved Changes</span>
              </div>
           )}
           <button 
             onClick={handleCloseAttempt}
             disabled={saving}
             className="flex-1 px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm hover:bg-surface-3 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
           >
             {isDirty ? "Discard" : "Cancel"}
           </button>
           <button 
             onClick={handleSave}
             disabled={saving || !isDirty}
             className={clsx(
               "flex-1 px-4 py-2.5 rounded-xl font-mono text-sm font-semibold transition-colors disabled:opacity-50 disabled:cursor-not-allowed",
               isDirty ? "bg-accent-blue/20 border border-accent-blue/40 text-accent-blue hover:bg-accent-blue/30" : "bg-surface-3 border border-surface-4 text-text-muted"
             )}
           >
             {saving ? "Saving..." : "Save"}
           </button>
        </div>

        {/* Discard Confirmation Modal */}
        {showDiscardConfirm && (
           <div className="absolute inset-0 z-[200] flex items-center justify-center bg-black/50 backdrop-blur-sm p-6 animate-fade-in">
              <div className="card p-6 w-full max-w-xs space-y-6 shadow-2xl bg-surface-1 border border-surface-3 rounded-2xl">
                 <div className="space-y-2">
                    <h4 className="font-bold text-text-primary">Unsaved Changes</h4>
                    <p className="text-xs text-text-muted leading-relaxed font-mono">
                      Your modifications have not been applied. Discard and return to dashboard?
                    </p>
                 </div>
                 <div className="flex flex-col gap-2">
                    <button onClick={handleDiscard} className="w-full px-4 py-2.5 rounded-xl bg-accent-red/20 border border-accent-red/40 text-accent-red font-mono text-sm font-semibold hover:bg-accent-red/30 transition-colors">Discard Changes</button>
                    <button onClick={() => setShowDiscardConfirm(false)} className="w-full px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm hover:bg-surface-3 transition-colors">Keep Editing</button>
                 </div>
              </div>
           </div>
        )}
      </div>
    </div>
  );
}
