// components/StatusBar.tsx
"use client";

import clsx from "clsx";
import { Wifi, WifiOff, Clock, AlertTriangle, Signal, Moon } from "lucide-react";

interface StatusBarProps {
  connected: boolean;
  esp32Online: boolean;
  updatedAt: number | null;
  mode: string;
  isLevelSensorError?: boolean;
  isFlowSensorError?: boolean;
  isOverflowError?: boolean;
  isSleeping?: boolean;
  wifiRssi?: number;
  bootReason?: string;
  uptimeMinutes?: number;
  levelLastValidAgeSec?: number;
  levelSensorHealthPct?: number;
  remoteSensorStable?: boolean;
  levelFresh?: boolean;
}

function formatUptime(min: number): string {
  if (min >= 1440) return `${Math.floor(min / 1440)}d ${Math.floor((min % 1440) / 60)}h`;
  if (min >= 60) return `${Math.floor(min / 60)}h ${min % 60}m`;
  return `${min}m`;
}

function wifiStrengthLabel(rssi: number): { label: string; color: string; bars: number } {
  if (rssi >= -50) return { label: "Excellent", color: "text-accent-green", bars: 4 };
  if (rssi >= -60) return { label: "Good", color: "text-accent-green", bars: 3 };
  if (rssi >= -70) return { label: "Fair", color: "text-accent-amber", bars: 2 };
  return { label: "Weak", color: "text-accent-red", bars: 1 };
}

