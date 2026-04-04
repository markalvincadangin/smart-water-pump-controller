"use client";

import clsx from "clsx";

interface MetricDisplayProps {
  label: string;
  value: string;
  isHighlighted?: boolean;
}

/**
 * Reusable metric row component for displaying label-value pairs.
 * Used in status cards to show telemetry or configuration values.
 */
export function MetricDisplay({ label, value, isHighlighted }: MetricDisplayProps) {
  return (
    <div className="flex items-end justify-between group border-b border-[var(--card-border)] border-dashed pb-2 last:border-0 last:pb-0">
      <span className="text-xs font-medium text-[var(--text-secondary)]">
        {label}
      </span>
      <span className={clsx(
        "font-mono text-sm font-semibold transition-colors tabular-nums tracking-tight",
        isHighlighted ? "text-sf-teal" : "text-[var(--text-primary)]"
      )}>
        {value}
      </span>
    </div>
  );
}
