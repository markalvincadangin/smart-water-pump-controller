// components/ModeControls.tsx
"use client";

import clsx from "clsx";
import { Zap, ZapOff, Cpu } from "lucide-react";
import { useMemo, useState } from "react";
import type { PumpControl } from "@/lib/types";

interface ModeControlsProps {
  currentMode: PumpControl["mode"];
  isError: boolean;
  dryRunTimeoutSec?: number;  // from device config, default 30
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
    sub:    "Automatic — pump follows water level",
    Icon:   Cpu,
    active: "bg-accent-cyan/15 border-accent-cyan/50 text-accent-cyan shadow-[0_0_20px_rgba(0,229,255,0.2)]",
    hover:  "hover:bg-accent-cyan/8 hover:border-accent-cyan/30",
  },
  {
    id:     "FORCE_OFF",
    label:  "FORCE OFF",
    sub:    "Manual — pump stays off",
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
  const [confirmEmergency, setConfirmEmergency] = useState(false);

  const emergencyDisabledReason = useMemo(() => {
    if (!allowForceOn) return "Admin only";
    if (pendingMode !== null) return "Command pending";
    if (isError) return "Clear error first";
    if (!confirmEmergency) return "Confirm required";
    return null;
  }, [allowForceOn, confirmEmergency, isError, pendingMode]);

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between mb-4">
        <h3 className="font-display font-semibold text-text-primary text-sm uppercase tracking-widest">
          Mode Control
        </h3>
        {pendingMode ? (
          <span className="badge bg-accent-amber/10 text-accent-amber border border-accent-amber/20" title="Waiting for controller to confirm">
            Sending…
          </span>
        ) : (
          <span className="badge bg-surface-3 text-text-secondary border border-surface-4" title="Controls sync to your pump via the cloud">
            Synced
          </span>
        )}
      </div>

      {/* Mode buttons — touch-friendly 44px min on mobile */}
      <div className="grid grid-cols-2 gap-2 sm:gap-3">
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
                "flex flex-col items-center justify-center gap-1 sm:gap-2 p-2.5 sm:p-3 min-h-[64px] sm:min-h-0 rounded-xl border transition-all duration-200 touch-manipulation active:scale-[0.98]",
                "cursor-pointer select-none focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1",
                isActive
                  ? active
                  : clsx("border-surface-3 text-text-secondary", hover)
              )}
            >
              <Icon
                size={18}
                className={clsx(
                  "transition-all",
                  isActive && id === "AUTO"     && "drop-shadow-[0_0_6px_rgba(0,229,255,0.8)]",
                  isActive && id === "FORCE_OFF"&& "drop-shadow-[0_0_6px_rgba(255,59,92,0.8)]"
                )}
              />
              <div className="text-center min-w-0">
                <div className="text-[10px] sm:text-xs font-mono font-semibold leading-none">{label}</div>
                <div className={clsx("text-[9px] sm:text-[10px] text-text-muted mt-0.5 leading-tight line-clamp-2", isPending && "text-accent-amber")}>
                  {isPending ? "Sending…" : sub}
                </div>
              </div>
            </button>
          );
        })}
      </div>

      {/* Emergency override (admin-only) */}
      <div className="mt-2 rounded-xl border border-surface-3 bg-surface-2">
        <button
          type="button"
          onClick={() => setShowEmergency((v) => !v)}
          className="w-full px-3 py-2.5 flex items-center justify-between text-xs font-mono text-text-secondary"
          title="Emergency override controls"
        >
          <span className="uppercase tracking-widest">Emergency override</span>
          <span className="text-text-muted">{showEmergency ? "Hide" : "Show"}</span>
        </button>

        {showEmergency && (
          <div className="px-3 pb-3 space-y-2">
            <p className="text-[10px] font-mono text-text-muted">
              Use only when needed. Prefer <span className="text-text-secondary">Run Pump</span> for manual/timed runs.
              This mode runs continuously and relies on safety cutoffs.
            </p>

            <label className="flex items-center gap-2 text-[11px] font-mono text-text-secondary select-none">
              <input
                type="checkbox"
                checked={confirmEmergency}
                onChange={(e) => setConfirmEmergency(e.target.checked)}
                className="w-4 h-4 rounded border-surface-4 text-accent-amber focus:ring-accent-amber/50"
              />
              I understand the risk
            </label>

            <button
              type="button"
              onClick={() => onSetMode("FORCE_ON")}
              disabled={!!emergencyDisabledReason}
              className={clsx(
                "w-full px-4 py-3 rounded-xl border font-mono text-sm min-h-[44px] flex items-center justify-center gap-2 transition-colors",
                currentMode === "FORCE_ON"
                  ? "bg-accent-green/15 border-accent-green/50 text-accent-green"
                  : "border-surface-4 text-text-secondary hover:bg-surface-3",
                emergencyDisabledReason && "opacity-50 cursor-not-allowed"
              )}
              title={emergencyDisabledReason ?? "Emergency ON (continuous)"}
            >
              <Zap size={14} />
              Emergency ON (continuous)
            </button>

            {!allowForceOn && (
              <p className="text-[10px] font-mono text-text-muted">Admin only.</p>
            )}
            {isError && (
              <p className="text-[10px] font-mono text-text-muted">Clear the error first (Acknowledge).</p>
            )}
          </div>
        )}
      </div>

      {/* Dry-Run error acknowledge */}
      {isError && (
        <div className="mt-3 p-3 rounded-xl bg-accent-red/10 border border-accent-red/30">
          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3">
            <div className="min-w-0">
              <p className="text-accent-red font-mono text-xs font-semibold uppercase tracking-wide">
                ⚠ Pump stopped — no water flow
              </p>
              <p className="text-text-secondary text-xs mt-0.5">
                No water detected for {dryRunTimeoutSec}s. Check the pump and water source, then tap Acknowledge to resume.
              </p>
            </div>
            <button
              onClick={onAcknowledge}
              title="Acknowledge and resume normal operation"
              disabled={pendingAcknowledge}
              className="shrink-0 px-4 py-3 min-h-[44px] sm:min-h-0 sm:py-1.5 rounded-lg bg-accent-red/20 border border-accent-red/50
                         text-accent-red text-xs font-mono font-semibold
                         hover:bg-accent-red/30 active:bg-accent-red/30 transition-colors touch-manipulation disabled:opacity-50 disabled:cursor-not-allowed
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
