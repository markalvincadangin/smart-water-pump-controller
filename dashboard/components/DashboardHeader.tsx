"use client";

import clsx from "clsx";
import Image from "next/image";
import OverflowMenu from "@/components/OverflowMenu";
import AppIcon from "@/components/AppIcon";
import { ThemeToggle } from "@/components/ThemeToggle";

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
    <header className="sticky top-0 z-30 min-h-[56px] border-b border-border-faint bg-canvas/90 px-4 py-3 backdrop-blur-md supports-[backdrop-filter]:bg-canvas/80 sm:px-6 md:min-h-[48px] md:py-3">
      <div className="mx-auto flex h-full max-w-6xl items-center">
        <div className="flex w-full items-center justify-between gap-3">
          {/* Left: Title and label */}
          <div className="min-w-0 flex-1">
            <h1 className="font-display text-lg sm:text-2xl md:text-3xl font-bold text-text-primary truncate leading-tight">
              <span className="sr-only">SmartFlow Dashboard</span>
              <div className="flex items-center">
                {/* Desktop / tablet: wordmark (full) */}
                <div className="hidden sm:flex items-center">
                  <Image
                    src="/logos/wordmark.svg"
                    alt="SmartFlow"
                    width={240}
                    height={48}
                    className="app-icon h-[32px] w-auto"
                    unoptimized
                    priority
                  />
                </div>
                {/* Mobile: combination mark (compact) */}
                <div className="sm:hidden flex items-center">
                  <Image
                    src="/logos/combinationmark.svg"
                    alt="SmartFlow"
                    width={44}
                    height={44}
                    className="app-icon h-[28px] w-auto"
                    unoptimized
                  />
                </div>
              </div>
            </h1>
          </div>

          {/* Right: Status badge + actions */}
          <div className="flex items-center gap-1.5 sm:gap-2 shrink-0">
            {/* Pump status badge */}
            <div
              className={clsx(
                "flex items-center gap-1.5 rounded-full border px-2.5 py-1.5 transition-colors duration-150 ease-out sm:gap-2 sm:px-3.5 sm:py-2",
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
                {isError ? "Error" : running ? "Running" : "Idle"}
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
                className="hidden md:flex min-h-[36px] min-w-[36px] items-center justify-center p-1.5 rounded-lg border border-transparent text-text-secondary
                         hover:border-[rgb(var(--c-border-subtle))] hover:text-accent-cyan transition-colors touch-manipulation
                         focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 disabled:opacity-50"
              >
                <AppIcon name="rotate-cw" size={16} className="text-current" />
              </button>
            )}
            <button
              onClick={onOpenDeviceConfig}
              title="Device settings"
              className="hidden md:flex min-h-[36px] min-w-[36px] items-center justify-center p-1.5 rounded-lg border border-transparent text-text-secondary
                       hover:border-[rgb(var(--c-border-subtle))] hover:text-accent-cyan transition-colors touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-cyan/50"
            >
              <AppIcon name="settings" size={16} className="text-current" />
            </button>
            <button
              onClick={onOpenNotifications}
              title="Alert preferences"
              className="hidden md:flex min-h-[36px] min-w-[36px] items-center justify-center p-1.5 rounded-lg border border-transparent text-text-secondary
                       hover:border-[rgb(var(--c-border-subtle))] hover:text-accent-cyan transition-colors touch-manipulation
                       focus:outline-none focus:ring-2 focus:ring-accent-cyan/50"
            >
              <AppIcon name="bell" size={16} className="text-current" />
            </button>
            <button
              onClick={onSignOut}
              className="hidden md:flex px-2.5 py-1.5 rounded-lg border border-transparent text-text-secondary
                       text-xs font-mono hover:border-[rgb(var(--c-border-subtle))] hover:text-accent-red
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
