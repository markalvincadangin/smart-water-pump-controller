"use client";

import clsx from "clsx";
import type { LucideIcon } from "lucide-react";
import { Clock, Settings, Bell, Zap, ShieldCheck, Timer, Square, RotateCw, Activity } from "lucide-react";
import { useAuditEvents } from "@/lib/useAuditEvents";
import type { AuditAction } from "@/lib/audit";

type AuditMeta = {
  mode?: string;
};

function formatAction(action: AuditAction) {
  switch (action) {
    case "control.set_mode": return "Changed mode";
    case "control.ack_error": return "Acknowledged error";
    case "control.request_reboot": return "Requested reboot";
    case "control.run_manual_start": return "Started manual run";
    case "control.run_timed_start": return "Started timed run";
    case "control.run_stop": return "Stopped run";
    case "config.device.save": return "Saved device settings";
    case "config.notifications.save": return "Saved alert settings";
    default: return "Activity";
  }
}

function iconFor(action: AuditAction): LucideIcon {
  switch (action) {
    case "control.set_mode": return Zap;
    case "control.ack_error": return ShieldCheck;
    case "control.request_reboot": return RotateCw;
    case "control.run_manual_start": return Activity;
    case "control.run_timed_start": return Timer;
    case "control.run_stop": return Square;
    case "config.device.save": return Settings;
    case "config.notifications.save": return Bell;
    default: return Activity;
  }
}

function safeTime(at: unknown) {
  // If rules store a numeric timestamp, show a relative-ish time. Otherwise show an em dash.
  if (typeof at !== "number") return "—";
  const s = Math.max(0, Math.round((Date.now() - at) / 1000));
  if (s < 5) return "Just now";
  if (s < 60) return `${s}s`;
  const m = Math.round(s / 60);
  if (m < 60) return `${m}m`;
  const h = Math.round(m / 60);
  return `${h}h`;
}

export default function ActivityPanel() {
  const { events } = useAuditEvents(10);

  return (
    <div className="card p-4 sm:p-5 border-surface-3 min-w-0">
      <div className="flex items-center justify-between gap-2 mb-3">
        <div>
          <h3 className="font-display font-semibold text-sm uppercase tracking-widest text-text-primary">
            Activity
          </h3>
          <p className="text-[10px] sm:text-xs font-mono text-text-muted mt-0.5">
            Recent changes from all users
          </p>
        </div>
        <div className="flex items-center gap-2 text-text-muted">
          <Clock size={12} />
          <span className="text-xs font-mono">Live</span>
        </div>
      </div>

      {events.length === 0 ? (
        <div className="flex flex-col items-center justify-center py-6 gap-2 text-center">
          <p className="text-xs font-mono text-text-muted">
            No recent activity yet
          </p>
          <p className="text-[10px] font-mono text-text-muted max-w-xs">
            Changes you make to modes or settings will show up here so you can see who did what and when.
          </p>
        </div>
      ) : (
        <div className="space-y-2">
          {events.map((e) => {
            const Icon = iconFor(e.action);
            const who = e.email || (e.uid ? `${e.uid.slice(0, 6)}…` : "Unknown user");
            const mode = (e.meta as AuditMeta | undefined)?.mode;
            const atMs = typeof e.at_ms === "number" ? e.at_ms : e.at;

            return (
              <div
                key={e.id}
                className="flex items-start gap-3 p-3 rounded-xl bg-surface-2 border border-surface-4 min-w-0"
              >
                <div className="shrink-0 p-2 rounded-lg bg-surface-3 text-text-secondary">
                  <Icon size={14} />
                </div>
                <div className="flex-1 min-w-0">
                  <p className="text-xs font-mono text-text-primary break-words">
                    {formatAction(e.action)}
                    {mode ? <span className="text-text-muted"> · {mode}</span> : null}
                  </p>
                  <p className="text-[10px] font-mono text-text-muted mt-0.5 break-words">
                    {who}
                  </p>
                </div>
                <div className={clsx("text-[10px] font-mono text-text-muted shrink-0")}>
                  {safeTime(atMs)}
                </div>
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
}

