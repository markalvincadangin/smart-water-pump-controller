"use client";

import clsx from "clsx";
import { Activity, Droplets, Wind } from "lucide-react";
import type { DeviceConfig, PumpControl } from "@/lib/types";
import { DEFAULT_DEVICE_CONFIG } from "@/lib/types";
import TankVisual from "@/components/TankVisual";
import StatCard from "@/components/StatCard";
import ModeControls from "@/components/ModeControls";

interface DashboardMainGridProps {
  level: number;
  flow: number;
  running: boolean;
  isError: boolean;
  mode: PumpControl["mode"];
  config: DeviceConfig | null;
  isAdmin: boolean;
  pendingMode: PumpControl["mode"] | null;
  pendingAck: boolean;
  onSetMode: (mode: PumpControl["mode"]) => void;
  onAcknowledge: () => void;
}

export default function DashboardMainGrid({
  level,
  flow,
  running,
  isError,
  mode,
  config,
  isAdmin,
  pendingMode,
  pendingAck,
  onSetMode,
  onAcknowledge,
}: DashboardMainGridProps) {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3 sm:gap-4">
      <div
        className={clsx(
          "card p-4 sm:p-6 flex flex-col items-center justify-center gap-2 min-h-[200px] sm:min-h-0",
          isError ? "card-glow-red" : running ? "card-glow-green" : "card-glow-cyan"
        )}
      >
        <h3 className="font-display font-semibold text-sm uppercase tracking-widest text-text-secondary self-start">
          Tank Level
        </h3>
        <TankVisual level={level} isRunning={running} isError={isError} />
      </div>

      <div className="flex flex-col gap-2 sm:gap-3 min-w-0">
        <StatCard
          label="Tank Water Level"
          value={level.toString()}
          unit="%"
          Icon={Droplets}
          color={level <= (config?.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level) ? "amber" : "cyan"}
          sub={`Pump on at ≤${config?.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level}%, off at ≥${config?.pump_stop_level ?? DEFAULT_DEVICE_CONFIG.pump_stop_level}%`}
        />
        <StatCard
          label="Flow Rate"
          value={flow.toFixed(1)}
          unit="LPM"
          Icon={Wind}
          color={running && flow < (config?.dry_run_threshold_lpm ?? DEFAULT_DEVICE_CONFIG.dry_run_threshold_lpm) ? "red" : "green"}
          sub={running
            ? (flow < (config?.dry_run_threshold_lpm ?? DEFAULT_DEVICE_CONFIG.dry_run_threshold_lpm) ? "⚠ Low flow — pump may stop" : "Normal flow")
            : "Pump idle"}
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

      <div className="card p-4 sm:p-5 card-glow-cyan md:col-span-2 lg:col-span-1">
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
  );
}

