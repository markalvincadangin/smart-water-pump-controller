"use client";

import React, { useState, useEffect } from "react";
import clsx from "clsx";

interface PumpStatusCardProps {
  runMode: string;
  isRunning: boolean;
  flowRate: number;
  uptimeMin: number;
  bootReason: string;
  totalCycles: number;
  cooldownRemainingSec?: number;
  isLoading?: boolean;
  /** The mode requested via control node — shows pending if different from runMode category. */
  requestedMode?: "AUTO" | "MANUAL" | "COUNTDOWN";
}

const MODE_LABELS: Record<string, { label: string; chipClass: string; dotClass: string }> = {
  'AUTO_STANDBY':    { label: 'AUTO — Standby',        chipClass: 'text-[var(--text-secondary)] border-[var(--card-border)]', dotClass: 'bg-[var(--text-muted)]' },
  'AUTO':            { label: 'AUTO — Running',        chipClass: 'text-sf-teal border-sf-teal/30 bg-sf-teal/5', dotClass: 'bg-sf-teal animate-pulse' },
  'AUTO_COOLDOWN':   { label: 'AUTO — Cooldown',       chipClass: 'text-sf-blue border-sf-blue/30 bg-sf-blue/5', dotClass: 'bg-sf-blue' },
  'MANUAL_ON':       { label: 'MANUAL — On',           chipClass: 'text-sf-teal border-sf-teal/30 bg-sf-teal/5', dotClass: 'bg-sf-teal animate-pulse' },
  'MANUAL_OFF':      { label: 'MANUAL — Off',          chipClass: 'text-[var(--text-secondary)] border-[var(--card-border)]', dotClass: 'bg-[var(--text-muted)]' },
  'MANUAL_COOLDOWN': { label: 'MANUAL — Cooldown',     chipClass: 'text-sf-blue border-sf-blue/30 bg-sf-blue/5', dotClass: 'bg-sf-blue' },
  'COUNTDOWN':       { label: 'Countdown Active',      chipClass: 'text-sf-blue border-sf-blue/30 bg-sf-blue/5', dotClass: 'bg-sf-blue animate-pulse' },
  'STOPPED':         { label: 'Emergency Stop',        chipClass: 'text-sf-red border-sf-red/30 bg-sf-red/5', dotClass: 'bg-sf-red animate-pulse' },
};

/**
 * REFACTOR [D4.3]: Pump Status & Metrics
 * Real-time pump diagnostics and run-mode feedback with cooldown tracking.
 */
export default function PumpStatusCard({
  runMode,
  isRunning,
  flowRate,
  uptimeMin,
  bootReason,
  totalCycles,
  cooldownRemainingSec = 0,
  isLoading = false,
  requestedMode,
}: PumpStatusCardProps) {
  const [localCooldown, setLocalCooldown] = useState(cooldownRemainingSec);

  // Derive the broad category of the current status run_mode
  const runModeCategory: "AUTO" | "MANUAL" | "COUNTDOWN" | null =
    runMode.startsWith("MANUAL") ? "MANUAL" :
    runMode.startsWith("COUNTDOWN") ? "COUNTDOWN" :
    (runMode.startsWith("AUTO") || runMode === "AUTO_STANDBY") ? "AUTO" : null;

  // True when the requested mode (from control node) doesn't match actual hardware state
  const isPendingModeChange = !!requestedMode && runModeCategory !== null && requestedMode !== runModeCategory;

  useEffect(() => {
    setLocalCooldown(cooldownRemainingSec);
  }, [cooldownRemainingSec]);

  useEffect(() => {
    if (localCooldown <= 0) return;
    
    const timer = setInterval(() => {
      setLocalCooldown(prev => Math.max(0, prev - 1));
    }, 1000);
    
    return () => clearInterval(timer);
  }, [localCooldown]);

  if (isLoading) {
    return (
      <div className="card p-6 flex flex-col gap-4 animate-pulse min-h-[280px]">
        <div className="h-4 w-24 skeleton" />
        <div className="h-8 w-48 skeleton rounded-chip mt-2" />
        <div className="space-y-3 mt-4">
          {[...Array(4)].map((_, i) => (
            <div key={i} className="flex justify-between">
              <div className="h-4 w-20 skeleton" />
              <div className="h-4 w-12 skeleton" />
            </div>
          ))}
        </div>
      </div>
    );
  }

  const activeMode = MODE_LABELS[runMode] || { label: runMode, chipClass: 'text-[var(--text-muted)] border-[var(--card-border)]', dotClass: 'bg-[var(--text-muted)]' };
  const showCooldown = runMode.endsWith('_COOLDOWN') && localCooldown > 0;

  const formatUptime = (min: number) => {
    if (min < 60) return `${min}m`;
    const h = Math.floor(min / 60);
    const m = min % 60;
    return `${h}h ${m}m`;
  };

  return (
    <div className="card p-6 flex flex-col gap-6">
      <h3 className="text-sm font-semibold uppercase tracking-wider text-[var(--text-muted)] self-start">Pump Status</h3>

      {/* Mode Chip */}
      <div 
        className={clsx(
          "border rounded-full px-3 py-1.5 inline-flex items-center gap-2 self-start transition-colors duration-300 text-sm font-medium",
          activeMode.chipClass
        )}
        role="status"
        aria-live="polite"
      >
        <div className={clsx("w-2 h-2 rounded-full", activeMode.dotClass)} />
        {activeMode.label}
        {showCooldown && (
          <span className="ml-1 font-mono opacity-80 border-l border-current pl-2 text-xs">
            {localCooldown}s
          </span>
        )}
      </div>

      {/* Pending mode indicator — shows while ESP32 is acknowledging the mode change */}
      {isPendingModeChange && (
        <div className="inline-flex items-center gap-1.5 text-[10px] font-mono text-[var(--text-muted)] animate-pulse">
          <div className="w-1.5 h-1.5 rounded-full bg-sf-blue animate-ping" />
          applying {requestedMode?.toLowerCase()} mode…
        </div>
      )}

      {/* Metrics Grid */}
      <div className="grid grid-cols-1 gap-y-3 mt-auto">
        <MetricRow 
          label="Flow Indicator" 
          value={isRunning ? `${flowRate.toFixed(2)} LPM` : "—"} 
          isHighlighted={isRunning && flowRate > 0}
        />
        <MetricRow 
          label="System Uptime" 
          value={formatUptime(uptimeMin)} 
        />
        <MetricRow 
          label="Boot Reason" 
          value={bootReason} 
        />
        <MetricRow 
          label="Total Cycles" 
          value={totalCycles.toLocaleString()} 
        />
      </div>
    </div>
  );
}

function MetricRow({ label, value, isHighlighted }: { label: string; value: string; isHighlighted?: boolean }) {
  return (
    <div className="flex items-end justify-between group border-b border-[var(--card-border)] border-dashed pb-2 last:border-0 last:pb-0">
      <span className="text-xs font-medium text-[var(--text-secondary)]">
        {label}
      </span>
      <span className={clsx(
        "font-mono text-sm font-semibold transition-colors tabular-nums tracking-tight",
        isHighlighted ? "text-sf-teal" : "text-[var(--text-primary)]"
      )}>
        {value}
      </span>
    </div>
  );
}
