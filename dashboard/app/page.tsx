// app/page.tsx
"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import clsx from "clsx";
import {
  Droplets,
  Wind,
  Activity,
  AlertTriangle,
  RefreshCw,
  Bell,
} from "lucide-react";

import { usePumpData } from "@/lib/usePumpData";
import NotificationSettings from "@/components/NotificationSettings";
import TankVisual       from "@/components/TankVisual";
import ModeControls     from "@/components/ModeControls";
import HistoryChart     from "@/components/HistoryChart";
import StatCard         from "@/components/StatCard";
import StatusBar        from "@/components/StatusBar";
import AuthGuard        from "@/components/AuthGuard";
import { signOut }      from "@/lib/auth";

export default function DashboardPage() {
  const router = useRouter();
  const {
    snapshot,
    history,
    connected,
    authReady,
    authChecked,
    authUser,
    error,
    setMode,
    acknowledgeError,
  } = usePumpData();

  // Redirect to login when auth is checked but no user
  useEffect(() => {
    if (authChecked && !authUser) {
      router.replace("/login");
    }
  }, [authChecked, authUser, router]);

  // Ticker for "time ago" refresh
  const [tick, setTick] = useState(0);
  const [showNotifications, setShowNotifications] = useState(false);
  useEffect(() => {
    const id = setInterval(() => setTick((t) => t + 1), 1000);
    return () => clearInterval(id);
  }, []);

  // Derived values with safe defaults
  const level    = snapshot?.status.water_level_percent ?? 0;
  const flow     = snapshot?.status.flow_rate_lpm       ?? 0;
  const running  = snapshot?.status.is_running          ?? false;
  const isError  = snapshot?.status.is_error            ?? false;
  const mode     = snapshot?.control.mode               ?? "AUTO";

  // ── Loading state ─────────────────────────────────────────────────────────
  if (!authChecked || !authReady) {
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
        updatedAt={snapshot?.updatedAt ?? null}
        mode={mode}
      />

      {/* ── Page header ───────────────────────────────────────────────────── */}
      <header className="px-4 sm:px-6 pt-4 sm:pt-8 pb-4">
        <div className="max-w-6xl mx-auto">
          <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
            <div className="min-w-0">
              <p className="text-[10px] sm:text-xs font-mono text-text-muted uppercase tracking-[0.2em] sm:tracking-[0.3em] mb-1">
                Deep Well Pump · 660L Storage Tank
              </p>
              <h1 className="font-display text-xl sm:text-2xl md:text-3xl font-bold text-text-primary truncate">
                Smart Water Pump{" "}
                <span className="text-gradient-cyan">System</span>
              </h1>
            </div>

            {/* User + Sign out + Pump running indicator */}
            <div className="flex flex-wrap items-center gap-2 sm:gap-3 shrink-0">
              <span className="text-xs font-mono text-text-muted hidden md:block truncate max-w-[140px]">
                {authUser?.email ?? ""}
              </span>
              <button
                onClick={() => setShowNotifications(true)}
                title="Notification settings"
                className="p-2 rounded-lg border border-surface-3 text-text-secondary
                           hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors
                           focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1"
              >
                <Bell size={18} />
              </button>
              <button
                onClick={() => signOut().then(() => window.location.assign("/login"))}
                className="px-3 py-2 min-h-[44px] sm:min-h-0 sm:py-1.5 rounded-lg border border-surface-3 text-text-secondary
                           text-xs font-mono hover:border-accent-red/40 hover:text-accent-red
                           transition-colors shrink-0 touch-manipulation
                           focus:outline-none focus:ring-2 focus:ring-accent-red/50 focus:ring-offset-2 focus:ring-offset-surface-1"
              >
                Sign out
              </button>
              {/* Pump running indicator */}
              <div className={clsx(
                "flex items-center gap-2 sm:gap-2.5 px-3 sm:px-4 py-2 rounded-xl border transition-all duration-500 min-h-[44px] sm:min-h-0",
                running && !isError
                  ? "bg-accent-green/10 border-accent-green/30 shadow-[0_0_20px_rgba(0,255,136,0.15)]"
                  : isError
                  ? "bg-accent-red/10 border-accent-red/30"
                  : "bg-surface-2 border-surface-3"
              )}>
                {running && !isError && <div className="dot-live" />}
                {isError            && <div className="dot-error" />}
                {!running && !isError && (
                  <div className="w-2 h-2 rounded-full bg-text-muted" />
                )}
                <span className={clsx(
                  "font-mono text-xs sm:text-sm font-semibold uppercase tracking-wider",
                  running && !isError ? "text-accent-green"
                  : isError           ? "text-accent-red"
                  :                     "text-text-secondary"
                )}>
                  {isError ? "ERROR" : running ? "RUNNING" : "IDLE"}
                </span>
              </div>
            </div>
          </div>
        </div>
      </header>

      {/* ── Main layout ───────────────────────────────────────────────────── */}
      <main className="flex-1 px-4 sm:px-6 pb-6 sm:pb-8 min-w-0">
        <div className="max-w-6xl mx-auto space-y-3 sm:space-y-4">

          {/* Connection error banner */}
          {error && (
            <div className="flex items-center gap-2 sm:gap-3 p-3 rounded-xl bg-accent-red/10
                            border border-accent-red/30 text-accent-red text-xs sm:text-sm font-mono">
              <AlertTriangle size={14} className="shrink-0" />
              <span className="min-w-0 break-words">Connection error — unable to reach pump. {error}</span>
            </div>
          )}

          {/* ── Row 1: Tank + Stats + Controls ─────────────────────────── */}
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3 sm:gap-4">

            {/* Tank visual */}
            <div className={clsx(
              "card p-4 sm:p-6 flex flex-col items-center justify-center gap-2",
              isError       ? "card-glow-red"
              : running     ? "card-glow-green"
              :               "card-glow-cyan"
            )}>
              <h3 className="font-display font-semibold text-sm uppercase tracking-widest
                             text-text-secondary self-start">
                Tank Level
              </h3>
              <TankVisual level={level} isRunning={running} isError={isError} />
            </div>

            {/* Stats column */}
            <div className="flex flex-col gap-2 sm:gap-3">
              <StatCard
                label="Tank Water Level"
                value={level.toString()}
                unit="%"
                Icon={Droplets}
                color={level <= 20 ? "amber" : "cyan"}
                sub={`Auto start ≤ 30% · stop ≥ 100%`}
              />
              <StatCard
                label="Flow Rate"
                value={flow.toFixed(1)}
                unit="LPM"
                Icon={Wind}
                color={running && flow < 0.5 ? "red" : "green"}
                sub={running ? (flow < 0.5 ? "⚠ Low — Dry-Run risk" : "Normal flow") : "Pump idle"}
                animate={running}
              />
              <StatCard
                label="Pump Status"
                value={isError ? "ERROR" : running ? "ON" : "OFF"}
                Icon={Activity}
                color={isError ? "red" : running ? "green" : "cyan"}
                sub={`Mode: ${mode}`}
              />
            </div>

            {/* Mode controls */}
            <div className="card p-4 sm:p-5 card-glow-cyan md:col-span-2 lg:col-span-1">
              <ModeControls
                currentMode={mode}
                isError={isError}
                onSetMode={setMode}
                onAcknowledge={acknowledgeError}
              />
            </div>
          </div>

          {/* ── Row 2: History chart ─────────────────────────────────────── */}
          <div className="card p-4 sm:p-5 card-glow-cyan min-w-0 overflow-hidden">
            <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2 mb-3 sm:mb-4">
              <div>
                <h3 className="font-display font-semibold text-sm uppercase tracking-widest
                               text-text-primary">
                  Level & Flow History
                </h3>
                <p className="text-[10px] sm:text-xs font-mono text-text-muted mt-0.5">
                  Last {history.length} readings · updated every 3s
                </p>
              </div>
              <div className="flex items-center gap-2 text-text-muted">
                <RefreshCw size={12} className={clsx(connected && "animate-spin-slow")} />
                <span className="text-xs font-mono">Real-time</span>
              </div>
            </div>
            <HistoryChart data={history} />
          </div>

          {/* ── Row 3: System info footer ────────────────────────────────── */}
          <div className="grid grid-cols-2 md:grid-cols-4 gap-2 sm:gap-3">
            {[
              { label: "Hardware",  value: "ESP32 38-pin" },
              { label: "Sensors",   value: "Ultrasonic · Flow Sensor" },
              { label: "Safety", value: "Thermal Overload · Dry-Run Protection" },
              { label: "Cloud",     value: "Realtime Sync" },
            ].map(({ label, value }) => (
              <div
                key={label}
                className="card p-3 border-surface-3"
              >
                <p className="text-[10px] font-mono text-text-muted uppercase tracking-widest">
                  {label}
                </p>
                <p className="text-xs font-mono text-text-secondary mt-1">{value}</p>
              </div>
            ))}
          </div>
        </div>
      </main>
    </div>
    {showNotifications && (
      <NotificationSettings
        userEmail={authUser?.email ?? null}
        onClose={() => setShowNotifications(false)}
      />
    )}
    </AuthGuard>
  );
}