export default function StatusBar({ connected, esp32Online, updatedAt, mode, isLevelSensorError, isFlowSensorError, isOverflowError, isSleeping, wifiRssi, bootReason, uptimeMinutes, levelLastValidAgeSec, levelSensorHealthPct, remoteSensorStable, levelFresh }: StatusBarProps) {
  const timeAgo = updatedAt ? Math.round((Date.now() - updatedAt) / 1000) : null;
  const wifi = wifiRssi != null && wifiRssi !== 0 ? wifiStrengthLabel(wifiRssi) : null;
  const hasAnyWarning = isLevelSensorError || isFlowSensorError || isOverflowError;

  return (
    <div className="flex items-center justify-between gap-2 px-3 sm:px-4 py-1.5 sm:py-2 pt-[max(0.375rem,env(safe-area-inset-top))] pl-[max(0.75rem,env(safe-area-inset-left))] pr-[max(0.75rem,env(safe-area-inset-right))] bg-surface-1 border-b border-surface-3 min-w-0 overflow-hidden">
      {/* Left: connectivity status */}
      <div className="flex items-center gap-1.5 sm:gap-2 shrink-0 min-w-0">
        {!connected ? (
          <>
            <div className="dot-error" />
            <WifiOff size={12} className="text-accent-red" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-red">Offline</span>
          </>
        ) : esp32Online ? (
          <>
            <div className="dot-live" />
            {wifi ? (
              <Signal size={12} className={wifi.color} />
            ) : (
              <Wifi size={12} className="text-accent-green" />
            )}
            <span className="text-[10px] sm:text-xs font-mono text-accent-green"
              title={[
                bootReason ? `Boot: ${bootReason}` : null,
                wifi ? `WiFi: ${wifiRssi}dBm (${wifi.label})` : null,
                uptimeMinutes != null ? `Uptime: ${formatUptime(uptimeMinutes)}` : null,
              ].filter(Boolean).join(" · ")}>
              Online
            </span>
            {uptimeMinutes != null && (
              <span className="text-[9px] font-mono text-text-muted hidden sm:inline" title="Uptime">
                {formatUptime(uptimeMinutes)}
              </span>
            )}
            {wifi && (
              <span className={clsx("text-[9px] font-mono hidden sm:inline", wifi.color)}
                title={`WiFi ${wifiRssi}dBm`}>
                {wifiRssi}dBm
              </span>
            )}
          </>
        ) : (
          <>
            <div className="dot-error" />
            <WifiOff size={12} className="text-accent-amber" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-amber">
              Controller off{timeAgo != null ? ` ${timeAgo}s` : ""}
            </span>
          </>
        )}
      </div>

      {/* Right: mode + warnings + freshness */}
      <div className="flex items-center gap-1 sm:gap-1.5 shrink-0">
        {/* Remote link gates (non-noisy) */}
        {remoteSensorStable === false && (
          <span
            className="badge text-[9px] sm:text-[10px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20"
            title="Remote sensor link is unstable (RS-485). Starts may be blocked for safety."
          >
            Link unstable
          </span>
        )}
        {levelFresh === false && (
          <span
            className="badge text-[9px] sm:text-[10px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20"
            title="Level data is stale. Starts may be blocked for safety."
          >
            Level stale
          </span>
        )}

        {/* Stale level warning */}
        {typeof levelLastValidAgeSec === "number" && levelLastValidAgeSec > 20 && (
          <span className="badge text-[9px] sm:text-[10px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20"
            title={`Level reading is ${levelLastValidAgeSec}s old`}>
            Level {levelLastValidAgeSec}s
          </span>
        )}

        {/* Sensor health (compact) */}
        {typeof levelSensorHealthPct === "number" && levelSensorHealthPct < 100 && (
          <span
            className={clsx("inline-flex items-center gap-0.5 shrink-0",
              levelSensorHealthPct >= 80 ? "text-accent-green" : levelSensorHealthPct >= 50 ? "text-accent-amber" : "text-accent-red"
            )}
            title={`Level sensor health ${levelSensorHealthPct}%`}
          >
            <span className="w-1.5 h-1.5 rounded-full bg-current" aria-hidden />
            <span className="text-[9px] font-mono hidden sm:inline">{levelSensorHealthPct}%</span>
          </span>
        )}

        {/* Warning badges — collapsed on mobile into single icon */}
        {hasAnyWarning && (
          <span className="sm:hidden badge text-[9px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20"
            title={[
              isLevelSensorError && "Level sensor error",
              isFlowSensorError && "Flow sensor error",
              isOverflowError && "Overflow error",
            ].filter(Boolean).join(", ")}>
            <AlertTriangle size={9} />
          </span>
        )}
        {isLevelSensorError && (
          <span className="hidden sm:inline-flex badge text-[9px] sm:text-[10px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20 items-center gap-0.5">
            <AlertTriangle size={9} /> LEVEL
          </span>
        )}
        {isFlowSensorError && (
          <span className="hidden sm:inline-flex badge text-[9px] sm:text-[10px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20 items-center gap-0.5">
            <AlertTriangle size={9} /> FLOW
          </span>
        )}
        {isOverflowError && (
          <span className="hidden sm:inline-flex badge text-[9px] sm:text-[10px] bg-accent-red/10 text-accent-red border border-accent-red/20 items-center gap-0.5">
            <AlertTriangle size={9} /> OVERFLOW
          </span>
        )}

        {isSleeping && (
          <span className="badge text-[9px] sm:text-[10px] bg-surface-3 text-text-muted border border-surface-4">
            <Moon size={10} aria-hidden className="text-text-muted" />
            <span className="hidden sm:inline">Sleep</span>
          </span>
        )}

        {/* Mode badge (policy mode) */}
        <span className={clsx(
          "badge text-[9px] sm:text-[10px] font-semibold",
          mode === "AUTO" && "bg-accent-cyan/10 text-accent-cyan border border-accent-cyan/20",
          mode === "MANUAL" && "bg-accent-green/10 text-accent-green border border-accent-green/20",
          mode === "COUNTDOWN" && "bg-accent-amber/10 text-accent-amber border border-accent-amber/20"
        )}>
          {mode}
        </span>

        {/* Freshness */}
        <div className="flex items-center gap-0.5 text-text-muted" title="Last update">
          <Clock size={10} className="sm:hidden" />
          <Clock size={11} className="hidden sm:block" />
          <span className="text-[9px] sm:text-[10px] font-mono tabular-nums">
            {timeAgo != null ? (timeAgo < 5 ? "now" : `${timeAgo}s`) : "—"}
          </span>
        </div>
      </div>
    </div>
  );
}
