// components/ModeControls.tsx
"use client";

import clsx from "clsx";
import { Zap, ZapOff, Cpu } from "lucide-react";
import { useMemo, useState } from "react";
import type { PumpControl } from "@/lib/types";

interface ModeControlsProps {
  currentMode: PumpControl["mode"];
  isError: boolean;
  dryRunTimeoutSec?: number;
  pendingMode?: PumpControl["mode"] | null;
  pendingAcknowledge?: boolean;
  allowForceOn?: boolean;
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
    sub:    "Follows water level",
    Icon:   Cpu,
    active: "bg-accent-cyan/15 border-accent-cyan/50 text-accent-cyan shadow-[0_0_20px_rgba(0,229,255,0.2)]",
    hover:  "hover:bg-accent-cyan/8 hover:border-accent-cyan/30",
  },
  {
    id:     "FORCE_OFF",
    label:  "FORCE OFF",
    sub:    "Pump stays off",
    Icon:   ZapOff,
    active: "bg-accent-red/15 border-accent-red/50 text-accent-red shadow-[0_0_20px_rgba(255,59,92,0.2)]",
    hover:  "hover:bg-accent-red/8 hover:border-accent-red/30",
  },
];

export default function ModeControls({
  currentMode,
  isError,
  dryRunTimeoutSec = 30,
  pendingMode = null,
  pendingAcknowledge = false,
  allowForceOn = true,
  onSetMode,
  onAcknowledge,
}: ModeControlsProps) {
  const [showEmergency, setShowEmergency] = useState(false);
  const [showEmergencyConfirmDialog, setShowEmergencyConfirmDialog] = useState(false);

  const emergencyDisabledReason = useMemo(() => {
    if (!allowForceOn) return "Admin only";
    if (pendingMode !== null) return "Command pending";
    if (isError) return "Clear error first";
    return null;
  }, [allowForceOn, isError, pendingMode]);

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

      {/* Mode toggle buttons */}
      <div className="grid grid-cols-2 gap-2">
        {MODES.map(({ id, label, sub, Icon, active, hover }) => {
          const isActive = currentMode === id;
          const isPending = pendingMode === id;
          return (
            <button
              key={id}
              onClick={() => onSetMode(id)}
              title={`${label}: ${sub}`}
              disabled={pendingMode !== null}
              className={clsx(
                "flex flex-col items-center justify-center gap-1 p-2.5 sm:p-3 min-h-[60px] rounded-xl border transition-all duration-200 touch-manipulation active:scale-[0.98]",
                "cursor-pointer select-none focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1",
                isActive ? active : clsx("border-surface-3 text-text-secondary", hover)
              )}
            >
              <Icon
                size={16}
                className={clsx(
                  "transition-all",
                  isActive && id === "AUTO"      && "drop-shadow-[0_0_6px_rgba(0,229,255,0.8)]",
                  isActive && id === "FORCE_OFF" && "drop-shadow-[0_0_6px_rgba(255,59,92,0.8)]"
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

      {/* Emergency override (collapsible, admin-only) */}
      <div className="rounded-xl border border-surface-3 bg-surface-2 overflow-hidden">
        <button
          type="button"
          onClick={() => setShowEmergency((v) => !v)}
          className="w-full px-3 py-2 flex items-center justify-between text-[10px] sm:text-xs font-mono text-text-muted hover:text-text-secondary transition-colors"
        >
          <span className="uppercase tracking-widest">Emergency override</span>
          <span>{showEmergency ? "▾" : "▸"}</span>
        </button>

        {showEmergency && (
          <div className="px-3 pb-3 space-y-2 border-t border-surface-3 pt-2">
            <p className="text-[9px] sm:text-[10px] font-mono text-text-muted leading-snug">
              Runs the pump continuously; relies on safety cutoffs only. Prefer Quick Start or Countdown for normal use.
            </p>

            {showEmergencyConfirmDialog && (
              <div className="p-3 rounded-xl bg-accent-amber/10 border border-accent-amber/30 space-y-2">
                <p className="text-[10px] sm:text-xs font-mono text-text-primary leading-snug">
                  Emergency Override will run the pump without auto-stop. Confirm?
                </p>
                <div className="flex gap-2">
                  <button
                    type="button"
                    onClick={() => setShowEmergencyConfirmDialog(false)}
                    className="flex-1 px-3 py-2 rounded-lg border border-surface-4 text-text-secondary font-mono text-xs hover:bg-surface-3 min-h-[44px]"
                  >
                    Cancel
                  </button>
                  <button
                    type="button"
                    onClick={() => {
                      setShowEmergencyConfirmDialog(false);
                      onSetMode("FORCE_ON");
                    }}
                    className="flex-1 px-3 py-2 rounded-lg bg-accent-amber/20 border border-accent-amber/50 text-accent-amber font-mono text-xs font-semibold hover:bg-accent-amber/30 min-h-[44px]"
                  >
                    Confirm
                  </button>
                </div>
              </div>
            )}

            {currentMode === "FORCE_ON" ? (
              <>
                <button
                  type="button"
                  onClick={() => onSetMode("FORCE_OFF")}
                  className="w-full px-4 py-3 rounded-xl min-h-[56px] flex items-center justify-center gap-2 font-mono text-sm font-semibold
                             bg-accent-red border border-accent-red/80 text-white hover:bg-accent-red/90 active:scale-[0.98]
                             touch-manipulation focus:outline-none focus:ring-2 focus:ring-accent-red/50 focus:ring-offset-2 focus:ring-offset-surface-1"
                  title="Emergency Stop"
                >
                  <ZapOff size={16} />
                  Emergency Stop
                </button>
                <p className="text-[9px] font-mono text-text-muted">
                  Tap <span className="text-text-primary font-semibold">AUTO</span> to return to automatic mode.
                </p>
              </>
            ) : (
              <button
                type="button"
                onClick={() => !emergencyDisabledReason && setShowEmergencyConfirmDialog(true)}
                disabled={!!emergencyDisabledReason}
                className={clsx(
                  "w-full px-4 py-2.5 rounded-xl border font-mono text-xs sm:text-sm min-h-[44px] flex items-center justify-center gap-2 transition-colors",
                  emergencyDisabledReason
                    ? "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
                    : "border-accent-amber/30 text-accent-amber hover:bg-accent-amber/10"
                )}
                title={emergencyDisabledReason ?? "Emergency ON (continuous)"}
              >
                <Zap size={14} />
                Emergency ON
              </button>
            )}

            {currentMode === "FORCE_OFF" && (
              <p className="text-[9px] font-mono text-accent-cyan">
                Pump stopped. Tap AUTO to resume automatic mode.
              </p>
            )}

            {!allowForceOn && (
              <p className="text-[9px] font-mono text-text-muted">Admin access required.</p>
            )}
          </div>
        )}
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
