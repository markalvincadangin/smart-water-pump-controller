"use client";

import { useEffect, useMemo, useState } from "react";
import clsx from "clsx";
import { Timer, Play, Square, Plus } from "lucide-react";

type RunMode = "OFF" | "AUTO" | "AUTO_STANDBY" | "MANUAL" | "COUNTDOWN";
type ControlMode = "AUTO" | "FORCE_ON" | "FORCE_OFF" | "COUNTDOWN";

interface RunControlsProps {
  runMode: RunMode;
  remainingSec: number;
  controlMode: ControlMode;
  isError: boolean;
  isOverflowError?: boolean;
  isAdmin: boolean;
  esp32Online?: boolean;
  pendingAck?: boolean;
  onAcknowledge?: () => void;
  isAddingCountdownTime?: boolean;
  onStartManual: () => void;
  onStartCountdown: (durationMin: number) => void;
  onAddCountdownTime: () => void;
  onStop: () => void;
}

function formatMmSs(totalSec: number) {
  const s = Math.max(0, Math.floor(totalSec));
  const mm = Math.floor(s / 60);
  const ss = s % 60;
  return `${mm.toString().padStart(2, "0")}:${ss.toString().padStart(2, "0")}`;
}

const PRESETS_MIN = [5, 10, 15, 30, 60];

