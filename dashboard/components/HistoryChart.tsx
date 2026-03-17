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
  ReferenceLine,
} from "recharts";
import type { HistoryEntry, HistoryEvent } from "@/lib/types";

interface HistoryChartProps {
  data: HistoryEntry[];
  pumpStartLevel?: number;
  pumpStopLevel?: number;
  events?: HistoryEvent[];
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
  eventsAtLabel?: HistoryEvent[];
}

const CustomTooltip = ({ active, payload, label, eventsAtLabel }: CustomTooltipProps) => {
  if (!active || !payload?.length) return null;
  return (
    <div className="bg-surface-2 border border-surface-4 rounded-lg p-2.5 text-xs font-mono shadow-lg"
      style={{ animation: "tooltipIn 0.15s ease-out" }}>
      <p className="text-text-secondary mb-1.5 text-[10px]">{label}</p>
      {payload.map((p, idx) => (
        <p key={p.name ?? idx} className="flex items-center gap-2">
          <span className="w-2 h-2 rounded-full shrink-0" style={{ backgroundColor: p.color }} />
          <span style={{ color: p.color }}>
            {p.name}: {p.name?.includes("Level") ? `${p.value}%` : `${Number(p.value ?? 0).toFixed(1)} L/min`}
          </span>
        </p>
      ))}
      {eventsAtLabel && eventsAtLabel.length > 0 && (
        <div className="mt-1.5 border-t border-surface-4 pt-1.5">
          <p className="text-[9px] text-text-muted mb-0.5">Events</p>
          {eventsAtLabel.map((evt, idx) => (
            <p key={`${evt.type}-${idx}`} className="text-[9px] text-text-secondary">
              {evt.type === "fault" && `Fault: ${evt.faultCode ?? "P1 safety"}`}
              {evt.type === "mode_change" &&
                `Mode: ${evt.prevRunMode ?? "?"} → ${evt.runMode ?? "?"}`}
              {evt.type === "run_start" && `Run: START (${evt.runMode ?? ""})`}
              {evt.type === "run_stop" && `Run: STOP (${evt.runMode ?? ""})`}
            </p>
          ))}
        </div>
      )}
    </div>
  );
};

export default function HistoryChart({ data, pumpStartLevel, pumpStopLevel, events }: HistoryChartProps) {
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
    const desiredTicks =
      containerWidth <= 0 ? 6
      : containerWidth < 360 ? 3
      : containerWidth < 520 ? 4
      : 6;
    return Math.max(1, Math.floor(data.length / desiredTicks));
  }, [containerWidth, data.length]);

  const maxFlow = useMemo(() => {
    const peak = Math.max(...data.map((d) => d.flow), 5);
    return Math.ceil(peak / 5) * 5;
  }, [data]);

  if (data.length < 2) {
    return (
      <div className="flex flex-col items-center justify-center min-h-[180px] h-[clamp(180px,35vh,280px)] gap-2 text-text-muted text-sm font-mono">
        <p>Waiting for data…</p>
        <div className="w-full max-w-[200px] h-24 rounded-lg border border-surface-4 bg-surface-2/50 animate-pulse" aria-hidden />
      </div>
    );
  }

  return (
    <div ref={containerRef} className="min-h-[180px] h-[clamp(180px,40vh,320px)] w-full min-w-0">
      <ResponsiveContainer width="100%" height="100%">
        <AreaChart data={data} margin={{ top: 8, right: 40, left: -20, bottom: 0 }}>
          <defs>
            <linearGradient id="levelGrad" x1="0" y1="0" x2="0" y2="1">
              <stop offset="5%"  stopColor="#00E5FF" stopOpacity={0.2} />
              <stop offset="95%" stopColor="#00E5FF" stopOpacity={0}   />
            </linearGradient>
            <linearGradient id="flowGrad" x1="0" y1="0" x2="0" y2="1">
              <stop offset="5%"  stopColor="#00FF88" stopOpacity={0.2} />
              <stop offset="95%" stopColor="#00FF88" stopOpacity={0}   />
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

          {/* Left Y-axis: Water level 0-100% */}
          <YAxis
            yAxisId="level"
            domain={[0, 100]}
            tick={{ fill: "#4A6080", fontSize: 9, fontFamily: "var(--font-jetbrains)" }}
            axisLine={false}
            tickLine={false}
            tickFormatter={(v: number) => `${v}%`}
          />

          {/* Right Y-axis: Flow rate (auto-scaled) */}
          <YAxis
            yAxisId="flow"
            orientation="right"
            domain={[0, maxFlow]}
            tick={{ fill: "#3D6050", fontSize: 9, fontFamily: "var(--font-jetbrains)" }}
            axisLine={false}
            tickLine={false}
            tickFormatter={(v: number) => `${v}`}
          />

          <Tooltip
            content={({ active, payload, label }) => {
              const eventsAtLabel =
                events && label
                  ? events.filter((evt) => evt.time === label)
                  : [];
              return (
                <CustomTooltip
                  active={active}
                  payload={payload as TooltipPayloadItem[] | undefined}
                  label={label as string | undefined}
                  eventsAtLabel={eventsAtLabel}
                />
              );
            }}
          />

          {typeof pumpStartLevel === "number" && (
            <ReferenceLine
              yAxisId="level"
              y={pumpStartLevel}
              stroke="#F5B800"
              strokeDasharray="4 4"
              strokeOpacity={0.5}
              label={{ value: `Start ${pumpStartLevel}%`, position: "insideTopRight", fill: "#F5B800", fontSize: 9 }}
            />
          )}
          {typeof pumpStopLevel === "number" && (
            <ReferenceLine
              yAxisId="level"
              y={pumpStopLevel}
              stroke="#00FF88"
              strokeDasharray="4 4"
              strokeOpacity={0.5}
              label={{ value: `Full ${pumpStopLevel}%`, position: "insideBottomRight", fill: "#00FF88", fontSize: 9 }}
            />
          )}

          <Legend
            wrapperStyle={{ fontSize: "clamp(9px, 2.2vw, 11px)", fontFamily: "var(--font-jetbrains)", color: "#7A8BA8" }}
            iconSize={8}
          />

          {/* Event markers: safety, mode changes, and start/stop edges */}
          {events?.map((evt, idx) => (
            <ReferenceLine
              key={`${evt.type}-${idx}-${evt.time}`}
              x={evt.time}
              stroke={
                evt.type === "fault"
                  ? "#FF4B6E"
                  : evt.type === "mode_change"
                    ? "#F5B800"
                    : "#5C6FFF"
              }
              strokeDasharray={evt.type === "mode_change" ? "4 4" : "2 3"}
              strokeOpacity={0.6}
            />
          ))}

          <Area
            yAxisId="level"
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
            yAxisId="flow"
            type="monotone"
            dataKey="flow"
            name="Flow (L/min)"
            stroke="#00FF88"
            strokeWidth={1.5}
            fill="url(#flowGrad)"
            dot={false}
            activeDot={{ r: 3, fill: "#00FF88", strokeWidth: 0 }}
          />
        </AreaChart>
      </ResponsiveContainer>
    </div>
  );
}
