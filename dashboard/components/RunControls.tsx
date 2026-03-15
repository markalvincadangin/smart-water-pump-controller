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
  onAddCountdownTime: (addMinutes: number) => void;
  onStop: () => void;
}

function formatMmSs(totalSec: number) {
  const s = Math.max(0, Math.floor(totalSec));
  const mm = Math.floor(s / 60);
  const ss = s % 60;
  return `${mm.toString().padStart(2, "0")}:${ss.toString().padStart(2, "0")}`;
}

const COUNTDOWN_MIN_MIN = 1;
const COUNTDOWN_MAX_MIN = 120;
const PRESETS_MIN = [5, 10, 15, 30, 60];
const ADD_TIME_PRESETS_MIN = [1, 5, 10, 15, 20, 30];

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
  const [customDurationInput, setCustomDurationInput] = useState("");
  const [addTimeMin, setAddTimeMin] = useState(5);
  const [customAddInput, setCustomAddInput] = useState("");
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
                const n = customDurationInput ? parseInt(customDurationInput, 10) : null;
                const duration = (n != null && !Number.isNaN(n) && n >= COUNTDOWN_MIN_MIN && n <= COUNTDOWN_MAX_MIN)
                  ? Math.max(COUNTDOWN_MIN_MIN, Math.min(COUNTDOWN_MAX_MIN, n)) : countdownMin;
                setBusy("countdown");
                try { onStartCountdown(duration); } finally { window.setTimeout(() => setBusy(null), 8000); }
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

          {/* Countdown duration — presets + custom (1–120 min) */}
          <div className="space-y-1.5">
            <label className="text-[10px] font-mono text-text-muted block">Duration (min)</label>
            <div className="flex flex-wrap items-center gap-2">
              {PRESETS_MIN.map((m) => (
                <button
                  key={m}
                  type="button"
                  onClick={() => { setCountdownMin(m); setCustomDurationInput(""); }}
                  disabled={busy !== null || showStop}
                  className={clsx(
                    "px-2 py-1.5 rounded-lg font-mono text-[10px] sm:text-xs transition-colors",
                    m === countdownMin && !customDurationInput
                      ? "bg-accent-amber/15 text-accent-amber border border-accent-amber/30"
                      : "bg-surface-2 text-text-muted border border-surface-4 hover:text-text-secondary"
                  )}
                >
                  {m}m
                </button>
              ))}
              <span className="text-[10px] font-mono text-text-muted">or</span>
              <input
                type="number"
                min={COUNTDOWN_MIN_MIN}
                max={COUNTDOWN_MAX_MIN}
                step={1}
                placeholder="Custom"
                value={customDurationInput}
                onChange={(e) => setCustomDurationInput(e.target.value.replace(/\D/g, "").slice(0, 3))}
                onBlur={() => {
                  const n = parseInt(customDurationInput, 10);
                  if (!Number.isNaN(n)) {
                    const clamped = Math.max(COUNTDOWN_MIN_MIN, Math.min(COUNTDOWN_MAX_MIN, n));
                    setCountdownMin(clamped);
                    setCustomDurationInput(String(clamped));
                  } else setCustomDurationInput("");
                }}
                disabled={busy !== null || showStop}
                className="w-14 px-2 py-1.5 rounded-lg font-mono text-xs bg-surface-2 border border-surface-4 text-text-primary disabled:opacity-50 [appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
                aria-label="Custom duration (1–120 min)"
              />
              <span className="text-[10px] font-mono text-text-muted">min</span>
            </div>
          </div>

          {/* Add time when countdown is active or mode is COUNTDOWN (e.g. after lockout so Add time still works) */}
          {(runMode === "COUNTDOWN" || controlMode === "COUNTDOWN") && (
            <div className="space-y-1.5">
              <label className="text-[10px] font-mono text-text-muted block">Add time</label>
              <div className="flex flex-wrap items-center gap-2">
                {ADD_TIME_PRESETS_MIN.map((m) => (
                  <button
                    key={m}
                    type="button"
                    onClick={() => setAddTimeMin(m)}
                    disabled={busy !== null || isAddingCountdownTime || controllerOffline}
                    className={clsx(
                      "px-2 py-1.5 rounded-lg font-mono text-[10px] sm:text-xs transition-colors",
                      m === addTimeMin ? "bg-accent-amber/15 text-accent-amber border border-accent-amber/30" : "bg-surface-2 text-text-muted border border-surface-4"
                    )}
                  >
                    +{m}m
                  </button>
                ))}
                <input
                  type="number"
                  min={COUNTDOWN_MIN_MIN}
                  max={COUNTDOWN_MAX_MIN}
                  step={1}
                  placeholder="Min"
                  value={customAddInput}
                  onChange={(e) => setCustomAddInput(e.target.value.replace(/\D/g, "").slice(0, 3))}
                  onBlur={() => {
                    const n = parseInt(customAddInput, 10);
                    if (!Number.isNaN(n)) {
                      const clamped = Math.max(COUNTDOWN_MIN_MIN, Math.min(COUNTDOWN_MAX_MIN, n));
                      setAddTimeMin(clamped);
                      setCustomAddInput(String(clamped));
                    } else setCustomAddInput("");
                  }}
                  disabled={busy !== null || isAddingCountdownTime || controllerOffline}
                  className="w-12 px-2 py-1.5 rounded-lg font-mono text-xs bg-surface-2 border border-surface-4 text-text-primary disabled:opacity-50 [appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
                  aria-label="Custom add min (1–120)"
                />
                <button
                  type="button"
                  disabled={busy !== null || isAddingCountdownTime || controllerOffline}
                  onClick={() => {
                    const toAdd = customAddInput ? (() => {
                      const n = parseInt(customAddInput, 10);
                      return Number.isNaN(n) ? addTimeMin : Math.max(COUNTDOWN_MIN_MIN, Math.min(COUNTDOWN_MAX_MIN, n));
                    })() : addTimeMin;
                    setBusy("add");
                    try { onAddCountdownTime(toAdd); } finally { window.setTimeout(() => setBusy(null), 8000); }
                  }}
                  className="ml-auto min-h-[44px] px-3 py-2 rounded-xl border border-accent-amber/30 text-accent-amber font-mono text-xs sm:text-sm hover:bg-accent-amber/10 disabled:opacity-50"
                >
                  <Plus size={12} /> Add {customAddInput ? (parseInt(customAddInput, 10) || addTimeMin) : addTimeMin} min
                </button>
              </div>
            </div>
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
