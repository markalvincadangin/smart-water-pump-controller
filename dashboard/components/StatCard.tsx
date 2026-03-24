// components/StatCard.tsx — Part 2 (data = Geist Mono), Part 3 spacing (24px card padding)
"use client";

import clsx from "clsx";
import type { LucideIcon } from "lucide-react";

interface StatCardProps {
  label: string;
  value: string;
  unit?: string;
  Icon: LucideIcon;
  color: "cyan" | "green" | "amber" | "red";
  sub?: string;
  animate?: boolean;
  proximity?: number;
  proximityLabel?: string;
}

const colorMap = {
  cyan: { icon: "text-accent-cyan", bg: "bg-[rgb(var(--c-brand-500)/0.1)]" },
  green: { icon: "text-accent-green", bg: "bg-[rgb(var(--c-status-ok)/0.1)]" },
  amber: { icon: "text-accent-amber", bg: "bg-[rgb(var(--c-status-warn)/0.1)]" },
  red: { icon: "text-accent-red", bg: "bg-[rgb(var(--c-status-error)/0.1)]" },
};

const valueTone: Record<StatCardProps["color"], string> = {
  cyan: "text-accent-cyan",
  green: "text-accent-green",
  amber: "text-accent-amber",
  red: "text-accent-red",
};

export default function StatCard({
  label,
  value,
  unit,
  Icon,
  color,
  sub,
  animate,
  proximity,
  proximityLabel,
}: StatCardProps) {
  const c = colorMap[color];
  const hasProximity = typeof proximity === "number" && proximity >= 0 && !!proximityLabel;
  const proximityCapped = hasProximity ? Math.max(0, Math.min(1.5, proximity as number)) : 0;
  const proximityWidthPct = hasProximity ? (proximityCapped / 1.5) * 100 : 0;
  const proximityBarColor =
    !hasProximity
      ? "bg-accent-green"
      : (proximity as number) < 1
        ? "bg-accent-red"
        : (proximity as number) < 1.5
          ? "bg-accent-amber"
          : "bg-accent-green";

  return (
    <div className="card flex min-w-0 flex-col gap-3 p-6">
      <div className="flex items-start justify-between gap-2">
        <span className="section-label text-text-secondary">{label}</span>
        <div className={clsx("rounded-md p-1.5", c.bg)}>
          <Icon size={14} className={clsx(c.icon, animate && "animate-pulse-slow")} />
        </div>
      </div>

      <div className="flex flex-col gap-2">
        <div className="flex min-h-[2.5rem] items-end gap-1 tabular-nums">
          <span className={clsx("font-mono text-metric font-semibold leading-none tracking-tight", valueTone[color])}>
            {value}
          </span>
          {unit && <span className="mb-0.5 font-mono text-xs text-text-unit">{unit}</span>}
        </div>

        {hasProximity && (
          <div className="flex items-center gap-2">
            <div className="h-2 flex-1 overflow-hidden rounded-full border border-border-subtle bg-surface-2">
              <div
                className={clsx("h-full rounded-full transition-[width] duration-150 ease-out", proximityBarColor)}
                style={{ width: `${proximityWidthPct}%` }}
              />
            </div>
            <span className="font-mono text-xs text-text-unit">{proximityLabel}</span>
          </div>
        )}

        {sub && <p className="font-mono text-xs leading-snug text-text-secondary">{sub}</p>}
      </div>
    </div>
  );
}
