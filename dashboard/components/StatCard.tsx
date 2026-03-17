// components/StatCard.tsx
"use client";

import clsx from "clsx";
import type { LucideIcon } from "lucide-react";

interface StatCardProps {
  label:     string;
  value:     string;
  unit?:     string;
  Icon:      LucideIcon;
  color:     "cyan" | "green" | "amber" | "red";
  sub?:      string;
  animate?:  boolean;
  /** Optional proximity bar (0–1) for threshold-style stats like flow vs dry-run threshold. */
  proximity?: number;
  proximityLabel?: string;
}

const colorMap = {
  cyan:  { icon: "text-accent-cyan",  glow: "card-glow-cyan",  bg: "bg-accent-cyan/10"  },
  green: { icon: "text-accent-green", glow: "card-glow-green", bg: "bg-accent-green/10" },
  amber: { icon: "text-accent-amber", glow: "",                bg: "bg-accent-amber/10" },
  red:   { icon: "text-accent-red",   glow: "card-glow-red",   bg: "bg-accent-red/10"   },
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

  return (
    <div className={clsx("card p-3 sm:p-4 flex flex-col gap-2 sm:gap-3 min-w-0", c.glow)}>
      <div className="flex items-center justify-between">
        <span className="text-xs font-mono text-text-secondary uppercase tracking-widest">
          {label}
        </span>
        <div className={clsx("p-1.5 rounded-lg", c.bg)}>
          <Icon
            size={14}
            className={clsx(c.icon, animate && "animate-pulse-slow")}
          />
        </div>
      </div>

      <div className="flex flex-col gap-1">
        <div className="flex items-end gap-1">
          <span
            className={clsx(
              "text-2xl sm:text-3xl font-display font-bold tabular-nums leading-none",
              color === "cyan" && "text-gradient-cyan",
              color === "green" && "text-gradient-green",
              color === "amber" && "text-accent-amber",
              color === "red" && "text-accent-red"
            )}
          >
            {value}
          </span>
          {unit && (
            <span className="text-xs sm:text-sm text-text-secondary mb-0.5 font-mono">
              {unit}
            </span>
          )}
        </div>

        {typeof proximity === "number" && proximity >= 0 && proximityLabel && (
          <div className="flex items-center gap-2">
            <div className="flex-1 h-1.5 rounded-full bg-surface-3 overflow-hidden">
              <div
                className={clsx(
                  "h-full rounded-full transition-all duration-200",
                  proximity < 1 ? "bg-accent-green" : "bg-accent-red"
                )}
                style={{ width: `${Math.min(1, proximity) * 100}%` }}
              />
            </div>
            <span className="text-[10px] font-mono text-text-muted">{proximityLabel}</span>
          </div>
        )}

        {sub && (
          <span className="text-xs text-text-muted font-mono">{sub}</span>
        )}
      </div>
    </div>
  );
}
