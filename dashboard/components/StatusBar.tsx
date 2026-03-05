// components/StatusBar.tsx
"use client";

import clsx from "clsx";
import { Wifi, WifiOff, Clock } from "lucide-react";

interface StatusBarProps {
  connected: boolean;
  updatedAt: number | null;
  mode:      string;
}

export default function StatusBar({ connected, updatedAt, mode }: StatusBarProps) {
  const timeAgo = updatedAt
    ? Math.round((Date.now() - updatedAt) / 1000)
    : null;

  return (
    <div className="flex items-center justify-between gap-2 px-3 sm:px-4 py-2 bg-surface-1 border-b border-surface-3 min-w-0">
      {/* Left: connection */}
      <div className="flex items-center gap-1.5 sm:gap-2 shrink-0">
        {connected ? (
          <>
            <div className="dot-live" />
            <Wifi size={12} className="sm:w-[13px] sm:h-[13px] text-accent-green" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-green">LIVE</span>
          </>
        ) : (
          <>
            <div className="dot-error" />
            <WifiOff size={12} className="sm:w-[13px] sm:h-[13px] text-accent-red" />
            <span className="text-[10px] sm:text-xs font-mono text-accent-red hidden sm:inline">DISCONNECTED</span>
            <span className="text-[10px] font-mono text-accent-red sm:hidden">OFF</span>
          </>
        )}
      </div>

      {/* Center: title — full name on sm+, abbreviation on mobile */}
      <span className="text-[10px] sm:text-xs font-mono text-text-secondary tracking-widest uppercase truncate hidden sm:block text-center min-w-0 mx-1">
        Smart Water Pump System
      </span>
      <span className="text-[10px] font-mono text-text-secondary tracking-widest uppercase sm:hidden shrink-0">
        Pump System
      </span>

      {/* Right: last update */}
      <div className="flex items-center gap-1.5 sm:gap-2 text-text-muted shrink-0">
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
          mode === "AUTO"      && "bg-accent-cyan/10 text-accent-cyan border border-accent-cyan/20",
          mode === "FORCE_ON"  && "bg-accent-green/10 text-accent-green border border-accent-green/20",
          mode === "FORCE_OFF" && "bg-accent-red/10 text-accent-red border border-accent-red/20"
        )}>
          {mode}
        </span>
      </div>
    </div>
  );
}
