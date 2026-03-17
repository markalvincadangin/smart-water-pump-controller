// components/ModeControls.tsx
"use client";

import clsx from "clsx";
import { Zap, ZapOff, Cpu, Hand } from "lucide-react";
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
    active: "bg-accent-cyan/15 border-accent-cyan/50 text-accent-cyan shadow-[0_0_20px_rgba(0,229,255,0.2)]",
    hover:  "hover:bg-accent-cyan/8 hover:border-accent-cyan/30",
  },
  {
    id:     "MANUAL",
    label:  "MANUAL",
    sub:    "Runs with full safety",
    Icon:   Hand,
    active: "bg-accent-green/15 border-accent-green/50 text-accent-green shadow-[0_0_20px_rgba(0,200,83,0.2)]",
    hover:  "hover:bg-accent-green/8 hover:border-accent-green/30",
  },
];

/** Typed confirmation keyword for FORCE_ON (v5.0 2-step confirmation, shortened for lower error rate under stress) */
const FORCE_ON_CONFIRM_KEYWORD = "FORCE";

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
  /** v4.0: two-step dialog state — step 1 = warning, step 2 = typed confirmation */
  const [forceOnStep, setForceOnStep] = useState<0 | 1 | 2>(0);
  const [confirmInput, setConfirmInput] = useState("");

  const emergencyDisabledReason = useMemo(() => {
    if (!allowForceOn) return "Admin only";
    if (pendingMode !== null) return "Command pending";
    if (isError) return "Clear error first";
    return null;
  }, [allowForceOn, isError, pendingMode]);

  const handleForceOnConfirm = () => {
    if (confirmInput.trim().toUpperCase() === FORCE_ON_CONFIRM_KEYWORD) {
      setForceOnStep(0);
      setConfirmInput("");
      onSetMode("FORCE_ON");
    }
  };

  const cancelForceOn = () => {
    setForceOnStep(0);
    setConfirmInput("");
  };

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

      {/* v4.0: Normal operation modes (AUTO, MANUAL) */}
      <div className="grid grid-cols-2 gap-2">
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
                "flex flex-col items-center justify-center gap-1 p-2.5 sm:p-3 min-h-[60px] rounded-xl border transition-all duration-200 touch-manipulation active:scale-[0.98]",
                "cursor-pointer select-none focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1",
                isActive ? active : clsx("border-surface-3 text-text-secondary", hover)
              )}
            >
              <Icon
                size={16}
                className={clsx(
                  "transition-all",
                  isActive && id === "AUTO"   && "drop-shadow-[0_0_6px_rgba(0,229,255,0.8)]",
                  isActive && id === "MANUAL" && "drop-shadow-[0_0_6px_rgba(0,200,83,0.8)]"
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

      {/* v4.0: Emergency controls (FORCE_OFF + FORCE_ON with 2-step confirm) */}
      <div className="rounded-xl border border-surface-3 bg-surface-2 overflow-hidden" id="emergency-controls">
        <button
          type="button"
          onClick={() => setShowEmergency((v) => !v)}
          className="w-full px-3 py-2 flex items-center justify-between text-[10px] sm:text-xs font-mono text-text-muted hover:text-text-secondary transition-colors"
        >
          <span className="uppercase tracking-widest">Emergency Controls</span>
          <span>{showEmergency ? "▾" : "▸"}</span>
        </button>

        {showEmergency && (
          <div className="px-3 pb-3 space-y-2 border-t border-surface-3 pt-2">
            {/* FORCE_OFF button — always visible in emergency section */}
            <button
              type="button"
              onClick={() => onSetMode("FORCE_OFF")}
              disabled={pendingMode !== null}
              className={clsx(
                "w-full px-4 py-2.5 rounded-xl border font-mono text-xs sm:text-sm min-h-[44px] flex items-center justify-center gap-2 transition-colors touch-manipulation",
                currentMode === "FORCE_OFF"
                  ? "bg-accent-red/15 border-accent-red/50 text-accent-red shadow-[0_0_20px_rgba(255,59,92,0.2)]"
                  : "border-accent-red/30 text-accent-red hover:bg-accent-red/10"
              )}
              title="Emergency Stop — persistent until manually cleared"
            >
              <ZapOff size={14} />
              {currentMode === "FORCE_OFF" ? "FORCE OFF (Active)" : "FORCE OFF"}
            </button>

            {currentMode === "FORCE_OFF" && (
              <p className="text-[9px] font-mono text-accent-cyan">
                Pump stopped. Tap AUTO to resume automatic mode.
              </p>
            )}

            {/* Divider */}
            <div className="border-t border-surface-3 my-1" />

            {/* v4.0: FORCE_ON — 2-step typed confirmation */}
            <p className="text-[9px] sm:text-[10px] font-mono text-text-muted leading-snug">
              <strong className="text-accent-amber">Emergency Override — Run Without Safety</strong> bypasses
              <span className="font-semibold"> all safety protections</span>. The pump runs regardless of sensor errors,
              dry-run, or overflow conditions until you exit or the auto-timeout expires.
            </p>

            {/* Step 0: Initial button */}
            {forceOnStep === 0 && currentMode !== "FORCE_ON" && (
              <button
                type="button"
                onClick={() => !emergencyDisabledReason && setForceOnStep(1)}
                disabled={!!emergencyDisabledReason}
                className={clsx(
                  "w-full px-4 py-2.5 rounded-xl border font-mono text-xs sm:text-sm min-h-[44px] flex items-center justify-center gap-2 transition-colors",
                  emergencyDisabledReason
                    ? "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
                    : "border-accent-amber/30 text-accent-amber hover:bg-accent-amber/10"
                )}
                title={emergencyDisabledReason ?? "FORCE ON — absolute override (admin only)"}
              >
                <Zap size={14} />
                FORCE ON
              </button>
            )}

            {/* Step 1: Warning */}
            {forceOnStep === 1 && (
              <div className="p-3 rounded-xl bg-accent-red/10 border border-accent-red/30 space-y-2">
                <p className="text-[10px] sm:text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
                  ⚠ Emergency Override — Run Without Safety
                </p>
                <p className="text-[10px] sm:text-xs font-mono text-text-primary leading-snug">
                  This will bypass <strong>ALL</strong> safety protections including dry-run lockout,
                  overflow protection, and sensor failure guards. The pump will run until
                  manually stopped or the auto-timeout ({60} min) expires.
                </p>
                <div className="flex gap-2">
                  <button
                    type="button"
                    onClick={cancelForceOn}
                    className="flex-1 px-3 py-2 rounded-lg border border-surface-4 text-text-secondary font-mono text-xs hover:bg-surface-3 min-h-[44px]"
                  >
                    Cancel
                  </button>
                  <button
                    type="button"
                    onClick={() => setForceOnStep(2)}
                    className="flex-1 px-3 py-2 rounded-lg bg-accent-amber/20 border border-accent-amber/50 text-accent-amber font-mono text-xs font-semibold hover:bg-accent-amber/30 min-h-[44px]"
                  >
                    I Understand →
                  </button>
                </div>
              </div>
            )}

            {/* Step 2: Typed confirmation */}
            {forceOnStep === 2 && (
              <div className="p-3 rounded-xl bg-accent-red/10 border border-accent-red/30 space-y-2">
                <p className="text-[10px] sm:text-xs font-mono text-text-primary leading-snug">
                  Type <strong className="text-accent-amber">{FORCE_ON_CONFIRM_KEYWORD}</strong> to confirm:
                </p>
                <input
                  type="text"
                  autoFocus
                  autoComplete="off"
                  value={confirmInput}
                  onChange={(e) => setConfirmInput(e.target.value)}
                  onKeyDown={(e) => e.key === "Enter" && handleForceOnConfirm()}
                  placeholder="Type here…"
                  className="w-full px-3 py-2 rounded-lg bg-surface-1 border border-surface-4 text-text-primary font-mono text-sm min-h-[44px] focus:outline-none focus:ring-2 focus:ring-accent-amber/50"
                />
                <div className="flex gap-2">
                  <button
                    type="button"
                    onClick={cancelForceOn}
                    className="flex-1 px-3 py-2 rounded-lg border border-surface-4 text-text-secondary font-mono text-xs hover:bg-surface-3 min-h-[44px]"
                  >
                    Cancel
                  </button>
                  <button
                    type="button"
                    onClick={handleForceOnConfirm}
                    disabled={confirmInput.trim().toUpperCase() !== FORCE_ON_CONFIRM_KEYWORD}
                    className={clsx(
                      "flex-1 px-3 py-2 rounded-lg font-mono text-xs font-semibold min-h-[44px] transition-colors",
                      confirmInput.trim().toUpperCase() === FORCE_ON_CONFIRM_KEYWORD
                        ? "bg-accent-red/30 border border-accent-red/50 text-accent-red hover:bg-accent-red/40"
                        : "border border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
                    )}
                  >
                    Activate Override
                  </button>
                </div>
              </div>
            )}

            {/* When FORCE_ON is active: show status + exit button */}
            {currentMode === "FORCE_ON" && (
              <>
                <div className="p-3 rounded-xl bg-accent-red/15 border border-accent-red/40 space-y-1">
                  <p className="text-[10px] sm:text-xs font-mono text-accent-red font-semibold uppercase tracking-wide animate-pulse">
                    ⚡ FORCE ON Active — All Safety Bypassed
                  </p>
                  <p className="text-[9px] font-mono text-text-muted">
                    Pump is running in absolute override. Use buttons below to exit.
                  </p>
                </div>
                <div className="grid grid-cols-2 gap-2">
                  <button
                    type="button"
                    onClick={() => onSetMode("AUTO")}
                    className="px-4 py-3 rounded-xl min-h-[44px] flex items-center justify-center gap-2 font-mono text-xs font-semibold
                               bg-accent-cyan/20 border border-accent-cyan/50 text-accent-cyan hover:bg-accent-cyan/30 active:scale-[0.98]
                               touch-manipulation focus:outline-none focus:ring-2 focus:ring-accent-cyan/50"
                    title="Return to AUTO mode"
                  >
                    <Cpu size={14} />
                    Exit to AUTO
                  </button>
                  <button
                    type="button"
                    onClick={() => onSetMode("FORCE_OFF")}
                    className="px-4 py-3 rounded-xl min-h-[44px] flex items-center justify-center gap-2 font-mono text-xs font-semibold
                               bg-accent-red border border-accent-red/80 text-white hover:bg-accent-red/90 active:scale-[0.98]
                               touch-manipulation focus:outline-none focus:ring-2 focus:ring-accent-red/50"
                    title="Emergency Stop"
                  >
                    <ZapOff size={14} />
                    E-Stop
                  </button>
                </div>
              </>
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
