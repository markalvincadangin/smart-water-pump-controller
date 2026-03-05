// components/HistoryChart.tsx
"use client";

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

const CustomTooltip = ({ active, payload, label }: any) => {
  if (!active || !payload?.length) return null;
  const formatValue = (name: string, value: number) =>
    name.includes("Level") ? `${value}%` : `${value} LPM`;
  return (
    <div className="bg-surface-2 border border-surface-4 rounded-lg p-2.5 text-xs font-mono">
      <p className="text-text-secondary mb-1.5">{label}</p>
      {payload.map((p: any) => (
        <p key={p.name} style={{ color: p.color }}>
          {p.name}: {formatValue(p.name, p.value)}
        </p>
      ))}
    </div>
  );
};

export default function HistoryChart({ data }: HistoryChartProps) {
  if (data.length < 2) {
    return (
      <div className="flex flex-col items-center justify-center h-40 gap-2 text-text-muted text-sm font-mono">
        <p>Waiting for data…</p>
        <div className="w-full max-w-[200px] h-24 rounded-lg border border-surface-4 bg-surface-2/50 animate-pulse" aria-hidden />
      </div>
    );
  }

  // Show only every N-th label on X axis to avoid crowding
  const tickInterval = Math.max(1, Math.floor(data.length / 6));

  return (
    <ResponsiveContainer width="100%" height={180}>
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
          wrapperStyle={{ fontSize: 11, fontFamily: "var(--font-jetbrains)", color: "#7A8BA8" }}
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
  );
}