export default function RunControls({
  runMode,
  remainingSec,
  controlMode,
  isError,
  isOverflowError = false,
  isAdmin,
  esp32Online = true,
  pendingAck = false,
  onAcknowledge,
  isAddingCountdownTime = false,
  onStartManual,
  onStartCountdown,
  onAddCountdownTime,
  onStop,
}: RunControlsProps) {
  const [busy, setBusy] = useState<"manual" | "countdown" | "add" | "stop" | null>(null);
  const [countdownMin, setCountdownMin] = useState<number>(10);

  const [localRemainingSec, setLocalRemainingSec] = useState<number | null>(null);

  useEffect(() => {
    if (runMode === "COUNTDOWN") {
      setLocalRemainingSec(remainingSec);
    } else {
      setLocalRemainingSec(null);
    }
  }, [runMode, remainingSec]);

  useEffect(() => {
    if (runMode !== "COUNTDOWN" || localRemainingSec == null || localRemainingSec <= 0) return;
    const id = window.setInterval(() => {
      setLocalRemainingSec((prev) => (prev != null && prev > 0 ? prev - 1 : prev));
    }, 1000);
    return () => window.clearInterval(id);
  }, [runMode, localRemainingSec]);

  const showStop = runMode === "MANUAL" || runMode === "COUNTDOWN";
  const effectiveRemaining = localRemainingSec ?? remainingSec;
  const countdown = useMemo(
    () => (runMode === "COUNTDOWN" ? formatMmSs(effectiveRemaining) : null),
    [effectiveRemaining, runMode]
  );
  const blockedByForceOff = controlMode === "FORCE_OFF" && !showStop;
  const isLockedOut = isError || isOverflowError;
  const controllerOffline = !esp32Online;
  const canAct = isAdmin && !isLockedOut && !blockedByForceOff && !controllerOffline;

  return (
    <div className="space-y-3">
      {/* Header with run mode badge */}
      <div className="flex items-center justify-between">
        <h3 className="font-display font-semibold text-text-primary text-sm uppercase tracking-widest">
          Run Pump
        </h3>
        <span
          className={clsx(
            "badge border font-mono text-[10px]",
            runMode === "COUNTDOWN" ? "bg-accent-amber/10 text-accent-amber border-accent-amber/20"
              : runMode === "MANUAL" ? "bg-accent-green/10 text-accent-green border-accent-green/20"
                : runMode === "AUTO" || runMode === "AUTO_STANDBY" ? "bg-accent-cyan/10 text-accent-cyan border-accent-cyan/20"
                  : "bg-surface-3 text-text-secondary border-surface-4"
          )}
          title="Run mode reported by the controller"
        >
          {runMode === "OFF" ? "OFF" : runMode === "AUTO_STANDBY" ? "Standby" : runMode}
          {countdown ? ` ${countdown}` : ""}
        </span>
      </div>

      {/* FORCE_ON notice */}
      {controlMode === "FORCE_ON" && (
        <div className="p-2.5 rounded-xl bg-accent-amber/10 border border-accent-amber/25">
          <p className="text-[10px] sm:text-xs font-mono text-accent-amber font-semibold uppercase tracking-wide">
            Emergency override active
          </p>
          <p className="text-[10px] font-mono text-text-secondary mt-0.5">
            Pump runs until you change mode. Use Quick Start or Countdown for normal runs with Stop.
          </p>
        </div>
      )}

      {/* FORCE_OFF notice */}
      {blockedByForceOff && (
        <div className="p-2.5 rounded-xl bg-accent-red/10 border border-accent-red/25">
          <p className="text-[10px] sm:text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
            Force Off active
          </p>
          <p className="text-[10px] font-mono text-text-secondary mt-0.5">
            Switch to AUTO to enable pump controls.
          </p>
        </div>
      )}

      {/* Error lockout */}
      {isLockedOut && (
        <div className="p-2.5 rounded-xl bg-accent-red/10 border border-accent-red/25 space-y-2">
          <p className="text-[10px] sm:text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
            {isError ? "Dry-run lockout" : "Overflow lockout"}
          </p>
          <p className="text-[10px] font-mono text-text-secondary">
            Clear the error to start a new run.
          </p>
          {onAcknowledge && (
            <button
              type="button"
              onClick={onAcknowledge}
              disabled={pendingAck || !esp32Online}
              className="min-h-[44px] px-4 py-2.5 rounded-xl bg-accent-red/20 border border-accent-red/50
                         text-accent-red text-xs font-mono font-semibold
                         hover:bg-accent-red/30 transition-colors touch-manipulation
                         disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {pendingAck ? "Sending…" : "Clear Error"}
            </button>
          )}
        </div>
      )}

      {/* Main controls — hidden when locked out or FORCE_ON */}
      {!isLockedOut && controlMode !== "FORCE_ON" && (
        <>
          {/* Start buttons */}
          <div className="grid grid-cols-2 gap-2">
            <button
              type="button"
              disabled={busy !== null || !canAct}
              onClick={() => {
                setBusy("manual");
                try { onStartManual(); } finally { window.setTimeout(() => setBusy(null), 8000); }
              }}
              className={clsx(
                "px-3 py-2.5 rounded-xl border font-mono text-xs sm:text-sm min-h-[48px] flex items-center justify-center gap-1.5 sm:gap-2 transition-colors touch-manipulation",
                canAct
                  ? "border-accent-green/30 text-accent-green hover:bg-accent-green/10 active:scale-[0.98]"
                  : "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
              )}
              title={!isAdmin ? "Admin only" : blockedByForceOff ? "FORCE OFF active" : "Start a manual run (stop anytime)"}
            >
              <Play size={14} />
              <span className="sm:hidden">Manual</span>
              <span className="hidden sm:inline">Quick Start</span>
            </button>

            <button
              type="button"
              disabled={busy !== null || !canAct}
              onClick={() => {
                setBusy("countdown");
                try { onStartCountdown(countdownMin); } finally { window.setTimeout(() => setBusy(null), 8000); }
              }}
              className={clsx(
                "px-3 py-2.5 rounded-xl border font-mono text-xs sm:text-sm min-h-[48px] flex items-center justify-center gap-1.5 sm:gap-2 transition-colors touch-manipulation",
                canAct
                  ? "border-accent-amber/30 text-accent-amber hover:bg-accent-amber/10 active:scale-[0.98]"
                  : "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
              )}
              title={!isAdmin ? "Admin only" : "Run for set time then auto-stop"}
            >
              <Timer size={14} />
              Countdown
            </button>
          </div>

          {/* Countdown duration — inline with label */}
          <div className="flex items-center gap-2">
            <label className="text-[10px] font-mono text-text-muted shrink-0">Duration</label>
            <div className="flex gap-1 flex-1 min-w-0">
              {PRESETS_MIN.map((m) => (
                <button
                  key={m}
                  type="button"
                  onClick={() => setCountdownMin(m)}
                  disabled={busy !== null || showStop}
                  className={clsx(
                    "flex-1 px-1.5 py-1.5 rounded-lg font-mono text-[10px] sm:text-xs transition-colors min-w-0",
                    m === countdownMin
                      ? "bg-accent-amber/15 text-accent-amber border border-accent-amber/30"
                      : "bg-surface-2 text-text-muted border border-surface-4 hover:text-text-secondary"
                  )}
                >
                  {m}m
                </button>
              ))}
            </div>
          </div>

          {/* Add time (during countdown) */}
          {runMode === "COUNTDOWN" && (
            <button
              type="button"
              disabled={busy !== null || isAddingCountdownTime || controllerOffline}
              onClick={() => {
                setBusy("add");
                try { onAddCountdownTime(); } finally { window.setTimeout(() => setBusy(null), 8000); }
              }}
              className="w-full px-3 py-2 rounded-xl border border-accent-amber/30 text-accent-amber font-mono text-xs sm:text-sm hover:bg-accent-amber/10 min-h-[44px] flex items-center justify-center gap-2 disabled:opacity-50 touch-manipulation"
              title={isAddingCountdownTime ? "Waiting for controller…" : "Add 5 minutes to the countdown"}
            >
              <Plus size={12} />
              Add 5 min
            </button>
          )}

          {/* Stop */}
          {showStop && (
            <button
              type="button"
              disabled={busy !== null || controllerOffline}
              onClick={() => {
                setBusy("stop");
                try { onStop(); } finally { window.setTimeout(() => setBusy(null), 8000); }
              }}
              className="w-full px-4 py-3 rounded-xl bg-accent-red/20 border border-accent-red/40 text-accent-red font-mono text-sm font-semibold hover:bg-accent-red/30 min-h-[56px] sm:min-h-[64px] flex items-center justify-center gap-2 disabled:opacity-50 touch-manipulation active:scale-[0.98]"
              title="Stop the pump now"
            >
              <Square size={16} />
              Stop
            </button>
          )}

          {!isAdmin && (
            <p className="text-[9px] font-mono text-text-muted">Admin access required for pump controls.</p>
          )}
        </>
      )}
    </div>
  );
}
