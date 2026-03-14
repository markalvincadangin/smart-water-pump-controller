// app/page.tsx
"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import {
  AlertTriangle,
} from "lucide-react";

import { usePumpData } from "@/lib/usePumpData";
import DeviceConfigSettings from "@/components/DeviceConfigSettings";
import { ESP32_STALE_SEC } from "@/lib/constants";
import { useDeviceConfig } from "@/lib/useDeviceConfig";
import NotificationSettings from "@/components/NotificationSettings";
import StatusBar from "@/components/StatusBar";
import AuthGuard from "@/components/AuthGuard";
import InstallPrompt from "@/components/InstallPrompt";
import { signOut } from "@/lib/auth";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import { toast } from "@/lib/toast";
import ActivityPanel from "@/components/ActivityPanel";
import { usePendingControl } from "@/lib/usePendingControl";
import { useIsAdmin } from "@/lib/useIsAdmin";
import DashboardHeader from "@/components/DashboardHeader";
import DashboardMainGrid from "@/components/DashboardMainGrid";
import DashboardSkeleton from "@/components/DashboardSkeleton";
import DashboardHistorySection from "@/components/DashboardHistorySection";
import DashboardSystemInfo from "@/components/DashboardSystemInfo";
import { getRankedAlerts } from "@/lib/alertRanking";

