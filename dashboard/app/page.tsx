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
import { usePresence } from "@/lib/usePresence";
import ActivityPanel from "@/components/ActivityPanel";
import { usePendingControl } from "@/lib/usePendingControl";
import { useIsAdmin } from "@/lib/useIsAdmin";
import DashboardHeader from "@/components/DashboardHeader";
import DashboardMainGrid from "@/components/DashboardMainGrid";
import DashboardHistorySection from "@/components/DashboardHistorySection";
import DashboardSystemInfo from "@/components/DashboardSystemInfo";

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
    startTimedRun,
    stopRun,
  } = usePumpData();

  const { config } = useDeviceConfig();
  const isAdmin = useIsAdmin(authUser?.uid ?? null);
  const { onlineCount } = usePresence(authUser?.uid ?? null, authUser?.email ?? null);

  // Ticker for "time ago" refresh
  const [tick, setTick] = useState(0);
  void tick;
  const [showNotifications, setShowNotifications] = useState(false);
  const [showDeviceConfig, setShowDeviceConfig] = useState(false);
  useEffect(() => {
    const id = setInterval(() => setTick((t) => t + 1), 1000);
    return () => clearInterval(id);
  }, []);

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
  const updatedAt = snapshot?.updatedAt ?? null;
  const esp32Online = updatedAt != null && (Date.now() - updatedAt) / 1000 < ESP32_STALE_SEC;

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
          isSensorError={snapshot?.status.is_sensor_error}
          isOverflowError={snapshot?.status.is_overflow_error}
          isSleeping={snapshot?.status.is_sleeping}
          wifiRssi={snapshot?.status.wifi_rssi}
          bootReason={snapshot?.status.last_boot_reason}
          uptimeMinutes={snapshot?.status.uptime_minutes}
          onlineUsers={onlineCount}
        />

        <DashboardHeader
          userEmail={authUser?.email ?? ""}
          running={running}
          isError={isError}
          isAdmin={isAdmin}
          esp32Online={esp32Online}
          onRequestReboot={() => {
            toast({ kind: "info", title: "Restarting controller…" });
            requestReboot().catch(() => {
              toast({ kind: "error", title: "Restart failed" });
            });
          }}
          onOpenDeviceConfig={() => setShowDeviceConfig(true)}
          onOpenNotifications={() => setShowNotifications(true)}
          onSignOut={() => signOut().then(() => router.replace("/login"))}
        />

        {/* ── Main layout ───────────────────────────────────────────────────── */}
        <main id="main" className="flex-1 px-4 sm:px-6 pb-6 sm:pb-8 min-w-0">
          <div className="max-w-6xl mx-auto space-y-3 sm:space-y-4">

            {/* Connection error banner */}
            {error && (
              <div className="flex items-center gap-2 sm:gap-3 p-3 rounded-xl bg-accent-red/10
                            border border-accent-red/30 text-accent-red text-xs sm:text-sm font-mono">
                <AlertTriangle size={14} className="shrink-0" />
                <span className="min-w-0 break-words">Can&apos;t connect to the pump. {error}</span>
              </div>
            )}

            <DashboardMainGrid
              level={level}
              flow={flow}
              running={running}
              isError={isError}
              mode={mode}
              status={snapshot?.status ?? null}
              config={config}
              isAdmin={isAdmin}
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
                window.setTimeout(() => setPendingMode(null), 8000);
              }}
              onAcknowledge={() => {
                setPendingAck(true);
                toast({ kind: "info", title: "Sending acknowledge…" });
                acknowledgeError();
                window.setTimeout(() => setPendingAck(false), 6000);
              }}
              onStartManualRun={() => {
                toast({ kind: "info", title: "Starting manual run…" });
                startManualRun();
              }}
              onStartTimedRun={(durationSec) => {
                const min = Math.round(durationSec / 60);
                toast({ kind: "info", title: `Starting timed run (${min} min)…` });
                startTimedRun(durationSec);
              }}
              onStopRun={() => {
                toast({ kind: "info", title: "Stopping pump…" });
                stopRun();
              }}
            />

            <DashboardHistorySection connected={connected} updateLabel={updateLabel} history={history} />

            <DashboardSystemInfo />

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
          onRequestReboot={requestReboot}
        />
      )}
      <InstallPrompt />
    </AuthGuard>
  );
}
