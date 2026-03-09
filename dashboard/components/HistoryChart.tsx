// components/HistoryChart.tsx
"use client";

import { useEffect, useMemo, useRef, useState } from "react";
import {
  ResponsiveContainer,
  AreaChart,
  Area,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
} from "recharts";
import type { HistoryEntry } from "@/lib/types";

interface HistoryChartProps {
  data: HistoryEntry[];
}

type TooltipPayloadItem = {
  name?: string;
  value?: number;
  color?: string;
};

interface CustomTooltipProps {
  active?: boolean;
  payload?: TooltipPayloadItem[];
  label?: string;
}

const CustomTooltip = ({ active, payload, label }: CustomTooltipProps) => {
  if (!active || !payload?.length) return null;
  const formatValue = (name: string, value: number) =>
    name.includes("Level") ? `${value}%` : `${value} LPM`;
  return (
    <div className="bg-surface-2 border border-surface-4 rounded-lg p-2.5 text-xs font-mono">
      <p className="text-text-secondary mb-1.5">{label}</p>
      {payload.map((p, idx) => (
        <p key={p.name ?? idx} style={{ color: p.color }}>
          {p.name}: {formatValue(p.name ?? "Value", Number(p.value ?? 0))}
        </p>
      ))}
    </div>
  );
};

export default function HistoryChart({ data }: HistoryChartProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [containerWidth, setContainerWidth] = useState<number>(0);

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;
    const ro = new ResizeObserver(() => setContainerWidth(el.clientWidth));
    ro.observe(el);
    setContainerWidth(el.clientWidth);
    return () => ro.disconnect();
  }, []);

  const tickInterval = useMemo(() => {
    // Heuristic: fewer X ticks on narrow screens to prevent overlap
    // interval means "show every Nth tick"
    const desiredTicks =
      containerWidth <= 0 ? 6
      : containerWidth < 360 ? 3
      : containerWidth < 520 ? 4
      : 6;

    return Math.max(1, Math.floor(data.length / desiredTicks));
  }, [containerWidth, data.length]);

  if (data.length < 2) {
    return (
      <div className="flex flex-col items-center justify-center min-h-[160px] h-[clamp(160px,35vh,220px)] gap-2 text-text-muted text-sm font-mono">
        <p>Waiting for data…</p>
        <div className="w-full max-w-[200px] h-24 rounded-lg border border-surface-4 bg-surface-2/50 animate-pulse" aria-hidden />
      </div>
    );
  }

  return (
    <div ref={containerRef} className="min-h-[160px] h-[clamp(160px,35vh,220px)] w-full min-w-0">
      <ResponsiveContainer width="100%" height="100%">
      <AreaChart data={data} margin={{ top: 8, right: 4, left: -24, bottom: 0 }}>
        <defs>
          <linearGradient id="levelGrad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="5%"  stopColor="#00E5FF" stopOpacity={0.25} />
            <stop offset="95%" stopColor="#00E5FF" stopOpacity={0}    />
          </linearGradient>
          <linearGradient id="flowGrad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="5%"  stopColor="#00FF88" stopOpacity={0.25} />
            <stop offset="95%" stopColor="#00FF88" stopOpacity={0}    />
          </linearGradient>
        </defs>

        <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.04)" />

        <XAxis
          dataKey="time"
          tick={{ fill: "#3D4F6B", fontSize: 10, fontFamily: "var(--font-jetbrains)" }}
          interval={tickInterval}
          axisLine={false}
          tickLine={false}
        />
        <YAxis
          tick={{ fill: "#3D4F6B", fontSize: 10, fontFamily: "var(--font-jetbrains)" }}
          axisLine={false}
          tickLine={false}
        />

        <Tooltip content={<CustomTooltip />} />

        <Legend
          wrapperStyle={{ fontSize: "clamp(9px, 2.2vw, 11px)", fontFamily: "var(--font-jetbrains)", color: "#7A8BA8" }}
          iconSize={8}
        />

        <Area
          type="monotone"
          dataKey="level"
          name="Water Level (%)"
          stroke="#00E5FF"
          strokeWidth={2}
          fill="url(#levelGrad)"
          dot={false}
          activeDot={{ r: 4, fill: "#00E5FF", strokeWidth: 0 }}
        />
        <Area
          type="monotone"
          dataKey="flow"
          name="Flow (LPM)"
          stroke="#00FF88"
          strokeWidth={2}
          fill="url(#flowGrad)"
          dot={false}
          activeDot={{ r: 4, fill: "#00FF88", strokeWidth: 0 }}
        />
      </AreaChart>
    </ResponsiveContainer>
    </div>
  );
}
