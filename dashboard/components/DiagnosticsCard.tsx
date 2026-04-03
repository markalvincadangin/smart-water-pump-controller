"use client";

import React, { useState } from "react";
import { ChevronDown, ChevronUp, Cpu, Network, Database, Terminal, AlertCircle } from "lucide-react";
import clsx from "clsx";

interface DiagnosticsCardProps {
  // System Health
  freeHeap: number;
  minFreeHeap: number;
  uptime: string;
  bootReason: string;
  totalCycles: number;
  totalRunTime: string;

  // RS-485 / Sensor details
  isSensorStable: boolean;
  isLevelFresh: boolean;
  sensorHealth: number;
  lastGoodDistanceCm: number;
  levelDiscardCount: number;

  // Firebase
  isFirebaseConnected: boolean;
  fbLastError?: string;
  fbConsecFailures: number;
  currentLogLevel: number;

  // Actions
  onSetLogLevel: (level: number) => void;
  isLoading?: boolean;
}

const LOG_LEVEL_LABELS = ['ERROR', 'WARN', 'INFO', 'DEBUG', 'VERBOSE'];

/**
 * REFACTOR [D4.6]: System Diagnostics
 * High-density diagnostic telemetry and remote hardware configuration.
 */
export default function DiagnosticsCard({
  freeHeap,
  minFreeHeap,
  uptime,
  bootReason,
  totalCycles,
  totalRunTime,
  isSensorStable,
  isLevelFresh,
  sensorHealth,
  lastGoodDistanceCm,
  levelDiscardCount,
  isFirebaseConnected,
  fbLastError,
  fbConsecFailures,
  currentLogLevel,
  onSetLogLevel,
  isLoading = false,
}: DiagnosticsCardProps) {
  const [isExpanded, setIsExpanded] = useState(false);

  if (isLoading) {
    return <div className="h-14 w-full skeleton rounded-card" />;
  }

  return (
    <div className="card transition-all duration-300">
      {/* Header / Toggle */}
      <button 
        onClick={() => setIsExpanded(!isExpanded)}
        className="w-full flex items-center justify-between p-6 group"
      >
        <div className="flex flex-col items-start gap-1">
          <div className="flex items-center gap-3">
            <Terminal size={18} className="text-sf-blue opacity-70 group-hover:opacity-100 transition-opacity" />
            <h3 className="text-sm font-semibold uppercase tracking-wider text-[var(--text-muted)] m-0">System Health & Telemetry</h3>
          </div>
          <p className="text-[10px] text-[var(--text-muted)] opacity-70 ml-[30px] text-left leading-relaxed">
            Raw diagnostic data from the controller. Intended for hardware debugging.
          </p>
        </div>
        <div className="text-[var(--text-muted)] shrink-0 ml-4">
          {isExpanded ? <ChevronUp size={20} /> : <ChevronDown size={20} />}
        </div>
      </button>

      {/* Expanded Content */}
      {isExpanded && (
        <div className="px-6 pb-6 space-y-8 animate-fade-in border-t border-[var(--card-border)]/20 pt-6">
          
          {/* Section: System Health */}
          <DiagSection 
            icon={<Cpu size={14} />} 
            title="System Health"
            items={[
              { label: "Free Heap", value: `${(freeHeap / 1024).toFixed(1)} KB` },
              { label: "Min Observed", value: `${(minFreeHeap / 1024).toFixed(1)} KB`, isDanger: minFreeHeap < 20480 },
              { label: "Uptime", value: uptime },
              { label: "Boot Reason", value: bootReason },
              { label: "Total Cycles", value: totalCycles.toString() },
              { label: "Total Run", value: totalRunTime },
            ]}
          />

          {/* Section: RS-485 / Sensor */}
          <DiagSection 
            icon={<Network size={14} />} 
            title="RS-485 / Sensor"
            items={[
              { label: "Sensor Stable", value: isSensorStable ? "✓ Yes" : "✗ No", isDanger: !isSensorStable },
              { label: "Level Fresh", value: isLevelFresh ? "✓ Yes" : "✗ No", isWarning: !isLevelFresh },
              { label: "Health %", value: `${sensorHealth}%`, isWarning: sensorHealth < 90 },
              { label: "Last Good", value: `${lastGoodDistanceCm.toFixed(1)} cm` },
              { label: "Discards / window", value: levelDiscardCount.toString(), isWarning: levelDiscardCount > 0 },
            ]}
          />

          <div className="text-[9px] text-[var(--text-muted)] opacity-80 ml-3 -mt-1 leading-relaxed">
            LDSC is a per-measurement-window snapshot (resets frequently, roughly once per ultrasonic window). It is not cumulative tank health.
          </div>

          {/* Section: Firebase */}
          <DiagSection 
            icon={<Database size={14} />} 
            title="Cloud Connectivity"
            items={[
              { label: "Connection", value: isFirebaseConnected ? "✓ Live" : "✗ Offline", isDanger: !isFirebaseConnected },
              { label: "Consec. Fails", value: fbConsecFailures.toString(), isWarning: fbConsecFailures > 0 },
              { label: "Last Error", value: fbLastError || "(none)", isDanger: !!fbLastError },
            ]}
          />

          {/* Log Level Control */}
          <div className="space-y-3">
             <div className="flex items-center gap-2">
                <Terminal size={14} className="text-[var(--text-muted)]" />
                <span className="text-[10px] font-bold uppercase tracking-widest text-[var(--text-secondary)]">Remote Log Level</span>
             </div>
             <div className="flex bg-[var(--card-bg)] rounded-md p-1 border border-[var(--card-border)]">
                {LOG_LEVEL_LABELS.map((label, i) => (
                  <button
                    key={label}
                    onClick={() => onSetLogLevel(i)}
                    className={clsx(
                      "flex-1 py-1.5 text-[9px] font-bold tracking-tight rounded-md transition-all uppercase",
                      currentLogLevel === i 
                        ? "bg-[var(--text-primary)] text-[var(--card-bg)] shadow-sm" 
                        : "text-[var(--text-muted)] hover:text-[var(--text-secondary)]"
                    )}
                  >
                    {label}
                  </button>
                ))}
             </div>
             {currentLogLevel > 2 && (
               <div className="flex items-start gap-2 text-sf-amber animate-slide-up px-1">
                  <AlertCircle size={12} className="shrink-0 mt-0.5" />
                  <p className="text-[9px] font-medium leading-tight italic">
                    Verbose logging may increase Firebase bandwidth.
                  </p>
               </div>
             )}
          </div>

        </div>
      )}
    </div>
  );
}

interface DiagItem {
  label: string;
  value: string;
  isDanger?: boolean;
  isWarning?: boolean;
}

function DiagSection({ icon, title, items }: { icon: React.ReactNode, title: string, items: DiagItem[] }) {
  return (
    <div className="space-y-4">
      <div className="flex items-center gap-2 border-l-2 border-sf-blue pl-3">
        <span className="text-[var(--text-muted)]">{icon}</span>
        <h4 className="text-[10px] font-bold uppercase tracking-wide text-[var(--text-secondary)]">{title}</h4>
      </div>
      <div className="grid grid-cols-2 gap-x-6 gap-y-3">
        {items.map((item) => (
          <div key={item.label} className="flex flex-col">
            <span className="text-[9px] font-mono text-[var(--text-muted)] uppercase tracking-tighter">{item.label}</span>
            <span className={clsx(
              "text-xs font-mono font-semibold tabular-nums",
              item.isDanger ? "text-sf-red" : item.isWarning ? "text-sf-amber" : "text-[var(--text-primary)]"
            )}>
              {item.value}
            </span>
          </div>
        ))}
      </div>
    </div>
  );
}
