// components/RunControls.tsx
"use client";

import { useEffect, useMemo, useState } from "react";
import clsx from "clsx";
import { Timer, Power, PowerOff, Square, Plus, AlertOctagon, RotateCcw } from "lucide-react";

/** vNext: runMode includes MANUAL_ON/MANUAL_OFF and STOPPED (E-STOP latch) */
type RunMode = "OFF" | "AUTO" | "AUTO_STANDBY" | "MANUAL_ON" | "MANUAL_OFF" | "COUNTDOWN" | "STOPPED";
type ControlMode = "AUTO" | "COUNTDOWN" | "MANUAL";

interface RunControlsProps {
  runMode: RunMode;
  remainingSec: number;
  controlMode: ControlMode;
  isError: boolean;
  isOverflowError?: boolean;
  isAdmin: boolean;
  esp32Online?: boolean;
  manualDesired?: boolean;
  emergencyStopLatched?: boolean;
  pendingAck?: boolean;
  onAcknowledge?: () => void;
  isAddingCountdownTime?: boolean;
  onSetManualDesired: (on: boolean) => void;
  onStartCountdown: (durationMin: number) => void;
  onAddCountdownTime: (addMinutes: number) => void;
  onStopCountdown: () => void;
  onEmergencyStop: () => void;
  onResetStop: () => void;
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
  manualDesired = false,
  emergencyStopLatched = false,
  pendingAck = false,
  onAcknowledge,
  isAddingCountdownTime = false,
  onSetManualDesired,
  onStartCountdown,
  onAddCountdownTime,
  onStopCountdown,
  onEmergencyStop,
  onResetStop,
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

  const effectiveRemaining = localRemainingSec ?? remainingSec;
  const countdown = useMemo(
    () => (runMode === "COUNTDOWN" ? formatMmSs(effectiveRemaining) : null),
    [effectiveRemaining, runMode]
  );
  const isLockedOut = isError || isOverflowError;
  const controllerOffline = !esp32Online;
  const canAct = isAdmin && !isLockedOut && !controllerOffline;

  // vNext: Is pump currently active in MANUAL mode?
  const isManualOn = controlMode === "MANUAL" && manualDesired === true && runMode === "MANUAL_ON";
  const isManualOff = controlMode === "MANUAL" && manualDesired === false && (runMode === "MANUAL_OFF" || runMode === "OFF");
  const isInManualMode = controlMode === "MANUAL";
  // v5: Is pump idle (can start countdown)?
  const isIdle = runMode === "OFF" || runMode === "AUTO_STANDBY" || runMode === "MANUAL_OFF";

