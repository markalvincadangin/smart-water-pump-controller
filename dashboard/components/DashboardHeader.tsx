"use client";

import clsx from "clsx";
import OverflowMenu from "@/components/OverflowMenu";
import AppIcon from "@/components/AppIcon";
import { ThemeToggle } from "@/components/ThemeToggle";

const TANK_LABEL =
  process.env.NEXT_PUBLIC_TANK_LABEL ?? "SmartFlow · 660L Tank";

interface DashboardHeaderProps {
  userEmail: string;
  running: boolean;
  isError: boolean;
  isAdmin: boolean;
  esp32Online: boolean;
  onOpenDeviceConfig: () => void;
  onOpenNotifications: () => void;
  onRequestReboot: () => void;
  onSignOut: () => void;
}

export default function DashboardHeader({
  userEmail,
  running,
  isError,
  isAdmin,
  esp32Online,
  onOpenDeviceConfig,
  onOpenNotifications,
  onRequestReboot,
  onSignOut,
}: DashboardHeaderProps) {
  return (
    <header className="px-4 sm:px-6 pt-3 sm:pt-6 pb-3 sm:pb-4">
      <div className="max-w-6xl mx-auto">
        <div className="flex items-center justify-between gap-3">
          {/* Left: Title and label */}
          <div className="min-w-0 flex-1">
            <p className="text-[9px] sm:text-xs font-mono text-text-muted uppercase tracking-[0.15em] sm:tracking-[0.25em] mb-0.5">
              {TANK_LABEL}
            </p>
            <h1 className="font-display text-lg sm:text-2xl md:text-3xl font-bold text-text-primary truncate leading-tight">
              SmartFlow <span className="text-gradient-cyan">Dashboard</span>
            </h1>
          </div>

          {/* Right: Status badge + actions */}
          <div className="flex items-center gap-2 sm:gap-3 shrink-0">
            {/* Pump status badge */}
            <div
              className={clsx(
                "flex items-center gap-1.5 sm:gap-2 px-2.5 sm:px-3.5 py-1.5 sm:py-2 rounded-xl border transition-all duration-500",
                running && !isError
                  ? "bg-accent-green/10 border-accent-green/30 shadow-[0_0_20px_rgb(var(--c-status-ok)/0.15)]"
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
              {!running && !isError && <div className="w-1.5 h-1.5 sm:w-2 sm:h-2 rounded-full bg-text-muted" />}
              <span
                className={clsx(
                  "font-mono text-[10px] sm:text-xs font-semibold uppercase tracking-wider",
                  running && !isError ? "text-accent-green"
                    : isError ? "text-accent-red"
                    : "text-text-secondary"
                )}
              >
                {isError ? "ERR" : running ? "RUN" : "IDLE"}
              </span>
            </div>

            <ThemeToggle />

            {/* Mobile: overflow menu */}
            <OverflowMenu
              userEmail={userEmail}
              isAdmin={isAdmin}
              esp32Online={esp32Online}
              onOpenDeviceConfig={onOpenDeviceConfig}
              onOpenNotifications={onOpenNotifications}
              onRequestReboot={onRequestReboot}
              onSignOut={onSignOut}
            />

            {/* Desktop: inline actions */}
            <span className="text-[10px] font-mono text-text-muted hidden md:block truncate max-w-[120px]"
              title={userEmail}>
              {userEmail}
            </span>
            {isAdmin && (
              <button
                onClick={onRequestReboot}
                title={esp32Online ? "Restart controller" : "Controller must be online"}
                disabled={!esp32Online}
                className="hidden md:flex min-h-[36px] min-w-[36px] items-center justify-center p-1.5 rounded-lg border border-surface-3 text-text-secondary
                         hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors touch-manipulation
                         focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 disabled:opacity-50"
              >
                <AppIcon name="rotate-cw" size={16} className="text-current" />
              </button>
            )}
            <button
              onClick={onOpenDeviceConfig}
              title="Device settings"
              className="hidden md:flex min-h-[36px] min-w-[36px] items-center justify-center p-1.5 rounded-lg border border-surface-3 text-text-secondary
                       hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-cyan/50"
            >
              <AppIcon name="settings" size={16} className="text-current" />
            </button>
            <button
              onClick={onOpenNotifications}
              title="Alert preferences"
              className="hidden md:flex min-h-[36px] min-w-[36px] items-center justify-center p-1.5 rounded-lg border border-surface-3 text-text-secondary
                       hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-cyan/50"
            >
              <AppIcon name="bell" size={16} className="text-current" />
            </button>
            <button
              onClick={onSignOut}
              className="hidden md:flex px-2.5 py-1.5 rounded-lg border border-surface-3 text-text-secondary
                       text-xs font-mono hover:border-accent-red/40 hover:text-accent-red
                       transition-colors shrink-0 touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-red/50"
            >
              Sign out
            </button>
          </div>
        </div>
      </div>
    </header>
  );
}
