"use client";

import clsx from "clsx";
import { RefreshCw } from "lucide-react";
import type { HistoryEntry } from "@/lib/types";
import HistoryChart from "@/components/HistoryChart";

interface DashboardHistorySectionProps {
  connected: boolean;
  updateLabel: string;
  history: HistoryEntry[];
}

export default function DashboardHistorySection({ connected, updateLabel, history }: DashboardHistorySectionProps) {
  return (
    <div className="card p-4 sm:p-5 card-glow-cyan min-w-0 overflow-hidden">
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2 mb-3 sm:mb-4">
        <div>
          <h3 className="font-display font-semibold text-sm uppercase tracking-widest text-text-primary">
            Level & Flow History
          </h3>
          <p className="text-[10px] sm:text-xs font-mono text-text-muted mt-0.5">
            Last {history.length} readings · {updateLabel}
          </p>
        </div>
        <div className="flex items-center gap-2 text-text-muted">
          <RefreshCw size={12} className={clsx(connected && "animate-spin-slow")} />
          <span className="text-xs font-mono">Real-time</span>
        </div>
      </div>
      <HistoryChart data={history} />
    </div>
  );
}

