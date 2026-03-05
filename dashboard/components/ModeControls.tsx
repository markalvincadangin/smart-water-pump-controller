// components/ModeControls.tsx
"use client";

import clsx from "clsx";
import { Zap, ZapOff, Cpu } from "lucide-react";
import type { PumpControl } from "@/lib/types";

interface ModeControlsProps {
  currentMode: PumpControl["mode"];
  isError: boolean;
  onSetMode: (mode: PumpControl["mode"]) => void;
  onAcknowledge: () => void;
}

const MODES: {
  id:      PumpControl["mode"];
  label:   string;
  sub:     string;
  Icon:    React.ElementType;
  active:  string;
  hover:   string;
}[] = [
  {
    id:     "AUTO",
    label:  "AUTO",
    sub:    "Level-based auto control",
    Icon:   Cpu,
    active: "bg-accent-cyan/15 border-accent-cyan/50 text-accent-cyan shadow-[0_0_20px_rgba(0,229,255,0.2)]",
    hover:  "hover:bg-accent-cyan/8 hover:border-accent-cyan/30",
  },
  {
    id:     "FORCE_ON",
    label:  "FORCE ON",
    sub:    "Manual — pump on",
    Icon:   Zap,
    active: "bg-accent-green/15 border-accent-green/50 text-accent-green shadow-[0_0_20px_rgba(0,255,136,0.2)]",
    hover:  "hover:bg-accent-green/8 hover:border-accent-green/30",
  },
  {
    id:     "FORCE_OFF",
    label:  "FORCE OFF",
    sub:    "Manual — pump off",
    Icon:   ZapOff,
    active: "bg-accent-red/15 border-accent-red/50 text-accent-red shadow-[0_0_20px_rgba(255,59,92,0.2)]",
    hover:  "hover:bg-accent-red/8 hover:border-accent-red/30",
  },
];

export default function ModeControls({
  currentMode,
  isError,
  onSetMode,
  onAcknowledge,
}: ModeControlsProps) {
  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between mb-4">
        <h3 className="font-display font-semibold text-text-primary text-sm uppercase tracking-widest">
          Mode Control
        </h3>
        <span className="badge bg-surface-3 text-text-secondary border border-surface-4" title="Remote control via cloud">
          Cloud → Device
        </span>
      </div>

      {/* Mode buttons — touch-friendly min height on mobile */}
      <div className="grid grid-cols-3 gap-2">
        {MODES.map(({ id, label, sub, Icon, active, hover }) => {
          const isActive = currentMode === id;
          return (
            <button
              key={id}
              onClick={() => onSetMode(id)}
              title={`${label}: ${sub}`}
              className={clsx(
                "flex flex-col items-center justify-center gap-1.5 sm:gap-2 p-3 sm:p-3 min-h-[72px] sm:min-h-0 rounded-xl border transition-all duration-200",
                "cursor-pointer select-none touch-manipulation active:scale-[0.98] focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1",
                isActive
                  ? active
                  : clsx("border-surface-3 text-text-secondary", hover)
              )}
            >
              <Icon
                size={18}
                className={clsx(
                  "transition-all",
                  isActive && id === "FORCE_ON" && "drop-shadow-[0_0_6px_rgba(0,255,136,0.8)]",
                  isActive && id === "AUTO"     && "drop-shadow-[0_0_6px_rgba(0,229,255,0.8)]",
                  isActive && id === "FORCE_OFF"&& "drop-shadow-[0_0_6px_rgba(255,59,92,0.8)]"
                )}
              />
              <div className="text-center min-w-0">
                <div className="text-[10px] sm:text-xs font-mono font-semibold leading-none">{label}</div>
                <div className="text-[9px] sm:text-[10px] text-text-muted mt-0.5 leading-tight line-clamp-2">{sub}</div>
              </div>
            </button>
          );
        })}
      </div>

      {/* Dry-Run error acknowledge */}
      {isError && (
        <div className="mt-3 p-3 rounded-xl bg-accent-red/10 border border-accent-red/30">
          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3">
            <div className="min-w-0">
              <p className="text-accent-red font-mono text-xs font-semibold uppercase tracking-wide">
                ⚠ Dry-Run Lockout Active
              </p>
              <p className="text-text-secondary text-xs mt-0.5">
                No flow detected for 30s. Check pump and water source, then acknowledge to resume.
              </p>
            </div>
            <button
              onClick={onAcknowledge}
              title="Acknowledge and resume normal operation"
              className="shrink-0 px-4 py-3 min-h-[44px] sm:min-h-0 sm:py-1.5 rounded-lg bg-accent-red/20 border border-accent-red/50
                         text-accent-red text-xs font-mono font-semibold
                         hover:bg-accent-red/30 active:bg-accent-red/30 transition-colors touch-manipulation
                         focus:outline-none focus:ring-2 focus:ring-accent-red/50 focus:ring-offset-2 focus:ring-offset-surface-1"
            >
              Acknowledge
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
