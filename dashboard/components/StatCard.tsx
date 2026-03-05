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
}

const colorMap = {
  cyan:  { icon: "text-accent-cyan",  glow: "card-glow-cyan",  bg: "bg-accent-cyan/10"  },
  green: { icon: "text-accent-green", glow: "card-glow-green", bg: "bg-accent-green/10" },
  amber: { icon: "text-accent-amber", glow: "",                bg: "bg-accent-amber/10" },
  red:   { icon: "text-accent-red",   glow: "card-glow-red",   bg: "bg-accent-red/10"   },
};

export default function StatCard({
  label, value, unit, Icon, color, sub, animate,
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

      <div className="flex items-end gap-1">
        <span className={clsx(
          "text-2xl sm:text-3xl font-display font-bold tabular-nums leading-none",
          color === "cyan"  && "text-gradient-cyan",
          color === "green" && "text-gradient-green",
          color === "amber" && "text-accent-amber",
          color === "red"   && "text-accent-red"
        )}>
          {value}
        </span>
        {unit && (
          <span className="text-sm text-text-secondary mb-0.5 font-mono">{unit}</span>
        )}
      </div>

      {sub && (
        <span className="text-xs text-text-muted font-mono">{sub}</span>
      )}
    </div>
  );
}