  return (
    <div className="space-y-3">
      {/* Header with run mode badge */}
      <div className="flex items-center justify-between min-w-0 gap-2">
        <h3 className="font-display font-semibold text-text-primary text-sm uppercase tracking-widest min-w-0">
          Controls
        </h3>
        <span
          className={clsx(
            "badge border font-mono text-[10px] min-w-0 max-w-[60%] truncate",
            runMode === "STOPPED" ? "bg-accent-red/15 text-accent-red border-accent-red/30"
              : runMode === "COUNTDOWN" ? "bg-accent-amber/10 text-accent-amber border-accent-amber/20"
                : runMode === "MANUAL_ON" ? "bg-accent-green/10 text-accent-green border-accent-green/20"
                  : runMode === "MANUAL_OFF" ? "bg-surface-3 text-accent-green border-accent-green/20"
                    : runMode === "AUTO" || runMode === "AUTO_STANDBY" ? "bg-accent-cyan/10 text-accent-cyan border-accent-cyan/20"
                      : "bg-surface-3 text-text-secondary border-surface-4"
          )}
          title="Run mode reported by the controller"
        >
          {runMode === "OFF" ? "OFF"
            : runMode === "AUTO_STANDBY" ? "Standby"
            : runMode === "STOPPED" ? "STOPPED"
            : runMode === "MANUAL_OFF" ? "MANUAL (Off)"
            : runMode === "MANUAL_ON" ? "MANUAL (On)"
            : runMode}
          {countdown ? ` ${countdown}` : ""}
        </span>
      </div>

      {/* ── E-STOP latch state ───────────────────────────────────────── */}
      {emergencyStopLatched && (
        <div className="p-2.5 rounded-xl bg-accent-red/10 border border-accent-red/25 space-y-2">
          <p className="text-[10px] sm:text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
            Emergency stop latched
          </p>
          <p className="text-[10px] font-mono text-text-secondary">
            Pump is stopped and locked out until Reset Stop is pressed (when safe).
          </p>
          <button
            type="button"
            disabled={busy !== null || controllerOffline}
            onClick={() => {
              setBusy("stop");
              try { onResetStop(); } finally { window.setTimeout(() => setBusy(null), 8000); }
            }}
            className="w-full min-h-[44px] px-4 py-2.5 rounded-xl border border-accent-amber/40 bg-accent-amber/10 text-accent-amber font-mono text-xs sm:text-sm font-semibold hover:bg-accent-amber/20 disabled:opacity-50"
            title="Reset emergency stop latch"
          >
            <RotateCcw size={14} />
            Reset Stop
          </button>
        </div>
      )}

      {/* ── Error lockout (inline per ISA-101) ──────────────────────── */}
      {isLockedOut && (
        <div className="p-2.5 rounded-xl bg-accent-red/10 border border-accent-red/25 space-y-2">
          <p className="text-[10px] sm:text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
            {isError ? "Dry-run lockout" : "Overflow lockout"}
          </p>
          <p className="text-[10px] font-mono text-text-secondary">
            {controlMode === "MANUAL"
              ? "Clearing this error will immediately restart the pump in MANUAL mode. Ensure water is available first."
              : "Clear the error to resume. Pump is stopped."}
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
              {pendingAck ? "Sending…" : controlMode === "MANUAL" ? "Clear Error & Restart" : "Clear Error"}
            </button>
          )}
        </div>
      )}

      {/* ── Main controls ─────────────────────────────────────────────── */}
      {!isLockedOut && !emergencyStopLatched && (
        <>
          {/* ── AUTO mode: informational ─────────────────────────── */}
          {controlMode === "AUTO" && !isIdle && runMode === "AUTO" && (
            <div className="p-3 rounded-xl bg-surface-2 border border-surface-3">
              <p className="text-[10px] sm:text-xs font-mono text-text-secondary">
                Pump is running automatically based on water level.
              </p>
            </div>
          )}

          {controlMode === "AUTO" && (runMode === "AUTO_STANDBY" || runMode === "OFF") && (
            <div className="p-3 rounded-xl bg-surface-2 border border-surface-3">
              <p className="text-[10px] sm:text-xs font-mono text-text-muted">
                Automatic mode — pump starts when water drops below threshold.
              </p>
            </div>
          )}

          {/* ── MANUAL mode: ON/OFF toggle ───────────────────────── */}
          {isInManualMode && (
            <div className="space-y-2">
              <div className="grid grid-cols-2 gap-2">
                {/* ON button */}
                <button
                  type="button"
                  disabled={busy !== null || !canAct || isManualOn}
                  onClick={() => {
                    setBusy("manual");
                    try { onSetManualDesired(true); } finally { window.setTimeout(() => setBusy(null), 8000); }
                  }}
                  className={clsx(
                    "px-3 py-3 rounded-xl border font-mono text-sm min-h-[56px] flex items-center justify-center gap-2 transition-all duration-200 touch-manipulation active:scale-[0.98]",
                    isManualOn
                      ? "bg-accent-green/20 border-accent-green/50 text-accent-green shadow-[0_0_20px_rgba(0,200,83,0.2)] cursor-default"
                      : canAct
                        ? "border-accent-green/30 text-accent-green hover:bg-accent-green/10"
                        : "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
                  )}
                  title={isManualOn ? "Pump is ON" : "Turn pump ON (full safety active)"}
                >
                  <Power size={20} />
                  <span className="font-semibold">{busy === "manual" ? "Starting…" : "ON"}</span>
                </button>

                {/* OFF button */}
                <button
                  type="button"
                  disabled={busy !== null || !canAct || isManualOff}
                  onClick={() => {
                    setBusy("stop");
                    try { onSetManualDesired(false); } finally { window.setTimeout(() => setBusy(null), 8000); }
                  }}
                  className={clsx(
                    "px-3 py-3 rounded-xl border font-mono text-sm min-h-[56px] flex items-center justify-center gap-2 transition-all duration-200 touch-manipulation active:scale-[0.98]",
                    isManualOff
                      ? "bg-surface-3 border-surface-4 text-text-secondary cursor-default"
                      : canAct
                        ? "border-accent-red/30 text-accent-red hover:bg-accent-red/10"
                        : "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
                  )}
                  title={isManualOff ? "Pump is OFF" : "Turn pump OFF"}
                >
                  <PowerOff size={20} />
                  <span className="font-semibold">{busy === "stop" ? "Stopping…" : "OFF"}</span>
                </button>
              </div>
              <p className="text-[9px] font-mono text-text-muted text-center">
                Manual mode — all safety protections remain active
              </p>
            </div>
          )}

          {/* ── COUNTDOWN active: timer display + stop + add time ──── */}
          {runMode === "COUNTDOWN" && (
            <div className="space-y-3">
              {/* Timer display */}
              <div className="flex items-center justify-center gap-3 p-3 rounded-xl bg-accent-amber/10 border border-accent-amber/30">
                <Timer size={20} className="text-accent-amber" />
                <span className="text-2xl font-display font-bold text-accent-amber tabular-nums">
                  {formatMmSs(effectiveRemaining)}
                </span>
              </div>

              {/* Stop */}
              <button
                type="button"
                disabled={busy !== null || controllerOffline}
                onClick={() => {
                  setBusy("stop");
                  try { onStopCountdown(); } finally { window.setTimeout(() => setBusy(null), 8000); }
                }}
                className="w-full px-4 py-3 rounded-xl bg-accent-red/20 border border-accent-red/40 text-accent-red font-mono text-sm font-semibold hover:bg-accent-red/30 min-h-[56px] flex items-center justify-center gap-2 disabled:opacity-50 touch-manipulation active:scale-[0.98]"
                title="Stop the pump now"
              >
                <Square size={16} />
                Stop
              </button>

              {/* Add time */}
              <div className="p-3 rounded-xl bg-surface-2 border border-surface-3 space-y-2">
                <label className="text-[10px] font-mono text-text-muted uppercase tracking-widest block">
                  Add time
                </label>
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
                    className="w-full sm:w-auto sm:ml-auto min-h-[44px] px-3 py-2 rounded-xl border border-accent-amber/30 text-accent-amber font-mono text-xs sm:text-sm hover:bg-accent-amber/10 disabled:opacity-50 flex items-center justify-center gap-1.5"
                  >
                    <Plus size={12} /> Add {customAddInput ? (parseInt(customAddInput, 10) || addTimeMin) : addTimeMin} min
                  </button>
                </div>
                <p className="text-[9px] font-mono text-text-muted">
                  Tip: Add time is safer when flow is stable above the dry-run threshold.
                </p>
              </div>
            </div>
          )}

          {/* ── Semi-Auto Timer (COUNTDOWN only) ───────────────────── */}
          {/* Mode-specific control: duration selection should only appear in COUNTDOWN mode.
              We still keep it visible during sync latency (controlMode=COUNTDOWN, runMode!=COUNTDOWN). */}
          {controlMode === "COUNTDOWN" && isIdle && (
            <div className="space-y-2">
              <div className="flex items-center justify-between">
                <label className="text-[10px] font-mono text-text-muted uppercase tracking-widest">
                  Semi-Auto Timer
                </label>
              </div>
              <div className="flex flex-wrap items-center gap-2">
                {PRESETS_MIN.map((m) => (
                  <button
                    key={m}
                    type="button"
                    onClick={() => { setCountdownMin(m); setCustomDurationInput(""); }}
                    disabled={busy !== null}
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
                  disabled={busy !== null}
                  className="w-14 px-2 py-1.5 rounded-lg font-mono text-xs bg-surface-2 border border-surface-4 text-text-primary disabled:opacity-50 [appearance:textfield] [&::-webkit-outer-spin-button]:appearance-none [&::-webkit-inner-spin-button]:appearance-none"
                  aria-label="Custom duration (1–120 min)"
                />
                <span className="text-[10px] font-mono text-text-muted">min</span>
              </div>
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
                  "w-full px-4 py-2.5 rounded-xl border font-mono text-xs sm:text-sm min-h-[48px] flex items-center justify-center gap-2 transition-colors touch-manipulation active:scale-[0.98]",
                  canAct
                    ? "border-accent-amber/30 text-accent-amber hover:bg-accent-amber/10"
                    : "border-surface-4 text-text-muted opacity-50 cursor-not-allowed"
                )}
                title={!isAdmin ? "Admin only" : "Start timed run — stops automatically when timer expires"}
              >
                <Timer size={14} />
                {busy === "countdown" ? "Starting…" : `Start ${customDurationInput || countdownMin} min Timer`}
              </button>
              <p className="text-[9px] font-mono text-text-muted text-center">
                Semi-auto — pump stops when timer expires, tank is full, or safety triggers
              </p>
            </div>
          )}

          {/* Stop for AUTO mode when running */}
          {controlMode === "AUTO" && runMode === "AUTO" && (
            <button
              type="button"
              disabled={busy !== null || controllerOffline}
              onClick={() => {
                setBusy("stop");
                try { onEmergencyStop(); } finally { window.setTimeout(() => setBusy(null), 8000); }
              }}
              className="w-full px-4 py-3 rounded-xl bg-accent-red/20 border border-accent-red/40 text-accent-red font-mono text-sm font-semibold hover:bg-accent-red/30 min-h-[56px] flex items-center justify-center gap-2 disabled:opacity-50 touch-manipulation active:scale-[0.98]"
              title="Emergency stop"
            >
              <AlertOctagon size={16} />
              Emergency Stop
            </button>
          )}

          {/* E-Stop button visible when running (any mode) */}
          {(runMode === "AUTO" || runMode === "COUNTDOWN" || runMode === "MANUAL_ON") && (
            <button
              type="button"
              disabled={busy !== null || controllerOffline}
              onClick={() => {
                setBusy("stop");
                try { onEmergencyStop(); } finally { window.setTimeout(() => setBusy(null), 8000); }
              }}
              className="w-full px-4 py-3 rounded-xl bg-accent-red border border-accent-red/80 text-white font-mono text-sm font-semibold hover:bg-accent-red/90 min-h-[56px] flex items-center justify-center gap-2 disabled:opacity-50 touch-manipulation active:scale-[0.98]"
              title="Emergency stop"
            >
              <AlertOctagon size={16} />
              Emergency Stop
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
