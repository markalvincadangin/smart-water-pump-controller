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
}

const MODE_LABELS: Record<string, { label: string; chipClass: string }> = {
  'AUTO_STANDBY':    { label: 'AUTO — Standby',       chipClass: 'bg-sf-gray-50 text-sf-gray-600 border-sf-gray-100' },
  'AUTO':            { label: 'AUTO — Running',        chipClass: 'bg-sf-teal-light text-sf-teal border-sf-teal/20' },
  'AUTO_COOLDOWN':   { label: 'AUTO — Cooldown',       chipClass: 'bg-sf-blue-light text-sf-blue-mid border-sf-blue/20' },
  'MANUAL_ON':       { label: 'MANUAL — On',           chipClass: 'bg-sf-teal-light text-sf-teal border-sf-teal/20' },
  'MANUAL_OFF':      { label: 'MANUAL — Off',          chipClass: 'bg-sf-gray-50 text-sf-gray-600 border-sf-gray-100' },
  'MANUAL_COOLDOWN': { label: 'MANUAL — Cooldown',     chipClass: 'bg-sf-blue-light text-sf-blue-mid border-sf-blue/20' },
  'COUNTDOWN':       { label: 'Countdown',             chipClass: 'bg-sf-blue-light text-sf-blue border-sf-blue/20' },
  'STOPPED':         { label: 'Emergency Stop',        chipClass: 'bg-sf-red-light text-sf-red border-sf-red/20 outline outline-sf-red/30' },
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
}: PumpStatusCardProps) {
  const [localCooldown, setLocalCooldown] = useState(cooldownRemainingSec);

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

  const activeMode = MODE_LABELS[runMode] || { label: runMode, chipClass: 'bg-sf-gray-50 text-sf-gray-400' };
  const showCooldown = runMode.endsWith('_COOLDOWN') && localCooldown > 0;

  const formatUptime = (min: number) => {
    if (min < 60) return `${min}m`;
    const h = Math.floor(min / 60);
    const m = min % 60;
    return `${h}h ${m}m`;
  };

  return (
    <div className="card p-6 flex flex-col gap-6">
      <h3 className="card-header">Pump Status</h3>

      {/* Mode Chip */}
      <div 
        className={clsx(
          "status-chip border transition-all duration-300 transform scale-105 origin-left px-3 py-1.5 inline-flex",
          activeMode.chipClass
        )}
        role="status"
        aria-live="polite"
      >
        {activeMode.label}
        {showCooldown && (
          <span className="ml-1.5 font-mono opacity-80 border-l border-current pl-1.5">
            {localCooldown}s
          </span>
        )}
      </div>

      {/* Metrics Grid */}
      <div className="grid grid-cols-1 gap-y-4 mt-2">
        <MetricRow 
          label="Flow" 
          value={isRunning ? `${flowRate.toFixed(2)} LPM` : "— LPM"} 
          isHighlighted={isRunning && flowRate > 0}
        />
        <MetricRow 
          label="Uptime" 
          value={formatUptime(uptimeMin)} 
        />
        <MetricRow 
          label="Boot" 
          value={bootReason} 
        />
        <MetricRow 
          label="Cycles" 
          value={totalCycles.toLocaleString()} 
        />
      </div>
    </div>
  );
}

function MetricRow({ label, value, isHighlighted }: { label: string; value: string; isHighlighted?: boolean }) {
  return (
    <div className="flex items-center justify-between group transition-colors">
      <span className="text-xs font-mono font-medium text-[var(--text-muted)] uppercase tracking-wider">
        {label}
      </span>
      <span className={clsx(
        "font-mono text-sm sm:text-base font-semibold transition-colors tabular-nums",
        isHighlighted ? "text-sf-teal" : "text-[var(--text-primary)]"
      )}>
        {value}
      </span>
    </div>
  );
}
