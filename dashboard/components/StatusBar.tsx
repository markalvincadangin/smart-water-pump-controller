// components/StatusBar.tsx
"use client";

import clsx from "clsx";
import { Wifi, WifiOff, Clock, AlertTriangle } from "lucide-react";

interface StatusBarProps {
  connected: boolean;
  esp32Online: boolean;
  updatedAt: number | null;
  mode: string;
  isSensorError?: boolean;
  isOverflowError?: boolean;
  isSleeping?: boolean;  // Phase 3: scheduled sleep active
  wifiRssi?: number;
  bootReason?: string;
  uptimeMinutes?: number; // Phase 5: Uptime
}

export default function StatusBar({ connected, esp32Online, updatedAt, mode, isSensorError, isOverflowError, isSleeping, wifiRssi, bootReason, uptimeMinutes }: StatusBarProps) {
  const timeAgo = updatedAt
    ? Math.round((Date.now() - updatedAt) / 1000)
    : null;

  return (
    <div className="flex items-center justify-between gap-1 sm:gap-2 px-3 sm:px-4 py-2 pt-[max(0.5rem,env(safe-area-inset-top))] pl-[max(0.75rem,env(safe-area-inset-left))] pr-[max(0.75rem,env(safe-area-inset-right))] bg-surface-1 border-b border-surface-3 min-w-0 overflow-hidden">
      {/* Left: ESP32 online/offline (stale = no update in 15s) */}
      <div className="flex items-center gap-1 sm:gap-2 shrink-0 min-w-0">
        {!connected ? (
          <>
            <div className="dot-error" />
            <WifiOff size={12} className="sm:w-[13px] sm:h-[13px] text-accent-red" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-red hidden sm:inline">DISCONNECTED</span>
            <span className="text-[10px] font-mono text-accent-red sm:hidden">OFF</span>
          </>
        ) : esp32Online ? (
          <>
            <div className="dot-live" />
            <Wifi size={12} className="sm:w-[13px] sm:h-[13px] text-accent-green" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-green"
              title={bootReason ? `Boot: ${bootReason}` : undefined}>
              ESP32 online
            </span>
            {uptimeMinutes !== undefined && (
              <span className="text-[9px] font-mono text-text-muted ml-0.5 sm:ml-1 hidden sm:inline"
                title="Uptime">
                up {uptimeMinutes >= 1440
                  ? `${Math.floor(uptimeMinutes / 1440)}d ${Math.floor((uptimeMinutes % 1440) / 60)}h`
                  : uptimeMinutes >= 60
                    ? `${Math.floor(uptimeMinutes / 60)}h ${uptimeMinutes % 60}m`
                    : `${uptimeMinutes}m`}
              </span>
            )}
            {wifiRssi !== undefined && wifiRssi !== 0 && (
              <span className={clsx(
                "text-[9px] font-mono ml-1 hidden sm:inline",
                wifiRssi >= -60 ? "text-accent-green" :
                  wifiRssi >= -75 ? "text-accent-amber" : "text-accent-red"
              )}>
                {wifiRssi}dBm
              </span>
            )}
          </>
        ) : (
          <>
            <div className="dot-error" />
            <WifiOff size={12} className="sm:w-[13px] sm:h-[13px] text-accent-amber" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-amber hidden sm:inline">
              ESP32 offline
              {timeAgo !== null ? ` (${timeAgo}s)` : ""}
            </span>
            <span className="text-[10px] font-mono text-accent-amber sm:hidden">
              Offline{timeAgo !== null ? ` ${timeAgo}s` : ""}
            </span>
          </>
        )}
      </div>

      {/* Center: title — full name on sm+, abbreviation on mobile */}
      <span className="text-[10px] sm:text-xs font-mono text-text-secondary tracking-widest uppercase truncate hidden sm:block text-center min-w-0 mx-1 flex-1">
        Smart Water Pump System
      </span>
      <span className="text-[10px] font-mono text-text-secondary tracking-widest uppercase sm:hidden shrink-0 truncate max-w-[80px]">
        Pump
      </span>

      {/* Right: last update */}
      <div className="flex items-center gap-1 sm:gap-2 text-text-muted shrink-0 flex-wrap justify-end">
        <Clock size={11} className="sm:w-3 sm:h-3" />
        <span className="text-[10px] sm:text-xs font-mono tabular-nums">
          {timeAgo !== null
            ? timeAgo < 5
              ? "Just now"
              : `${timeAgo}s`
            : "—"}
        </span>
        <span className={clsx(
          "badge text-[9px] sm:text-[10px] ml-0.5 sm:ml-1 shrink-0",
          mode === "AUTO" && "bg-accent-cyan/10 text-accent-cyan border border-accent-cyan/20",
          mode === "FORCE_ON" && "bg-accent-green/10 text-accent-green border border-accent-green/20",
          mode === "FORCE_OFF" && "bg-accent-red/10 text-accent-red border border-accent-red/20"
        )}>
          {mode}
        </span>
        {isSensorError && (
          <span className="badge text-[9px] sm:text-[10px] ml-0.5 sm:ml-1 shrink-0 bg-accent-amber/10 text-accent-amber border border-accent-amber/20 flex items-center gap-0.5">
            <AlertTriangle size={9} />
            <span className="hidden sm:inline">SENSOR</span>
          </span>
        )}
        {isOverflowError && (
          <span className="badge text-[9px] sm:text-[10px] ml-0.5 sm:ml-1 shrink-0 bg-accent-red/10 text-accent-red border border-accent-red/20 flex items-center gap-0.5">
            <AlertTriangle size={9} />
            <span className="hidden sm:inline">OVERFLOW</span>
          </span>
        )}
        {isSleeping && (
          <span className="badge text-[9px] sm:text-[10px] ml-0.5 sm:ml-1 shrink-0 bg-surface-3 text-text-muted border border-surface-4 flex items-center gap-0.5">
            <span aria-hidden>😴</span>
            <span className="hidden sm:inline">Sleep</span>
          </span>
        )}
      </div>
    </div>
  );
}
