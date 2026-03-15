"use client";

import clsx from "clsx";
import { Activity, Droplets, Wind } from "lucide-react";
import type { DeviceConfig, PumpControl } from "@/lib/types";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import TankVisual from "@/components/TankVisual";
import StatCard from "@/components/StatCard";
import ModeControls from "@/components/ModeControls";
import RunControls from "@/components/RunControls";
import type { PumpStatus } from "@/lib/types";

interface DashboardMainGridProps {
  level: number;
  flow: number;
  running: boolean;
  isError: boolean;
  mode: PumpControl["mode"];
  status?: PumpStatus | null;
  config: DeviceConfig | null;
  isAdmin: boolean;
  esp32Online: boolean;
  pendingMode: PumpControl["mode"] | null;
  pendingAck: boolean;
  onSetMode: (mode: PumpControl["mode"]) => void;
  onAcknowledge: () => void;
  onStartManualRun: () => void;
  onStartCountdown: (durationMin: number) => void;
  onAddCountdownTime: (addMinutes: number) => void;
  isAddingCountdownTime?: boolean;
  onStopRun: () => void;
}

export default function DashboardMainGrid({
  level,
  flow,
  running,
  isError,
  mode,
  status = null,
  config,
  isAdmin,
  esp32Online,
  pendingMode,
  pendingAck,
  onSetMode,
  onAcknowledge,
  onStartManualRun,
  onStartCountdown,
  onAddCountdownTime,
  isAddingCountdownTime = false,
  onStopRun,
}: DashboardMainGridProps) {
  const runMode = status?.run_mode ?? (running ? "AUTO" : "OFF");
  const remainingSec = status?.countdown_remaining_sec ?? 0;

  const levelEstimateActive = status?.level_estimate_active ?? false;
  const displayLevel =
    levelEstimateActive && status?.estimated_level_pct != null && status.estimated_level_pct >= 0
      ? Math.round(status.estimated_level_pct)
      : level;

  const startLevel = config?.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level;
  const stopLevel = config?.pump_stop_level ?? DEFAULT_DEVICE_CONFIG.pump_stop_level;

  const dryRunThreshold = config?.dry_run_threshold_lpm ?? DEFAULT_DEVICE_CONFIG.dry_run_threshold_lpm;
  const flowColor = running && flow < dryRunThreshold ? "red" : "green";

  const pumpLabel = isError ? "ERR" : running ? "ON" : "OFF";
  const pumpColorMobile = isError ? "text-accent-red" : running ? "text-accent-green" : "text-text-secondary";

  return (
    <div className="grid grid-cols-1 md:grid-cols-[1.4fr_0.8fr_1fr] gap-3 sm:gap-4">
      {/* Col 1: Tank */}
      <div
        className={clsx(
          "card p-4 sm:p-6 flex flex-col items-center justify-center gap-2 min-h-[260px] md:min-h-0",
          isError ? "card-glow-red" : running ? "card-glow-green" : ""
        )}
      >
        <h3 className="font-display font-semibold text-xs sm:text-sm uppercase tracking-widest text-text-secondary self-start">
          Tank Level
        </h3>
        <TankVisual
          level={displayLevel}
          isRunning={running}
          isError={isError}
          levelEstimateActive={levelEstimateActive}
          pumpStartLevel={startLevel}
          pumpStopLevel={stopLevel}
        />
      </div>

      {/* Col 2: Stats */}
      <div className="flex flex-col gap-2 sm:gap-3 min-w-0">
        {/* Mobile: horizontal stat strip */}
        <div className="grid grid-cols-3 gap-2 md:hidden">
          <div className="flex flex-col items-center justify-center min-w-0 p-2.5 rounded-xl bg-surface-2 border border-surface-3">
            <span className="text-[9px] font-mono text-text-muted uppercase">Level</span>
            <span className={clsx(
              "text-lg font-display font-bold tabular-nums leading-tight",
              levelEstimateActive ? "text-accent-amber" : displayLevel <= startLevel ? "text-accent-amber" : "text-accent-cyan"
            )}>
              {levelEstimateActive ? `~${displayLevel}` : displayLevel}%
            </span>
          </div>
          <div className="flex flex-col items-center justify-center min-w-0 p-2.5 rounded-xl bg-surface-2 border border-surface-3">
            <span className="text-[9px] font-mono text-text-muted uppercase">Flow</span>
            <span className={clsx(
              "text-lg font-display font-bold tabular-nums leading-tight",
              flowColor === "red" ? "text-accent-red" : "text-accent-green"
            )}>
              {flow.toFixed(1)}
            </span>
            <span className="text-[8px] font-mono text-text-muted">L/min</span>
          </div>
          <div className="flex flex-col items-center justify-center min-w-0 p-2.5 rounded-xl bg-surface-2 border border-surface-3">
            <span className="text-[9px] font-mono text-text-muted uppercase">Pump</span>
            <span className={clsx("text-lg font-display font-bold tabular-nums leading-tight", pumpColorMobile)}>
              {pumpLabel}
            </span>
            {runMode === "COUNTDOWN" && remainingSec > 0 && (
              <span className="text-[8px] font-mono text-accent-amber tabular-nums">
                {Math.floor(remainingSec / 60)}:{(remainingSec % 60).toString().padStart(2, "0")}
              </span>
            )}
          </div>
        </div>

        {/* Desktop: vertical stat cards */}
        <div className="hidden md:flex flex-col gap-2 sm:gap-3 min-w-0">
          <StatCard
            label="Tank Level"
            value={levelEstimateActive ? `~${displayLevel}` : displayLevel.toString()}
            unit="%"
            Icon={Droplets}
            color={levelEstimateActive ? "amber" : (displayLevel <= startLevel ? "amber" : "cyan")}
            sub={levelEstimateActive
              ? "Estimated from flow (±5%)"
              : `Start ≤${startLevel}% · Stop ≥${stopLevel}%`}
          />
          <StatCard
            label="Flow Rate"
            value={flow.toFixed(1)}
            unit="L/min"
            Icon={Wind}
            color={flowColor}
            sub={running
              ? (flow < dryRunThreshold ? "⚠ Low flow" : "Normal flow")
              : "Pump idle"}
            animate={running}
          />
          <StatCard
            label="Pump"
            value={isError ? "ERROR" : running ? "ON" : "OFF"}
            Icon={Activity}
            color={isError ? "red" : running ? "green" : "cyan"}
            sub={`${mode} · ${runMode === "AUTO_STANDBY" ? "Standby" : runMode}`}
          />
        </div>
      </div>

      {/* Col 3: Controls */}
      <div className="card p-4 sm:p-5 space-y-4">
        <RunControls
          runMode={runMode as "OFF" | "AUTO" | "AUTO_STANDBY" | "MANUAL" | "COUNTDOWN"}
          remainingSec={remainingSec}
          controlMode={mode}
          isError={isError}
          isOverflowError={status?.is_overflow_error}
          isAdmin={isAdmin}
          esp32Online={esp32Online}
          pendingAck={pendingAck}
          onAcknowledge={onAcknowledge}
          isAddingCountdownTime={isAddingCountdownTime}
          onStartManual={onStartManualRun}
          onStartCountdown={onStartCountdown}
          onAddCountdownTime={onAddCountdownTime}
          onStop={onStopRun}
        />
        <div className="border-t border-surface-3 pt-4">
          <ModeControls
            currentMode={mode}
            isError={isError}
            dryRunTimeoutSec={config?.dry_run_timeout_sec ?? DEFAULT_DEVICE_CONFIG.dry_run_timeout_sec}
            pendingMode={pendingMode}
            pendingAcknowledge={pendingAck}
            allowForceOn={isAdmin}
            onSetMode={onSetMode}
            onAcknowledge={onAcknowledge}
          />
        </div>
      </div>
    </div>
  );
}
