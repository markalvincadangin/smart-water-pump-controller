// components/ModeControls.tsx — Part 7.3 mode selector (radiogroup, segmented)
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
  /** When true, segments stay visible at 38% opacity and do not accept input (Part 7.3) */
  controlsLocked?: boolean;
}

const NORMAL_MODES: {
  id: PumpControl["mode"];
  label: string;
  sub: string;
  Icon: React.ElementType;
}[] = [
  { id: "AUTO", label: "AUTO", sub: "Level-based", Icon: Cpu },
  { id: "MANUAL", label: "MANUAL", sub: "Override", Icon: Hand },
  { id: "COUNTDOWN", label: "TIMER", sub: "Timed run", Icon: Timer },
];

export default function ModeControls({
  currentMode,
  isError,
  dryRunTimeoutSec = 30,
  pendingMode = null,
  pendingAcknowledge = false,
  onSetMode,
  onAcknowledge,
  controlsLocked = false,
}: ModeControlsProps) {
  const segmentDisabled = pendingMode !== null || controlsLocked || isError;

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between gap-2">
        <h3 className="section-label text-text-primary">Mode</h3>
        {pendingMode ? (
          <span
            className="badge border border-accent-amber/30 bg-accent-amber/10 text-accent-amber"
            role="status"
          >
            <span className="font-mono text-xs font-medium">Sending…</span>
          </span>
        ) : (
          <span className="badge border border-surface-4 bg-surface-2 text-text-muted">
            <span className="font-mono text-xs font-medium">Synced</span>
          </span>
        )}
      </div>

      <div
        role="radiogroup"
        aria-label="Pump control mode"
        className={clsx(
          "flex w-full gap-0.5 rounded-lg bg-surface-2 p-[3px]",
          (isError || controlsLocked) && "opacity-[0.38]"
        )}
      >
        {NORMAL_MODES.map(({ id, label, sub, Icon }) => {
          const isActive = currentMode === id;
          const isPending = pendingMode === id;
          return (
            <button
              key={id}
              type="button"
              role="radio"
              aria-checked={isActive}
              aria-disabled={segmentDisabled}
              disabled={segmentDisabled}
              onClick={() => onSetMode(id)}
              title={`${label}: ${sub}`}
              className={clsx(
                "mode-segment flex min-h-[44px] flex-1 flex-col items-center justify-center gap-0.5 rounded px-1 py-2",
                "touch-manipulation select-none transition-colors duration-150 ease-out",
                "focus-visible:outline-none focus-visible:ring-[3px] focus-visible:ring-[rgb(var(--c-border-focus)/0.45)] focus-visible:ring-offset-2 focus-visible:ring-offset-[rgb(var(--c-bg-surface))]",
                isActive
                  ? "bg-[rgb(var(--c-brand-500))] text-white shadow-sm"
                  : "bg-transparent text-text-secondary hover:bg-surface-1/60"
              )}
            >
              <Icon size={14} className={clsx(isActive ? "text-white" : "text-text-muted")} />
              <span className="font-mono text-[11px] font-semibold leading-none sm:text-xs">{label}</span>
              <span
                className={clsx(
                  "hidden text-[10px] leading-tight sm:block",
                  isActive ? "text-white/85" : "text-text-muted",
                  isPending && "text-accent-amber"
                )}
              >
                {isPending ? "Sending…" : sub}
              </span>
            </button>
          );
        })}
      </div>

      {isError && (
        <div
          role="alert"
          className="rounded-lg border border-accent-red/25 border-l-[3px] border-l-accent-red bg-accent-red/10 p-4"
        >
          <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
            <div className="min-w-0">
              <p className="font-body text-sm font-semibold text-accent-red">Pump stopped — no water flow</p>
              <p className="mt-1 max-w-[28ch] font-body text-body text-text-secondary">
                No flow for {dryRunTimeoutSec}s. Check pump and water source.
              </p>
            </div>
            <button
              type="button"
              onClick={onAcknowledge}
              disabled={pendingAcknowledge}
              className="min-h-[44px] shrink-0 rounded-lg border border-accent-red/50 bg-accent-red/20 px-4 py-2.5 font-mono text-xs font-semibold text-accent-red transition-colors duration-150 ease-out hover:bg-accent-red/30 focus-visible:outline-none focus-visible:ring-[3px] focus-visible:ring-[rgb(var(--c-border-focus)/0.45)] disabled:cursor-not-allowed disabled:opacity-[0.38]"
            >
              {pendingAcknowledge ? "Sending…" : "Acknowledge"}
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
