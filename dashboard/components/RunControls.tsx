"use client";

import { useEffect, useMemo, useState } from "react";
import clsx from "clsx";
import { Timer, Play, Square } from "lucide-react";

type RunMode = "OFF" | "AUTO" | "MANUAL" | "TIMED";
type ControlMode = "AUTO" | "FORCE_ON" | "FORCE_OFF";

interface RunControlsProps {
  runMode: RunMode;
  remainingSec: number;
  controlMode: ControlMode;
  isError: boolean;
  isAdmin: boolean;
  lastFaultCode?: string;
  onStartManual: () => void;
  onStartTimed: (durationSec: number) => void;
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
  isAdmin,
  lastFaultCode,
  onStartManual,
  onStartTimed,
  onStop,
}: RunControlsProps) {
  const [busy, setBusy] = useState<"manual" | "timed" | "stop" | null>(null);
  const [timedMin, setTimedMin] = useState<number>(10);

  // Local countdown so UI stays smooth even if status updates are delayed.
  const [localRemainingSec, setLocalRemainingSec] = useState<number | null>(null);

  // Snap local countdown to controller value whenever a fresh update arrives
  useEffect(() => {
    if (runMode === "TIMED") {
      setLocalRemainingSec(remainingSec);
    } else {
      setLocalRemainingSec(null);
    }
  }, [runMode, remainingSec]);

  // Tick local countdown every second between updates
  useEffect(() => {
    if (runMode !== "TIMED" || localRemainingSec == null || localRemainingSec <= 0) return;
    const id = window.setInterval(() => {
      setLocalRemainingSec((prev) => (prev != null && prev > 0 ? prev - 1 : prev));
    }, 1000);
    return () => window.clearInterval(id);
  }, [runMode, localRemainingSec]);

  const showStop = runMode === "MANUAL" || runMode === "TIMED";
  const effectiveRemaining = localRemainingSec ?? remainingSec;
  const countdown = useMemo(
    () => (runMode === "TIMED" ? formatMmSs(effectiveRemaining) : null),
    [effectiveRemaining, runMode]
  );
  const blockedByForceOff = controlMode === "FORCE_OFF" && !showStop;
  const isDryRunLockout = lastFaultCode === "DRY_RUN";

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between">
        <h3 className="font-display font-semibold text-text-primary text-sm uppercase tracking-widest">
          Run Pump
        </h3>
        <span
          className={clsx(
            "badge border font-mono",
            runMode === "TIMED" ? "bg-accent-amber/10 text-accent-amber border-accent-amber/20"
              : runMode === "MANUAL" ? "bg-accent-green/10 text-accent-green border-accent-green/20"
                : runMode === "AUTO" ? "bg-accent-cyan/10 text-accent-cyan border-accent-cyan/20"
                  : "bg-surface-3 text-text-secondary border-surface-4"
          )}
          title="Run mode reported by the controller"
        >
          {runMode === "OFF" ? "OFF" : runMode}
          {countdown ? ` · ${countdown}` : ""}
        </span>
      </div>

      {controlMode === "FORCE_ON" && (
        <div className="p-3 rounded-xl bg-accent-amber/10 border border-accent-amber/25">
          <p className="text-xs font-mono text-accent-amber font-semibold uppercase tracking-wide">
            ⚠ Emergency override active (FORCE ON)
          </p>
          <p className="text-[11px] font-mono text-text-secondary mt-1">
            Pump is running continuously. Prefer stopping this and using <span className="text-text-primary">Manual</span> or <span className="text-text-primary">Timed</span> runs for normal operation.
          </p>
        </div>
      )}

      {blockedByForceOff && (
        <div className="p-3 rounded-xl bg-accent-red/10 border border-accent-red/25">
          <p className="text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
            FORCE OFF is active
          </p>
          <p className="text-[11px] font-mono text-text-secondary mt-1">
            Switch Mode to <span className="text-text-primary">AUTO</span> (or Emergency ON) to start a run.
          </p>
        </div>
      )}

      {isDryRunLockout && (
        <div className="p-3 rounded-xl bg-accent-red/10 border border-accent-red/25">
          <p className="text-xs font-mono text-accent-red font-semibold uppercase tracking-wide">
            Dry-run lockout active
          </p>
          <p className="text-[11px] font-mono text-text-secondary mt-1">
            Pump was stopped due to low flow protection. Acknowledge the error in the Errors &amp; Alerts panel before starting a new run.
          </p>
        </div>
      )}

      <div className="grid grid-cols-1 sm:grid-cols-2 gap-2">
        <button
          type="button"
          disabled={busy !== null || isError || !isAdmin || blockedByForceOff || isDryRunLockout}
          onClick={async () => {
            setBusy("manual");
            try { onStartManual(); } finally { window.setTimeout(() => setBusy(null), 1200); }
          }}
          className={clsx(
            "px-4 py-3 rounded-xl border font-mono text-sm min-h-[44px] flex items-center justify-center gap-2 transition-colors",
            "border-surface-4 text-text-secondary hover:bg-surface-3",
            (!isAdmin || isError || blockedByForceOff) && "opacity-50 cursor-not-allowed"
          )}
          title={
            !isAdmin ? "Admin only"
              : blockedByForceOff ? "FORCE OFF is active"
                : isDryRunLockout ? "Dry-run lockout: acknowledge in Errors & Alerts before starting"
                  : isError ? "Clear error before starting"
                  : "Start a manual run (stop anytime)"
          }
        >
          <Play size={14} />
          Quick start (Manual)
        </button>

        <button
          type="button"
          disabled={busy !== null || isError || !isAdmin || blockedByForceOff || isDryRunLockout}
          onClick={async () => {
            setBusy("timed");
            try { onStartTimed(timedMin * 60); } finally { window.setTimeout(() => setBusy(null), 1200); }
          }}
          className={clsx(
            "px-4 py-3 rounded-xl border font-mono text-sm min-h-[44px] flex items-center justify-center gap-2 transition-colors",
            "border-surface-4 text-text-secondary hover:bg-surface-3",
            (!isAdmin || isError || blockedByForceOff) && "opacity-50 cursor-not-allowed"
          )}
          title={
            !isAdmin ? "Admin only"
              : blockedByForceOff ? "FORCE OFF is active"
                : isDryRunLockout ? "Dry-run lockout: acknowledge in Errors & Alerts before starting"
                  : isError ? "Clear error before starting"
                  : "Run for a fixed time then auto-stop"
          }
        >
          <Timer size={14} />
          Timed run
        </button>
      </div>

      <div className="flex items-center justify-between gap-2">
        <label className="text-[11px] font-mono text-text-muted">
          Duration
        </label>
        <select
          value={timedMin}
          onChange={(e) => setTimedMin(parseInt(e.target.value, 10))}
          className="px-3 py-2 rounded-lg bg-surface-2 border border-surface-4 text-text-primary font-mono text-sm"
          disabled={busy !== null || showStop}
          title={showStop ? "Stop the current run to change duration" : "Pick a timed run duration"}
        >
          {PRESETS_MIN.map((m) => (
            <option key={m} value={m}>{m} min</option>
          ))}
        </select>
      </div>

      {showStop && (
        <button
          type="button"
          disabled={busy !== null}
          onClick={async () => {
            setBusy("stop");
            try { onStop(); } finally { window.setTimeout(() => setBusy(null), 1200); }
          }}
          className="w-full px-4 py-3 rounded-xl bg-accent-red/20 border border-accent-red/40 text-accent-red font-mono text-sm font-semibold hover:bg-accent-red/30 min-h-[44px] flex items-center justify-center gap-2 disabled:opacity-50"
          title="Stop the pump now"
        >
          <Square size={14} />
          Stop
        </button>
      )}

      {!isAdmin && (
        <p className="text-[10px] font-mono text-text-muted">
          Run controls are admin-only.
        </p>
      )}
      {isError && (
        <p className="text-[10px] font-mono text-text-muted">
          Clear the error (Acknowledge) before starting a run.
        </p>
      )}
      <p className="text-[10px] font-mono text-text-muted">
        Safety protections stay active (dry-run, overflow, sensor fail-safe). Use Emergency override only for troubleshooting.
      </p>
    </div>
  );
}

