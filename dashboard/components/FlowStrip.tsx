"use client";

import clsx from "clsx";

interface FlowStripProps {
  flowLpm: number;
  running: boolean;
  dryRunThresholdLpm: number;
  className?: string;
}

/**
 * Part 6.2 — Flow strip: 80px height, flush panel, primary flow readout (Geist Mono).
 */
export default function FlowStrip({ flowLpm, running, dryRunThresholdLpm, className }: FlowStripProps) {
  const lowFlow = running && dryRunThresholdLpm > 0 && flowLpm < dryRunThresholdLpm;

  return (
    <div
      className={clsx(
        "flex h-20 min-h-[80px] shrink-0 items-center justify-between gap-4 px-6",
        "bg-surface-1 border-t border-border-faint",
        className
      )}
    >
      <div className="min-w-0">
        <p className="section-label mb-1">Flow rate</p>
        <p className="hidden font-body text-body text-text-secondary sm:block">
          {running ? (lowFlow ? "Below dry-run threshold" : "Live sensor") : "Pump idle"}
        </p>
      </div>
      <div className="flex items-baseline gap-1 tabular-nums">
        <span
          className={clsx(
            "font-mono text-metric font-semibold leading-none tracking-tight",
            lowFlow
              ? "text-accent-red"
              : running
                ? "text-accent-green"
                : "text-text-muted"
          )}
        >
          {flowLpm.toFixed(1)}
        </span>
        <span className="font-mono text-xs text-text-unit">L/min</span>
      </div>
    </div>
  );
}
