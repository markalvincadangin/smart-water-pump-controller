"use client";

import clsx from "clsx";
import { Bell, Settings } from "lucide-react";
import Logo from "@/components/Logo";

interface DashboardHeaderProps {
  userEmail: string;
  running: boolean;
  isError: boolean;
  onOpenDeviceConfig: () => void;
  onOpenNotifications: () => void;
  onSignOut: () => void;
}

export default function DashboardHeader({
  userEmail,
  running,
  isError,
  onOpenDeviceConfig,
  onOpenNotifications,
  onSignOut,
}: DashboardHeaderProps) {
  return (
    <header className="px-4 sm:px-6 pt-4 sm:pt-8 pb-4">
      <div className="max-w-6xl mx-auto">
        <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
          <div className="flex items-center gap-3 min-w-0">
            <div className="shrink-0 w-10 h-10 sm:w-12 sm:h-12 flex items-center justify-center rounded-xl bg-accent-cyan/10 border border-accent-cyan/20">
              <Logo size="md" />
            </div>
            <div className="min-w-0">
              <p className="text-[10px] sm:text-xs font-mono text-text-muted uppercase tracking-[0.2em] sm:tracking-[0.3em] mb-0.5 sm:mb-1">
                Deep Well Pump · 660L Tank
              </p>
              <h1 className="font-display text-lg sm:text-2xl md:text-3xl font-bold text-text-primary truncate leading-tight">
                Smart Water Pump <span className="text-gradient-cyan">System</span>
              </h1>
            </div>
          </div>

          <div className="flex flex-wrap items-center gap-2 sm:gap-3 shrink-0">
            <span className="text-xs font-mono text-text-muted hidden md:block truncate max-w-[140px]">
              {userEmail}
            </span>
            <button
              onClick={onOpenDeviceConfig}
              title="Device settings"
              className="min-h-[44px] min-w-[44px] flex items-center justify-center p-2 rounded-lg border border-surface-3 text-text-secondary
                       hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1"
            >
              <Settings size={18} />
            </button>
            <button
              onClick={onOpenNotifications}
              title="Alert preferences"
              className="min-h-[44px] min-w-[44px] flex items-center justify-center p-2 rounded-lg border border-surface-3 text-text-secondary
                       hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1"
            >
              <Bell size={18} />
            </button>
            <button
              onClick={onSignOut}
              className="px-3 py-2 min-h-[44px] sm:min-h-0 sm:py-1.5 rounded-lg border border-surface-3 text-text-secondary
                       text-xs font-mono hover:border-accent-red/40 hover:text-accent-red
                       transition-colors shrink-0 touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-red/50 focus:ring-offset-2 focus:ring-offset-surface-1"
            >
              Sign out
            </button>

            <div
              className={clsx(
                "flex items-center gap-2 sm:gap-2.5 px-3 sm:px-4 py-2 rounded-xl border transition-all duration-500 min-h-[44px] sm:min-h-0",
                running && !isError
                  ? "bg-accent-green/10 border-accent-green/30 shadow-[0_0_20px_rgba(0,255,136,0.15)]"
                  : isError
                    ? "bg-accent-red/10 border-accent-red/30"
                    : "bg-surface-2 border-surface-3"
              )}
            >
              <span className="sr-only" aria-live="polite">
                Pump status: {isError ? "Error" : running ? "Running" : "Idle"}
              </span>
              {running && !isError && <div className="dot-live" />}
              {isError && <div className="dot-error" />}
              {!running && !isError && <div className="w-2 h-2 rounded-full bg-text-muted" />}
              <span
                className={clsx(
                  "font-mono text-xs sm:text-sm font-semibold uppercase tracking-wider hidden sm:inline",
                  running && !isError
                    ? "text-accent-green"
                    : isError
                      ? "text-accent-red"
                      : "text-text-secondary"
                )}
              >
                {isError ? "ERROR" : running ? "RUNNING" : "IDLE"}
              </span>
              <span
                className={clsx(
                  "font-mono text-xs font-semibold uppercase tracking-wider sm:hidden",
                  running && !isError
                    ? "text-accent-green"
                    : isError
                      ? "text-accent-red"
                      : "text-text-secondary"
                )}
              >
                {isError ? "ERR" : running ? "RUN" : "IDLE"}
              </span>
            </div>
          </div>
        </div>
      </div>
    </header>
  );
}

