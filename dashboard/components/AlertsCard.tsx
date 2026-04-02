"use client";

import React from "react";
import { AlertCircle, AlertTriangle, Info, ShieldAlert, CheckCircle2 } from "lucide-react";
import clsx from "clsx";

interface AlertsCardProps {
  isEmergencyStopLatched: boolean;
  isDryRunError: boolean;
  isOverflowError: boolean;
  lastFaultMessage?: string;
  isSensorError: boolean;
  isFlowSensorError: boolean;
  isManualRuntimeWarning: boolean;
  isRemoteSensorStable: boolean;
  isLevelBypass: boolean;
  isFlowBypass: boolean;
  isIdleMode: boolean;
  onResetEStop: () => void;
  onClearError: () => void;
  isLoading?: boolean;
}

/**
 * REFACTOR [D4.5]: Prioritized Alerts & Faults
 * Centralizes all system warnings and operational exceptions with actionability.
 */
export default function AlertsCard({
  isEmergencyStopLatched,
  isDryRunError,
  isOverflowError,
  lastFaultMessage,
  isSensorError,
  isFlowSensorError,
  isManualRuntimeWarning,
  isRemoteSensorStable,
  isLevelBypass,
  isFlowBypass,
  isIdleMode,
  onResetEStop,
  onClearError,
  isLoading = false,
}: AlertsCardProps) {
  if (isLoading) {
    return (
      <div className="card p-6 gap-3 flex flex-col animate-pulse">
        <div className="h-4 w-20 skeleton" />
        <div className="h-12 w-full skeleton rounded-lg" />
      </div>
    );
  }

  const alerts: React.ReactNode[] = [];

  // 1. Emergency Stop (Highest Priority)
  if (isEmergencyStopLatched) {
    alerts.push(
      <AlertItem 
        key="estop"
        variant="critical"
        icon={<ShieldAlert size={18} />}
        title="Emergency stop active"
        message="Pump is locked out. System requires manual reset."
        action={<button onClick={onResetEStop} className="btn-primary py-1 px-3 text-[10px]">RESET</button>}
      />
    );
  }

  // 2. Hardware Faults (Dry Run / Overflow)
  if (isDryRunError || isOverflowError) {
    alerts.push(
      <AlertItem 
        key="fault"
        variant="danger"
        icon={<AlertCircle size={18} />}
        title={isDryRunError ? "Dry-run protection" : "Overflow protection"}
        message={lastFaultMessage ?? "Hardware protection triggered."}
        action={<button onClick={onClearError} className="btn-ghost py-1 px-3 text-[10px]">CLEAR</button>}
      />
    );
  }

  // 3. Sensor Errors
  if (isSensorError) {
    alerts.push(
      <AlertItem 
        key="sens-err"
        variant="warning"
        icon={<AlertTriangle size={18} />}
        title="Level sensor error"
        message="Ultrasonic sensor data is missing or corrupted."
      />
    );
  }

  if (isFlowSensorError) {
    alerts.push(
      <AlertItem 
        key="flow-err"
        variant="warning"
        icon={<AlertTriangle size={18} />}
        title="Flow sensor error"
        message="Dry-run protection may be impaired."
      />
    );
  }

  // 4. Runtime / Stability Warnings
  if (isManualRuntimeWarning) {
    alerts.push(
      <AlertItem 
        key="manual-warn"
        variant="warning"
        icon={<Info size={18} />}
        title="Manual runtime warning"
        message="Approaching configured max pump runtime — the pump will stop automatically if that limit is reached."
      />
    );
  }

  if (!isRemoteSensorStable) {
    alerts.push(
      <AlertItem 
        key="comm-loss"
        variant="warning"
        icon={<AlertTriangle size={18} />}
        title="Sensor Comm Loss"
        message="Remote sensor not responding; system in fail-safe mode."
      />
    );
  }

  // 5. Operation Bypasses
  if (isLevelBypass) {
    alerts.push(
      <AlertItem 
        key="level-bypass"
        variant="info"
        icon={<Info size={18} />}
        title="Level sensor bypassed"
        message="Operating on flow-guard only."
      />
    );
  }

  if (isFlowBypass) {
    alerts.push(
      <AlertItem 
        key="flow-bypass"
        variant="info"
        icon={<Info size={18} />}
        title="Flow sensor bypassed"
        message="Dry-run protection disabled."
      />
    );
  }

  // 6. Idle Mode
  if (isIdleMode) {
    alerts.push(
      <AlertItem 
        key="idle"
        variant="info"
        icon={<Info size={18} />}
        title="Idle mode active"
        message="Deep sleep cycles active to preserve bandwidth."
      />
    );
  }

  return (
    <div className="card p-6 flex flex-col gap-5">
      <h3 className="text-sm font-semibold uppercase tracking-wider text-[var(--text-muted)] self-start">System Alerts</h3>
      
      <div className="flex flex-col gap-3">
        {alerts.length > 0 ? (
          alerts
        ) : (
          <div className="flex items-center gap-3 py-3 text-[var(--text-muted)] animate-fade-in border border-transparent rounded-lg">
             <CheckCircle2 size={16} className="text-sf-green opacity-80" />
             <span className="text-xs font-medium uppercase tracking-widest text-[var(--text-secondary)]">System operating normally</span>
          </div>
        )}
      </div>
    </div>
  );
}

interface AlertItemProps {
  variant: 'critical' | 'danger' | 'warning' | 'info';
  icon: React.ReactNode;
  title: string;
  message: string;
  action?: React.ReactNode;
}

function AlertItem({ variant, icon, title, message, action }: AlertItemProps) {
  const styles = {
    critical: "bg-sf-red text-white border-transparent",
    danger: "border border-sf-red/30 bg-sf-red/5 text-[var(--text-primary)]",
    warning: "border border-sf-amber/30 bg-sf-amber/5 text-[var(--text-primary)]",
    info: "border border-sf-blue/30 bg-sf-blue/5 text-[var(--text-primary)]",
  };

  const iconColors = {
    critical: "text-white",
    danger: "text-sf-red",
    warning: "text-sf-amber",
    info: "text-sf-blue",
  }

  return (
    <div className={clsx(
      "p-4 rounded-lg flex items-start gap-3 animate-slide-up transition-shadow",
      styles[variant]
    )}>
      <div className={clsx("shrink-0 mt-0.5", iconColors[variant])}>{icon}</div>
      <div className="flex-1 min-w-0">
        <p className="text-xs font-semibold uppercase tracking-widest leading-none mb-1.5">{title}</p>
        <p className="text-sm leading-snug opacity-90 text-[var(--text-secondary)]">{message}</p>
      </div>
      {action && <div className="ml-3 shrink-0 flex items-center h-full">{action}</div>}
    </div>
  );
}
