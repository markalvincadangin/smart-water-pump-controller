// components/DeviceConfigSettings.tsx
"use client";

import { useState, useEffect, useMemo } from "react";
import { Settings, X, RotateCw, AlertTriangle, ShieldAlert, Cpu, Droplets, Info } from "lucide-react";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
import { validateDeviceConfig } from "@/lib/validation";
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
  const [showAdvanced, setShowAdvanced] = useState(false);
  const [rebootBusy, setRebootBusy] = useState(false);
  const [bypassBusy, setBypassBusy] = useState(false);

  // SF Styles
  const sectionTitleClass = "text-xs font-bold uppercase tracking-widest text-sf-blue-mid mb-4 flex items-center gap-2";
  const fieldGroupClass = "space-y-4 p-4 rounded-xl bg-sf-gray-50/50 border border-sf-gray-100";
  const fieldLabelClass = "flex items-center gap-2 text-[10px] uppercase font-bold tracking-wider text-sf-gray-600 mb-1.5";
  const inputClass = "w-full bg-white border border-sf-gray-200 rounded-chip px-3 py-2 text-sm font-mono focus:outline-sf-blue transition-all disabled:opacity-50";
  const helperTextClass = "text-[10px] text-sf-gray-400 mt-1.5 leading-relaxed font-mono";

  useEffect(() => {
    if (config) setForm(config);
  }, [config]);

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
    const { isValid, error } = validateDeviceConfig(form);
    if (!isValid) {
      setSaveError(error);
      return;
    }
    setSaving(true);
    try {
      await saveConfig(form);
      toast({ kind: "success", title: "Config synched", detail: "Controller will update on next sync." });
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
      setBypassBusy(true);
      try {
         await onSetBypassLevelSensor(checked);
      } finally {
         setBypassBusy(false);
      }
   }

   async function handleSetBypassFlowSensor(checked: boolean) {
      if (!onSetBypassFlowSensor || bypassBusy) return;
      setBypassBusy(true);
      try {
         await onSetBypassFlowSensor(checked);
      } finally {
         setBypassBusy(false);
      }
   }

  if (loading) {
    return (
      <div className="fixed inset-0 z-50 flex items-center justify-center bg-sf-gray-900/60 backdrop-blur-sm animate-fade-in">
        <div className="w-10 h-10 border-2 border-sf-blue/30 border-t-sf-blue rounded-full animate-spin" />
      </div>
    );
  }

  return (
    <div 
      className="fixed inset-0 z-[100] flex items-end sm:items-center justify-center bg-sf-gray-900/40 backdrop-blur-md p-0 sm:p-4 animate-fade-in"
      onClick={(e) => e.target === e.currentTarget && handleCloseAttempt()}
      role="dialog"
      aria-modal="true"
      aria-labelledby="settings-title"
    >
      <div className="card w-full max-w-md max-h-[90dvh] flex flex-col rounded-t-2xl sm:rounded-2xl shadow-2xl animate-slide-up overflow-hidden">
        
        {/* Header (Fixed) */}
        <div className="flex items-center justify-between p-6 pb-4 border-b border-sf-gray-100 shrink-0">
          <div className="flex items-center gap-3">
             <div className="h-10 w-10 flex items-center justify-center rounded-chip bg-sf-blue-light text-sf-blue">
                <Settings size={22} className={saving ? "animate-spin-slow" : ""} />
             </div>
             <div>
                <h2 id="settings-title" className="text-base font-bold text-sf-gray-900">Device Settings</h2>
                <div 
                  className={clsx(
                    "text-[10px] font-mono",
                    esp32Online ? "text-sf-teal" : "text-sf-amber"
                  )}
                >
                  {esp32Online ? "ONLINE \u2014 Sync Active" : "OFFLINE \u2014 Sync Pending"}
                </div>
             </div>
          </div>
          <button onClick={handleCloseAttempt} className="p-2 text-sf-gray-400 hover:text-sf-gray-900 transition-colors">
            <X size={20} />
          </button>
        </div>

        {/* Scrollable Form Content */}
        <div className="flex-1 overflow-y-auto px-6 py-6 space-y-8 scrollbar-hide">
          
          {/* Admin Mode Toggle */}
          {isAdmin && (
             <div className="flex p-1 bg-sf-gray-50 rounded-chip border border-sf-gray-100 gap-1">
                <button 
                   onClick={() => setShowAdvanced(false)}
                   className={clsx(
                     "flex-1 py-1.5 text-[10px] font-bold tracking-widest rounded-chip transition-all",
                     !showAdvanced ? "bg-white text-sf-blue shadow-sm" : "text-sf-gray-400"
                   )}
                >
                   BASIC
                </button>
                <button 
                   onClick={() => setShowAdvanced(true)}
                   className={clsx(
                     "flex-1 py-1.5 text-[10px] font-bold tracking-widest rounded-chip transition-all",
                     showAdvanced ? "bg-white text-sf-blue shadow-sm" : "text-sf-gray-400"
                   )}
                >
                   ADVANCED
                </button>
             </div>
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
                         value={form.tank_empty_cm} 
                         onChange={(e) => setForm(f => ({...f, tank_empty_cm: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                      />
                   </div>
                   <div>
                      <label className={fieldLabelClass}>
                         Full (cm)
                         <InfoTooltip content="Distance from sensor to water when tank is full" />
                      </label>
                      <input 
                         type="number" 
                         value={form.tank_full_cm} 
                         onChange={(e) => setForm(f => ({...f, tank_full_cm: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                      />
                   </div>
                </div>
                <p className={helperTextClass}>These values define the 0% and 100% water level visualization.</p>
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
                      />
                   </div>
                   <div>
                      <label className={fieldLabelClass}>Stop AT (%)</label>
                      <input 
                         type="number" 
                         value={form.pump_stop_level} 
                         onChange={(e) => setForm(f => ({...f, pump_stop_level: parseInt(e.target.value) || 0}))} 
                         className={inputClass}
                      />
                   </div>
                </div>
                <div>
                   <label className={fieldLabelClass}>Max Runtime (MIN)</label>
                   <input 
                      type="number" 
                      value={form.max_pump_runtime_min} 
                      onChange={(e) => setForm(f => ({...f, max_pump_runtime_min: parseInt(e.target.value) || 0}))} 
                      className={inputClass}
                   />
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
                            />
                         </div>
                         <div>
                            <label className={fieldLabelClass}>Cutoff (SEC)</label>
                            <input 
                               type="number" 
                               value={form.dry_run_timeout_sec} 
                               onChange={(e) => setForm(f => ({...f, dry_run_timeout_sec: parseInt(e.target.value) || 0}))} 
                               className={inputClass}
                            />
                         </div>
                      </div>
                      <div className="pt-2">
                        <label className="flex items-center gap-3 cursor-pointer group">
                           <input 
                              type="checkbox" 
                              checked={form.auto_bypass_on_sensor_fail ?? false} 
                              onChange={(e) => setForm(f => ({...f, auto_bypass_on_sensor_fail: e.target.checked}))}
                              className="w-4 h-4 rounded border-sf-gray-300 text-sf-blue focus:ring-sf-blue"
                           />
                           <span className="text-xs font-bold text-sf-gray-700 group-hover:text-sf-blue transition-colors">
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
                              />
                           </div>
                        )}
                      </div>
                   </div>
                </section>

                {/* Maintenance Bypass (Active Overrides) */}
                {(onSetBypassLevelSensor || onSetBypassFlowSensor) && (
                   <section className="space-y-4">
                      <div className={sectionTitleClass}><AlertTriangle size={14} /> Maintenance Overrides</div>
                      <div className="space-y-3">
                         {onSetBypassLevelSensor && (
                            <div className="p-4 rounded-xl bg-sf-amber-light/30 border border-sf-amber/10">
                               <label className="flex items-center gap-3 cursor-pointer">
                                  <input 
                                     type="checkbox" 
                                     checked={!!bypassLevelSensor} 
                                     onChange={(e) => handleSetBypassLevelSensor(e.target.checked)}
                                     disabled={bypassBusy}
                                     className="w-4 h-4 rounded border-sf-amber/30 text-sf-amber focus:ring-sf-amber"
                                  />
                                  <span className="text-xs font-bold text-sf-amber-dark">Bypass Level Sensor</span>
                               </label>
                               <p className={helperTextClass}>Enables pump operation regardless of tank level telemetry.</p>
                            </div>
                         )}
                         {onSetBypassFlowSensor && (
                            <div className="p-4 rounded-xl bg-sf-red-light/30 border border-sf-red/10">
                               <label className="flex items-center gap-3 cursor-pointer">
                                  <input 
                                     type="checkbox" 
                                     checked={!!bypassFlowSensor} 
                                     onChange={(e) => handleSetBypassFlowSensor(e.target.checked)}
                                     disabled={bypassBusy}
                                     className="w-4 h-4 rounded border-sf-red/30 text-sf-red focus:ring-sf-red"
                                  />
                                  <span className="text-xs font-bold text-sf-red-dark">Bypass Flow Guard</span>
                               </label>
                               <p className={helperTextClass}>Disables dry-run protection. <span className="font-bold underline">EXTREME CAUTION REQUIRED.</span></p>
                            </div>
                         )}
                      </div>
                   </section>
                )}

                {/* Quiet Hours */}
                <section className="space-y-4">
                   <div className={sectionTitleClass}><ShieldAlert size={14} /> Quiet Hours</div>
                   <div className={fieldGroupClass}>
                      <label className="flex items-center gap-3 cursor-pointer">
                         <input 
                            type="checkbox" 
                            checked={form.sleep_enabled} 
                            onChange={(e) => setForm(f => ({...f, sleep_enabled: e.target.checked}))}
                            className="w-4 h-4 rounded border-sf-gray-300 text-sf-blue focus:ring-sf-blue"
                         />
                         <span className="text-xs font-bold text-sf-gray-700 font-mono">Enable Silent Period</span>
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
                            className="w-full flex items-center justify-center gap-2 py-3 border border-sf-gray-200 rounded-chip text-sf-gray-600 font-bold hover:bg-sf-gray-50 disabled:opacity-50 transition-colors"
                         >
                            <RotateCw size={16} className={rebootBusy ? "animate-spin" : ""} />
                            {rebootBusy ? "Restarting..." : "Restart Device"}
                         </button>
                      )}
                      
                      <button 
                         onClick={handleSeedDefaults}
                         disabled={seeding}
                         className="w-full py-3 text-[10px] font-bold text-sf-gray-400 uppercase tracking-widest hover:text-sf-blue transition-colors"
                      >
                         {seeding ? "Syncing..." : "Seed Default Constants"}
                      </button>
                   </div>
                </section>
             </div>
          )}

          {saveError && (
             <div className="p-4 rounded-xl bg-sf-red-light/30 border border-sf-red/20 text-sf-red text-xs font-mono animate-fade-in">
                {saveError}
             </div>
          )}
        </div>

        {/* Action Footer (Sticky) */}
        <div className="p-6 border-t border-sf-gray-100 bg-white shrink-0 space-y-4 shadow-top">
           {isDirty && (
              <div className="flex items-center gap-2 text-sf-amber animate-pulse">
                <Info size={14} />
                <span className="text-[10px] font-bold uppercase tracking-wider font-mono">Unsaved Changes Detected</span>
              </div>
           )}
           <div className="flex gap-4">
              <button 
                onClick={handleCloseAttempt}
                className="flex-1 btn-ghost py-3 font-bold"
                disabled={saving}
              >
                {isDirty ? "Discard" : "Cancel"}
              </button>
              <button 
                onClick={handleSave}
                disabled={saving || !isDirty}
                className={clsx(
                  "flex-1 py-3 rounded-chip font-bold transition-all shadow-lg",
                  isDirty ? "btn-primary hover:shadow-sf-blue/20" : "bg-sf-gray-100 text-sf-gray-400 cursor-not-allowed"
                )}
              >
                {saving ? "Saving..." : "Save Settings"}
              </button>
           </div>
        </div>

        {/* Discard Confirmation Modal */}
        {showDiscardConfirm && (
           <div className="absolute inset-0 z-[200] flex items-center justify-center bg-sf-gray-900/60 backdrop-blur-sm p-6 animate-fade-in">
              <div className="card p-6 w-full max-w-xs space-y-6 shadow-2xl">
                 <div className="space-y-2">
                    <h4 className="font-bold text-sf-gray-900">Unsaved Changes</h4>
                    <p className="text-xs text-sf-gray-500 leading-relaxed font-mono">
                      Your modifications have not been applied. Discard and return to dashboard?
                    </p>
                 </div>
                 <div className="flex flex-col gap-2">
                    <button onClick={handleDiscard} className="btn-danger py-2.5 text-xs">Discard Changes</button>
                    <button onClick={() => setShowDiscardConfirm(false)} className="btn-ghost py-2.5 text-xs">Keep Editing</button>
                 </div>
              </div>
           </div>
        )}
      </div>
    </div>
  );
}
