// app/page.tsx
"use client";

import { useCallback, useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { ControlMode, LogLevel } from "@/lib/types";

import { usePumpData } from "@/lib/usePumpData";
import DeviceConfigSettings from "@/components/DeviceConfigSettings";
import { ESP32_STALE_SEC } from "@/lib/constants";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
import NotificationSettings from "@/components/NotificationSettings";
import AuthGuard from "@/components/AuthGuard";
import InstallPrompt from "@/components/InstallPrompt";
import { signOut } from "@/lib/auth";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import { toast } from "@/lib/toast";
import ActivityPanel from "@/components/ActivityPanel";
import { usePendingControl } from "@/lib/usePendingControl";
import { useIsAdmin } from "@/lib/useIsAdmin";
import ErrorBoundary from "@/components/ErrorBoundary";

// NEW Phase D4 Components
import Header from "@/components/Header";
import TankLevelCard from "@/components/TankLevelCard";
import PumpStatusCard from "@/components/PumpStatusCard";
import ControlPanel from "@/components/ControlPanel";
import AlertsCard from "@/components/AlertsCard";
import DiagnosticsCard from "@/components/DiagnosticsCard";
import DashboardHistorySection from "@/components/DashboardHistorySection";

export default function DashboardPage() {
  const router = useRouter();
  const {
    snapshot,
    history,
    historyEvents,
    connected,
    authChecked,
    authUser,
    setMode,
    acknowledgeError,
    requestReboot,
    setManualDesired,
    startCountdown,
    addCountdownTime,
    triggerEmergencyStop,
    resetEmergencyStop,
    setBypassLevelSensor,
    setBypassFlowSensor,
  } = usePumpData();

  const { config, saveConfig } = useDeviceConfig();
  const isAdmin = useIsAdmin(authUser?.uid ?? null);

  const handleSetDebugLogLevel = useCallback(async (level: number) => {
    const clamped = Math.max(0, Math.min(4, Math.floor(level))) as LogLevel;
    await saveConfig({ debug_log_level: clamped });
    toast({ kind: "success", title: `Log level set to ${clamped}` });
  }, [saveConfig]);

  const [showNotifications, setShowNotifications] = useState(false);
  const [showDeviceConfig, setShowDeviceConfig] = useState(false);
  const [restartSentAt, setRestartSentAt] = useState<number | null>(null);
  const [restartSawStale, setRestartSawStale] = useState(false);

  // Status timestamp for staleness
  const updatedAt = snapshot?.updatedAt ?? null;

  // Restart feedback logic
  useEffect(() => {
    if (restartSentAt == null || updatedAt == null) return;
    const ageMs = Date.now() - updatedAt;
    if (ageMs > 20000) {
      setRestartSawStale(true);
    } else if (restartSawStale && ageMs < 5000) {
      toast({ kind: "success", title: "Controller back online." });
      setRestartSentAt(null);
      setRestartSawStale(false);
    }
  }, [updatedAt, restartSentAt, restartSawStale]);

  // Derived values with safe defaults
  const status = snapshot?.status;
  const control = snapshot?.control;
  
  const level = status?.water_level_percent ?? 0;
  const flow = status?.flow_rate_lpm ?? 0;
  const running = status?.is_running ?? false;
  const modeCandidate = control?.mode;
  const mode: ControlMode =
    modeCandidate === "AUTO" || modeCandidate === "MANUAL" || modeCandidate === "COUNTDOWN"
      ? modeCandidate
      : "AUTO";
  const manualDesired = status?.manual_desired ?? control?.manual_desired ?? false;
  const emergencyStopLatched = status?.emergency_stop_latched ?? false;

  const { pendingMode, setPendingMode } = usePendingControl(mode);

  const handleRequestReboot = (): Promise<void> => {
    setRestartSentAt(Date.now());
    setRestartSawStale(false);
    toast({ kind: "info", title: "Controller restarting…" });
    return requestReboot().catch(() => {
      toast({ kind: "error", title: "Restart failed" });
      setRestartSentAt(null);
    });
  };

  const handleSignOut = () => {
    signOut().then(() => router.replace("/login"));
  };

  if (!authChecked) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-sf-gray-50">
        <div className="text-center space-y-4">
          <div className="w-10 h-10 border-2 border-sf-blue/30 border-t-sf-blue rounded-full animate-spin mx-auto" />
          <p className="text-sf-gray-600 font-mono text-xs uppercase tracking-widest">Initialising SmartFlow…</p>
        </div>
      </div>
    );
  }

  return (
    <AuthGuard>
      <div className="min-h-screen flex flex-col bg-[var(--page-bg)]">
        <Header 
          isConnected={connected} 
          rssi={status?.wifi_rssi} 
          lastUpdated={updatedAt} 
        />

        <main id="main" className="flex-1 w-full max-w-screen-lg mx-auto px-4 py-6 md:py-8 space-y-6">
          
          {/* Restart Progress Banner */}
          {restartSentAt != null && (
             <div className="card p-3 bg-sf-blue-light border-sf-blue/20 flex items-center gap-3 animate-pulse">
                <div className="w-4 h-4 border-2 border-sf-blue border-t-transparent rounded-full animate-spin" />
                <span className="text-xs font-mono font-medium text-sf-blue">Controller restarting... syncing telemetry</span>
             </div>
          )}

          {/* Alert Section */}
          <ErrorBoundary componentName="AlertsCard">
            <AlertsCard 
              isEmergencyStopLatched={emergencyStopLatched}
              isDryRunError={status?.is_error ?? false}
              isOverflowError={status?.is_overflow_error ?? false}
              lastFaultMessage={status?.last_fault_message}
              isSensorError={status?.is_sensor_error ?? false}
              isFlowSensorError={status?.is_flow_sensor_error ?? false}
              isManualRuntimeWarning={status?.manual_runtime_warning ?? false}
              isRemoteSensorStable={status?.remote_sensor_stable ?? true}
              isLevelBypass={status?.bypass_level_sensor ?? false}
              isFlowBypass={status?.bypass_flow_sensor ?? false}
              isIdleMode={status?.is_idle_mode ?? false}
              onResetEStop={resetEmergencyStop}
              onClearError={acknowledgeError}
            />
          </ErrorBoundary>

          {/* Primary Operations Grid */}
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            
            <ErrorBoundary componentName="TankLevelCard">
              <TankLevelCard 
                level={status?.level_estimate_active ? (status?.estimated_level_pct ?? level) : level}
                distanceCm={status?.ultrasonic_last_good_cm}
                isFresh={status?.level_fresh ?? true}
                isSensorError={status?.is_sensor_error ?? false}
                isEstimate={status?.level_estimate_active ?? false}
                addedVolumeL={status?.flow_volume_added_l}
                startLevel={config?.pump_start_level}
                stopLevel={config?.pump_stop_level}
                isLoading={!status}
              />
            </ErrorBoundary>

            <ErrorBoundary componentName="PumpStatusCard">
              <PumpStatusCard 
                runMode={status?.run_mode ?? "AUTO_STANDBY"}
                isRunning={running}
                flowRate={flow}
                uptimeMin={status?.uptime_minutes ?? 0}
                bootReason={status?.last_boot_reason ?? "Unknown"}
                totalCycles={status?.total_pump_cycles ?? 0}
                cooldownRemainingSec={status?.pump_cooldown_remaining_sec}
                isLoading={!status}
              />
            </ErrorBoundary>

            <ErrorBoundary componentName="ControlPanel">
               <ControlPanel 
                  currentMode={mode}
                  manualDesired={manualDesired}
                  isEmergencyStopLatched={emergencyStopLatched}
                  isPending={!!pendingMode}
                  countdownRemainingSec={status?.countdown_remaining_sec}
                  onSetMode={async (m) => {
                    setPendingMode(m);
                    try { await setMode(m); } catch { toast({ kind: "error", title: "Mode change failed" }); }
                  }}
                  onSetManualDesired={setManualDesired}
                  onStartCountdown={startCountdown}
                  onAddCountdownTime={addCountdownTime}
                  onEmergencyStop={triggerEmergencyStop}
                  onResetStop={resetEmergencyStop}
                  startLevel={config?.pump_start_level}
                  stopLevel={config?.pump_stop_level}
                  isLoading={!status}
               />
            </ErrorBoundary>
          </div>

          {/* Secondary Layout: History & Diagnostics */}
          <div className="grid grid-cols-1 gap-6">
            <ErrorBoundary componentName="DiagnosticsCard">
              <DiagnosticsCard 
                freeHeap={status?.free_heap_bytes ?? 0}
                minFreeHeap={status?.min_free_heap_observed_bytes ?? 0}
                uptime={status?.uptime_minutes ? `${Math.floor(status.uptime_minutes / 60)}h ${status.uptime_minutes % 60}m` : "Checking..."}
                bootReason={status?.last_boot_reason ?? "Unknown"}
                totalCycles={status?.total_pump_cycles ?? 0}
                totalRunTime={status?.total_pump_run_min ? `${Math.floor(status.total_pump_run_min / 60)}h ${status.total_pump_run_min % 60}m` : "Checking..."}
                isSensorStable={status?.remote_sensor_stable ?? true}
                isLevelFresh={status?.level_fresh ?? true}
                sensorHealth={status?.level_sensor_health_pct ?? 100}
                lastGoodDistanceCm={status?.ultrasonic_last_good_cm ?? 0}
                levelDiscardCount={status?.remote_level_discard_count ?? 0}
                isFirebaseConnected={connected}
                fbLastError={status?.firebase_last_error}
                fbConsecFailures={status?.firebase_consecutive_failures ?? 0}
                currentLogLevel={status?.debug_log_level ?? 2}
                onSetLogLevel={handleSetDebugLogLevel}
                isLoading={!status}
              />
            </ErrorBoundary>

            <ErrorBoundary componentName="DashboardHistorySection">
              <DashboardHistorySection
                connected={connected}
                updateLabel={running ? "Real-time sync" : "Power-save sync active"}
                history={history}
                events={historyEvents}
                pumpStartLevel={config?.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level}
                pumpStopLevel={config?.pump_stop_level ?? DEFAULT_DEVICE_CONFIG.pump_stop_level}
              />
            </ErrorBoundary>

            <ErrorBoundary componentName="ActivityPanel">
               <ActivityPanel />
            </ErrorBoundary>
          </div>

          {/* Footer Actions */}
          <div className="flex flex-wrap items-center justify-center gap-4 pt-8 pb-4 opacity-60 hover:opacity-100 transition-opacity">
             <button onClick={() => setShowDeviceConfig(true)} className="text-[10px] font-bold uppercase tracking-widest hover:text-sf-blue">Device Settings</button>
             <button onClick={() => setShowNotifications(true)} className="text-[10px] font-bold uppercase tracking-widest hover:text-sf-blue">Alerts Config</button>
             <button onClick={handleSignOut} className="text-[10px] font-bold uppercase tracking-widest hover:text-sf-red">Sign Out</button>
          </div>

        </main>
      </div>

      {showNotifications && (
        <NotificationSettings
          userUid={authUser?.uid ?? null}
          userEmail={authUser?.email ?? null}
          isAdmin={isAdmin}
          onClose={() => setShowNotifications(false)}
        />
      )}
      {showDeviceConfig && (
        <DeviceConfigSettings
          isAdmin={isAdmin}
          actorUid={authUser?.uid ?? null}
          actorEmail={authUser?.email ?? null}
          onClose={() => setShowDeviceConfig(false)}
          esp32Online={updatedAt != null && (Date.now() - updatedAt) / 1000 < ESP32_STALE_SEC}
          onRequestReboot={handleRequestReboot}
          bypassLevelSensor={status?.bypass_level_sensor ?? false}
          bypassFlowSensor={status?.bypass_flow_sensor ?? false}
          onSetBypassLevelSensor={setBypassLevelSensor}
          onSetBypassFlowSensor={setBypassFlowSensor}
        />
      )}
      <InstallPrompt />
    </AuthGuard>
  );
}