export default function DashboardPage() {
  const router = useRouter();
  const {
    snapshot,
    history,
    connected,
    authChecked,
    authUser,
    error,
    setMode,
    acknowledgeError,
    requestReboot,
    startManualRun,
    startCountdown,
    addCountdownTime,
    isAddingCountdownTime,
    stopRun,
    setBypassLevelSensor,
  } = usePumpData();

  const { config } = useDeviceConfig();
  const isAdmin = useIsAdmin(authUser?.uid ?? null);

  // Ticker for "time ago" refresh
  const [tick, setTick] = useState(0);
  void tick;
  const [showNotifications, setShowNotifications] = useState(false);
  const [showDeviceConfig, setShowDeviceConfig] = useState(false);
  const [restartSentAt, setRestartSentAt] = useState<number | null>(null);
  const [restartSawStale, setRestartSawStale] = useState(false);
  useEffect(() => {
    const id = setInterval(() => setTick((t) => t + 1), 1000);
    return () => clearInterval(id);
  }, []);

  // Status timestamp for staleness and restart feedback (declared early for useEffect deps)
  const updatedAt = snapshot?.updatedAt ?? null;

  // Restart feedback: detect when controller goes stale then comes back online
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
  const level = snapshot?.status.water_level_percent ?? 0;
  const flow = snapshot?.status.flow_rate_lpm ?? 0;
  const running = snapshot?.status.is_running ?? false;
  const isError = snapshot?.status.is_error ?? false;
  const mode = snapshot?.control.mode ?? "AUTO";
  const isSleeping = snapshot?.status.is_sleeping ?? false;

  const { pendingMode, setPendingMode, pendingAck, setPendingAck } = usePendingControl(mode);

  const idleUpdateSec = Math.round((config?.idle_firebase_interval_ms ?? DEFAULT_DEVICE_CONFIG.idle_firebase_interval_ms) / 1000);
  const updateLabel = running
    ? "updated every 3s"
    : isSleeping
      ? `updated every ~${idleUpdateSec}s (sleep)`
      : `updated every ~${idleUpdateSec}s when idle`;

  // ESP32 online = we have received a status update within the last STALE_SEC seconds
  const esp32Online = updatedAt != null && (Date.now() - updatedAt) / 1000 < ESP32_STALE_SEC;

  const handleRequestReboot = (): Promise<void> => {
    setRestartSentAt(Date.now());
    setRestartSawStale(false);
    toast({ kind: "info", title: "Controller restarting…" });
    return requestReboot().catch(() => {
      toast({ kind: "error", title: "Restart failed" });
      setRestartSentAt(null);
    });
  };

  // ── Loading state ─────────────────────────────────────────────────────────
  // Only block while Firebase auth is still initializing.
  // Once authChecked is true, AuthGuard will redirect unauthenticated/unauthorized users.
  if (!authChecked) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <div className="text-center space-y-4">
          <div className="w-10 h-10 border-2 border-accent-cyan/30 border-t-accent-cyan
                          rounded-full animate-spin mx-auto" />
          <p className="text-text-secondary font-mono text-sm">Connecting to Smart Water Pump System…</p>
        </div>
      </div>
    );
  }

  return (
    <AuthGuard>
      <div className="min-h-screen flex flex-col">
        {/* ── Top status bar ────────────────────────────────────────────────── */}
        <StatusBar
          connected={connected}
          esp32Online={esp32Online}
          updatedAt={updatedAt}
          mode={mode}
          isLevelSensorError={snapshot?.status.is_level_sensor_error ?? snapshot?.status.is_sensor_error}
          isFlowSensorError={snapshot?.status.is_flow_sensor_error}
          isOverflowError={snapshot?.status.is_overflow_error}
          isSleeping={snapshot?.status.is_sleeping}
          wifiRssi={snapshot?.status.wifi_rssi}
          bootReason={snapshot?.status.last_boot_reason}
          uptimeMinutes={snapshot?.status.uptime_minutes}
          levelLastValidAgeSec={snapshot?.status.level_last_valid_age_sec}
          levelSensorHealthPct={snapshot?.status.level_sensor_health_pct}
        />

        <DashboardHeader
          userEmail={authUser?.email ?? ""}
          running={running}
          isError={isError}
          isAdmin={isAdmin}
          esp32Online={esp32Online}
          onRequestReboot={handleRequestReboot}
          onOpenDeviceConfig={() => setShowDeviceConfig(true)}
          onOpenNotifications={() => setShowNotifications(true)}
          onSignOut={() => signOut().then(() => router.replace("/login"))}
        />

        {/* ── Main layout ───────────────────────────────────────────────────── */}
        <main id="main" className="flex-1 px-4 sm:px-6 pb-6 sm:pb-8 min-w-0">
          <div className="max-w-6xl mx-auto space-y-3 sm:space-y-4">

            {/* Dashboard offline / Firebase unreachable */}
            {!connected && error && (
              <div className="flex flex-col gap-1 p-3 rounded-xl bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs sm:text-sm font-mono">
                <div className="flex items-center gap-2">
                  <AlertTriangle size={14} className="shrink-0" />
                  <span className="font-semibold uppercase tracking-wide">Dashboard offline</span>
                </div>
                <span className="min-w-0 break-words text-text-primary">Can&apos;t connect to the cloud. {error}</span>
              </div>
            )}

            {/* Restart feedback banner — B6: 3-phase messaging */}
            {restartSentAt != null && (() => {
              const restartElapsedSec = Math.floor((Date.now() - restartSentAt) / 1000);
              const phase1 = restartElapsedSec < 10;
              const phase2 = restartElapsedSec >= 10 && restartElapsedSec < 30;
              const phase3 = restartElapsedSec >= 30;
              const isAmber = phase3;
              return (
                <div className={isAmber
                  ? "flex items-center gap-2 sm:gap-3 p-3 rounded-xl bg-accent-amber/10 border border-accent-amber/30 text-accent-amber text-xs sm:text-sm font-mono"
                  : "flex items-center gap-2 sm:gap-3 p-3 rounded-xl bg-accent-cyan/10 border border-accent-cyan/30 text-accent-cyan text-xs sm:text-sm font-mono"
                }>
                  {phase1 && (
                    <div className="w-4 h-4 border-2 border-current border-t-transparent rounded-full animate-spin shrink-0" />
                  )}
                  {!phase1 && <span className="shrink-0">⟳</span>}
                  <span className="min-w-0">
                    {phase1 && "Controller restarting… (usually completes in 10–20 seconds)"}
                    {phase2 && `Waiting for controller… (${restartElapsedSec}s elapsed)`}
                    {phase3 && `Controller hasn't responded yet (${restartElapsedSec}s elapsed). If it doesn't reconnect in the next 30 seconds, try a manual power cycle.`}
                  </span>
                </div>
              );
            })()}

            {/* Ranked alerts (v2 §11.3): controller offline, dry-run, overflow, auto-maintenance, maintenance, level/flow error, sleeping */}
            {getRankedAlerts(snapshot?.status, esp32Online).map((alert) => (
              <div
                key={alert.id}
                className={alert.severity === "red"
                  ? "flex flex-col gap-1 sm:gap-2 p-3 rounded-xl bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs sm:text-sm font-mono"
                  : alert.severity === "amber"
                    ? "flex flex-col gap-1 p-3 rounded-xl bg-accent-amber/10 border border-accent-amber/30 text-accent-amber text-xs sm:text-sm font-mono"
                    : "flex flex-col gap-0.5 p-2.5 rounded-lg bg-accent-cyan/10 border border-accent-cyan/20 text-accent-cyan text-xs font-mono"
                }
              >
                <div className="flex items-center justify-between gap-2 flex-wrap">
                  <div className="flex items-center gap-2 min-w-0">
                    <AlertTriangle size={14} className="shrink-0" />
                    <span className="font-semibold uppercase tracking-wide">{alert.title}</span>
                  </div>
                  {(alert.id === "dry_run" || alert.id === "overflow") && (
                    <button
                      type="button"
                      onClick={() => {
                        setPendingAck(true);
                        toast({ kind: "info", title: "Sending acknowledge…" });
                        acknowledgeError();
                        window.setTimeout(() => {
                          setPendingAck((prev) => {
                            if (prev) toast({ kind: "warning", title: "Command timed out" });
                            return false;
                          });
                        }, 8000);
                      }}
                      disabled={pendingAck || !esp32Online}
                      className="min-h-[44px] min-w-[44px] shrink-0 px-3 py-2 rounded-lg bg-accent-red/20 border border-accent-red/50 text-accent-red text-xs font-mono font-semibold hover:bg-accent-red/30 touch-manipulation disabled:opacity-50"
                    >
                      {pendingAck ? "Sending…" : "Clear Error"}
                    </button>
                  )}
                  {(alert.id === "level_sensor" || alert.id === "maintenance") && (
                    <button
                      type="button"
                      onClick={() => isAdmin && setBypassLevelSensor(true)}
                      disabled={!isAdmin}
                      className="min-h-[44px] min-w-[44px] shrink-0 px-3 py-2 rounded-lg bg-accent-amber/20 border border-accent-amber/50 text-accent-amber text-xs font-mono font-semibold hover:bg-accent-amber/30 touch-manipulation disabled:opacity-50 disabled:cursor-not-allowed"
                      title={!isAdmin ? "Admin access required" : alert.id === "maintenance" ? "Bypass already enabled" : "Enable level sensor bypass"}
                    >
                      {alert.id === "maintenance" ? "Bypass On" : "Enable Bypass"}
                    </button>
                  )}
                </div>
                <span className="min-w-0 text-text-primary">{alert.description}</span>
                {alert.recovery && <span className="min-w-0 text-text-muted">{alert.recovery}</span>}
              </div>
            ))}

            {snapshot?.status == null ? (
              <DashboardSkeleton />
            ) : (
            <DashboardMainGrid
              level={level}
              flow={flow}
              running={running}
              isError={isError}
              mode={mode}
              status={snapshot?.status ?? null}
              config={config}
              isAdmin={isAdmin}
              esp32Online={esp32Online}
              pendingMode={pendingMode}
              pendingAck={pendingAck}
              onSetMode={(nextMode) => {
                if (nextMode === "FORCE_ON" && !isAdmin) {
                  toast({ kind: "warning", title: "FORCE ON is admin-only" });
                  return;
                }
                setPendingMode(nextMode);
                toast({ kind: "info", title: `Sending mode: ${nextMode}` });
                setMode(nextMode);
                window.setTimeout(() => {
                  setPendingMode((prev) => {
                    if (prev === nextMode) toast({ kind: "warning", title: "Command timed out" });
                    return null;
                  });
                }, 8000);
              }}
              onAcknowledge={() => {
                setPendingAck(true);
                toast({ kind: "info", title: "Sending acknowledge…" });
                acknowledgeError();
                window.setTimeout(() => {
                  setPendingAck((prev) => {
                    if (prev) toast({ kind: "warning", title: "Command timed out" });
                    return false;
                  });
                }, 8000);
              }}
              onStartManualRun={() => {
                toast({ kind: "info", title: "Starting manual run…" });
                startManualRun();
              }}
              onStartCountdown={(durationMin) => {
                toast({ kind: "info", title: `Starting countdown (${durationMin} min)…` });
                startCountdown(durationMin);
              }}
              onAddCountdownTime={() => {
                toast({ kind: "info", title: "Adding 5 min to countdown…" });
                addCountdownTime();
              }}
              isAddingCountdownTime={isAddingCountdownTime}
              onStopRun={() => {
                toast({ kind: "info", title: "Stopping pump…" });
                stopRun();
              }}
            />
            )}

            <DashboardHistorySection
              connected={connected}
              updateLabel={updateLabel}
              history={history}
              pumpStartLevel={config?.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level}
              pumpStopLevel={config?.pump_stop_level ?? DEFAULT_DEVICE_CONFIG.pump_stop_level}
            />

            <DashboardSystemInfo status={snapshot?.status ?? null} />

            {/* ── Row 4: Activity (multi-user) ─────────────────────────────── */}
            <ActivityPanel />
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
          esp32Online={esp32Online}
          onRequestReboot={handleRequestReboot}
          bypassLevelSensor={snapshot?.status.bypass_level_sensor ?? false}
          onSetBypassLevelSensor={setBypassLevelSensor}
        />
      )}
      <InstallPrompt />
    </AuthGuard>
  );
}
