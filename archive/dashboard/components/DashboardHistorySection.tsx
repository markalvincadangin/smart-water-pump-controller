"use client";

import clsx from "clsx";
import { RefreshCw } from "lucide-react";
import type { HistoryEntry, HistoryEvent } from "@/lib/types";
import HistoryChart from "@/components/HistoryChart";
import CollapsibleSection from "@/components/CollapsibleSection";

interface DashboardHistorySectionProps {
  connected: boolean;
  updateLabel: string;
  history: HistoryEntry[];
  pumpStartLevel?: number;
  pumpStopLevel?: number;
  events?: HistoryEvent[];
}

export default function DashboardHistorySection({ connected, updateLabel, history, pumpStartLevel, pumpStopLevel, events }: DashboardHistorySectionProps) {
  return (
    <div className="card p-4 sm:p-5 min-w-0 overflow-hidden">
      <CollapsibleSection
        title="Level & Flow"
        subtitle={`${history.length} readings · ${updateLabel}`}
        headerExtra={
          <div className="flex items-center gap-1.5 text-text-muted">
            <RefreshCw size={11} className={clsx(connected && "animate-spin-slow")} />
            <span className="text-[10px] sm:text-xs font-mono">Live</span>
          </div>
        }
      >
        <HistoryChart data={history} pumpStartLevel={pumpStartLevel} pumpStopLevel={pumpStopLevel} events={events} />
      </CollapsibleSection>
    </div>
  );
}
