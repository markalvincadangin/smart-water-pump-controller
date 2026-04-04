"use client";

import clsx from "clsx";
import { Clock } from "lucide-react";
import { useAuditEvents } from "@/lib/useAuditEvents";
import { getAuditActionLabel, getAuditActionIcon } from "@/lib/audit";
import CollapsibleSection from "@/components/CollapsibleSection";
import { formatPhtDateTimeOrDefault, getPhtTimezoneLabel } from "@/lib/time";

type AuditMeta = {
  mode?: string;
};

export default function ActivityPanel() {
  const { events } = useAuditEvents(10);

  return (
    <div className="card p-4 sm:p-5 border-surface-3 min-w-0">
      <CollapsibleSection
        title="Activity"
        subtitle={`Showing last 10 actions · Recent changes from all users · ${getPhtTimezoneLabel()}`}
        headerExtra={
          <div className="flex items-center gap-2 text-text-muted">
            <Clock size={12} />
            <span className="text-xs font-mono">Live</span>
          </div>
        }
      >
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
            const Icon = getAuditActionIcon(e.action);
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
                    {e.detail ?? `${getAuditActionLabel(e.action)}${mode ? ` · ${mode}` : ""}`}
                  </p>
                  <p className="text-[10px] font-mono text-text-muted mt-0.5 break-words">
                    {who}
                  </p>
                </div>
                <div className={clsx("text-[10px] font-mono text-text-muted shrink-0")}>
                  {formatPhtDateTimeOrDefault(atMs)}
                </div>
              </div>
            );
          })}
        </div>
      )}
      </CollapsibleSection>
    </div>
  );
}

