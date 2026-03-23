// components/ModeControls.tsx
"use client";

import clsx from "clsx";
import { Cpu, Hand, Timer } from "lucide-react";
import type { PumpControl } from "@/lib/types";

interface ModeControlsProps {
  currentMode: PumpControl["mode"];
  isError: boolean;
  dryRunTimeoutSec?: number;
  pendingMode?: PumpControl["mode"] | null;
  pendingAcknowledge?: boolean;
  onSetMode: (mode: PumpControl["mode"]) => void;
  onAcknowledge: () => void;
}

/** v4.0: Normal operation modes (AUTO, MANUAL) */
const NORMAL_MODES: {
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
    sub:    "Follows water level",
    Icon:   Cpu,
    active: "bg-accent-cyan/15 border-accent-cyan/50 text-accent-cyan shadow-[0_0_20px_rgb(var(--c-brand-500)/0.2)]",
    hover:  "hover:bg-accent-cyan/8 hover:border-accent-cyan/30",
  },
  {
    id:     "MANUAL",
    label:  "MANUAL",
    sub:    "Runs with full safety",
    Icon:   Hand,
    active: "bg-accent-green/15 border-accent-green/50 text-accent-green shadow-[0_0_20px_rgb(var(--c-status-ok)/0.2)]",
    hover:  "hover:bg-accent-green/8 hover:border-accent-green/30",
  },
  {
    id:     "COUNTDOWN",
    label:  "COUNTDOWN",
    sub:    "Timed run",
    Icon:   Timer,
    active: "bg-accent-amber/10 border-accent-amber/40 text-accent-amber shadow-[0_0_20px_rgb(var(--c-status-warn)/0.15)]",
    hover:  "hover:bg-accent-amber/5 hover:border-accent-amber/25",
  },
];

export default function ModeControls({
  currentMode,
  isError,
  dryRunTimeoutSec = 30,
  pendingMode = null,
  pendingAcknowledge = false,
  onSetMode,
  onAcknowledge,
}: ModeControlsProps) {
  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between">
        <h3 className="font-display font-semibold text-text-primary text-sm uppercase tracking-widest">
          Mode
        </h3>
        {pendingMode ? (
          <span className="badge text-[10px] bg-accent-amber/10 text-accent-amber border border-accent-amber/20">
            Sending…
          </span>
        ) : (
          <span className="badge text-[10px] bg-surface-3 text-text-muted border border-surface-4">
            Synced
          </span>
        )}
      </div>

      {/* Policy modes only (AUTO, MANUAL, COUNTDOWN) */}
      <div className="grid grid-cols-2 sm:grid-cols-3 gap-2">
        {NORMAL_MODES.map(({ id, label, sub, Icon, active, hover }) => {
          const isActive = currentMode === id;
          const isPending = pendingMode === id;
          return (
            <button
              key={id}
              onClick={() => onSetMode(id)}
              title={`${label}: ${sub}`}
              disabled={pendingMode !== null}
              className={clsx(
                "flex flex-col items-center justify-center gap-1 p-2 sm:p-3.5 min-h-[56px] sm:min-h-[64px] rounded-xl border transition-all duration-200 touch-manipulation active:scale-[0.98]",
                "cursor-pointer select-none focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1",
                id === "COUNTDOWN" && "col-span-2 sm:col-span-1",
                isActive ? active : clsx("border-surface-3 text-text-secondary", hover)
              )}
            >
              <Icon
                size={16}
                className={clsx(
                  "transition-all",
                  isActive && id === "AUTO"   && "drop-shadow-[0_0_6px_rgb(var(--c-brand-500)/0.8)]",
                  isActive && id === "MANUAL" && "drop-shadow-[0_0_6px_rgb(var(--c-status-ok)/0.8)]"
                )}
              />
              <div className="text-center min-w-0">
                <div className="text-[10px] sm:text-xs font-mono font-semibold leading-none">{label}</div>
                <div className={clsx("text-[9px] sm:text-[10px] text-text-muted mt-0.5 leading-tight", isPending && "text-accent-amber")}>
                  {isPending ? "Sending…" : sub}
                </div>
              </div>
            </button>
          );
        })}
      </div>

      {/* Dry-Run error acknowledge */}
      {isError && (
        <div className="p-3 rounded-xl bg-accent-red/10 border border-accent-red/30">
          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2 sm:gap-3">
            <div className="min-w-0">
              <p className="text-accent-red font-mono text-[10px] sm:text-xs font-semibold uppercase tracking-wide">
                Pump stopped — no water flow
              </p>
              <p className="text-text-secondary text-[10px] sm:text-xs mt-0.5">
                No flow for {dryRunTimeoutSec}s. Check pump and water source.
              </p>
            </div>
            <button
              onClick={onAcknowledge}
              disabled={pendingAcknowledge}
              className="shrink-0 px-4 py-2.5 min-h-[44px] rounded-lg bg-accent-red/20 border border-accent-red/50
                         text-accent-red text-xs font-mono font-semibold
                         hover:bg-accent-red/30 transition-colors touch-manipulation disabled:opacity-50
                         focus:outline-none focus:ring-2 focus:ring-accent-red/50 focus:ring-offset-2 focus:ring-offset-surface-1"
            >
              {pendingAcknowledge ? "Sending…" : "Acknowledge"}
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
