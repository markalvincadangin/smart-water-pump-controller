"use client";

import clsx from "clsx";
import { Activity, Droplets, Wind } from "lucide-react";
import type { DeviceConfig, PumpControl } from "@/lib/types";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import TankVisual from "@/components/TankVisual";
import StatCard from "@/components/StatCard";
import ModeControls from "@/components/ModeControls";
import RunControls from "@/components/RunControls";
import FlowStrip from "@/components/FlowStrip";
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
  onSetManualDesired: (on: boolean) => void;
  onStartCountdown: (durationMin: number) => void;
  onAddCountdownTime: (addMinutes: number) => void;
  isAddingCountdownTime?: boolean;
  onStopCountdown: () => void;
  onEmergencyStop: () => void;
  onResetStop: () => void;
  manualDesired: boolean;
  emergencyStopLatched: boolean;
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
  onSetManualDesired,
  onStartCountdown,
  onAddCountdownTime,
  isAddingCountdownTime = false,
  onStopCountdown,
  onEmergencyStop,
  onResetStop,
  manualDesired,
  emergencyStopLatched,
}: DashboardMainGridProps) {
  const runMode = status?.run_mode ?? (running ? "AUTO" : "OFF") as string;
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

  const levelFresh = status?.level_fresh;

  return (
    <div className="flex min-w-0 flex-col gap-3">
      {/* Mobile: horizontal stat strip (Part 6.4 status rail density) */}
      <div className="grid grid-cols-3 gap-2 lg:hidden">
        <div className="flex min-w-0 flex-col items-center justify-center rounded-lg border border-border-subtle bg-surface-1 p-3">
          <span className="font-mono text-xs text-text-unit uppercase tracking-wide">Level</span>
          <span
            className={clsx(
              "mt-1 font-mono text-metric font-semibold tabular-nums leading-none",
              levelEstimateActive
                ? "text-accent-amber"
                : displayLevel <= startLevel
                  ? "text-accent-amber"
                  : "text-accent-cyan"
            )}
          >
            {(levelEstimateActive || (status?.level_last_valid_age_sec ?? 0) > 300) ? `~${displayLevel}` : displayLevel}
            <span className="text-xs text-text-unit">%</span>
          </span>
        </div>
        <div className="flex min-w-0 flex-col items-center justify-center rounded-lg border border-border-subtle bg-surface-1 p-3">
          <span className="font-mono text-xs text-text-unit uppercase tracking-wide">Flow</span>
          <span
            className={clsx(
              "mt-1 font-mono text-metric font-semibold tabular-nums leading-none",
              flowColor === "red" ? "text-accent-red" : "text-accent-green"
            )}
          >
            {flow.toFixed(1)}
          </span>
          <span className="font-mono text-xs text-text-unit">L/min</span>
        </div>
        <div className="flex min-w-0 flex-col items-center justify-center rounded-lg border border-border-subtle bg-surface-1 p-3">
          <span className="font-mono text-xs text-text-unit uppercase tracking-wide">Pump</span>
          <span className={clsx("mt-1 font-mono text-metric font-semibold tabular-nums leading-none", pumpColorMobile)}>
            {pumpLabel}
          </span>
          {runMode === "COUNTDOWN" && remainingSec > 0 && (
            <span className="mt-0.5 font-mono text-xs text-accent-amber tabular-nums">
              {Math.floor(remainingSec / 60)}:{(remainingSec % 60).toString().padStart(2, "0")}
            </span>
          )}
        </div>
      </div>

      {/* Part 6.2 — desktop: 220px · 1fr · 300px, 1px faint gutters */}
      <div className="grid grid-cols-1 gap-3 lg:grid-cols-[220px_1fr_300px] lg:gap-px lg:overflow-hidden lg:rounded-xl lg:bg-border-faint">
        {/* Left rail — system metrics */}
        <aside className="hidden min-h-0 flex-col lg:flex">
          <div className="flex min-h-0 flex-1 flex-col gap-3 overflow-y-auto bg-surface-1 p-6">
            <StatCard
              label="Tank Level"
              value={levelEstimateActive ? `~${displayLevel}` : displayLevel.toString()}
              unit="%"
              Icon={Droplets}
              color={levelEstimateActive ? "amber" : displayLevel <= startLevel ? "amber" : "cyan"}
              sub={
                levelEstimateActive
                  ? "Estimated from flow (±5%)"
                  : `Start ≤${startLevel}% · Stop ≥${stopLevel}%`
              }
            />
            <StatCard
              label="Flow Rate"
              value={flow.toFixed(1)}
              unit="L/min"
              Icon={Wind}
              color={flowColor}
              sub={
                running
                  ? flow < dryRunThreshold
                    ? "Low flow"
                    : "Normal flow"
                  : "Pump idle"
              }
              animate={running}
              proximity={
                running && dryRunThreshold > 0 ? Math.min(1.5, flow / dryRunThreshold) : undefined
              }
              proximityLabel={
                running && dryRunThreshold > 0
                  ? flow < dryRunThreshold
                    ? "Below dry-run threshold"
                    : flow < dryRunThreshold * 2
                      ? "Near dry-run threshold"
                      : "Well above threshold"
                  : undefined
              }
            />
            <StatCard
              label="Pump"
              value={isError ? "ERROR" : running ? "ON" : "OFF"}
              Icon={Activity}
              color={isError ? "red" : running ? "green" : "cyan"}
              sub={`${mode} · ${runMode === "AUTO_STANDBY" ? "Standby" : runMode === "MANUAL_OFF" ? "Off" : runMode}`}
            />
          </div>
        </aside>

        {/* Center — tank hero + flow strip */}
        <div className="flex min-w-0 flex-col gap-px">
          <div
            className={clsx(
              "panel-tank-glass flex min-h-[280px] flex-col items-center justify-center gap-4 p-6 md:min-h-0",
              isError ? "card-glow-red" : running ? "card-glow-green" : "",
              levelFresh === false && "border-l-4 border-l-accent-amber"
            )}
          >
            <h3 className="section-label self-start text-text-secondary">Tank Level</h3>
            <TankVisual
              level={displayLevel}
              isRunning={running}
              isError={isError}
              levelEstimateActive={levelEstimateActive}
              pumpStartLevel={startLevel}
              pumpStopLevel={stopLevel}
              levelLastValidAgeSec={status?.level_last_valid_age_sec}
              levelFresh={levelFresh}
            />
          </div>
          <FlowStrip flowLpm={flow} running={running} dryRunThresholdLpm={dryRunThreshold} />
        </div>

        {/* Right — operator controls */}
        <div className="bg-surface-1 p-6" id="run-mode-section">
          <RunControls
            runMode={runMode as "OFF" | "AUTO" | "AUTO_STANDBY" | "MANUAL_ON" | "MANUAL_OFF" | "COUNTDOWN" | "STOPPED"}
            remainingSec={remainingSec}
            controlMode={mode}
            isError={isError}
            isOverflowError={status?.is_overflow_error}
            isAdmin={isAdmin}
            esp32Online={esp32Online}
            pendingAck={pendingAck}
            onAcknowledge={onAcknowledge}
            isAddingCountdownTime={isAddingCountdownTime}
            manualDesired={manualDesired}
            emergencyStopLatched={emergencyStopLatched}
            onSetManualDesired={onSetManualDesired}
            onStartCountdown={onStartCountdown}
            onAddCountdownTime={onAddCountdownTime}
            onStopCountdown={onStopCountdown}
            onEmergencyStop={onEmergencyStop}
            onResetStop={onResetStop}
          />
          <div className="mt-6 border-t border-border-faint pt-6">
            <ModeControls
              currentMode={mode}
              isError={isError}
              dryRunTimeoutSec={config?.dry_run_timeout_sec ?? DEFAULT_DEVICE_CONFIG.dry_run_timeout_sec}
              pendingMode={pendingMode}
              pendingAcknowledge={pendingAck}
              onSetMode={onSetMode}
              onAcknowledge={onAcknowledge}
              controlsLocked={emergencyStopLatched}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
