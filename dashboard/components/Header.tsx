"use client";

import React, { useEffect, useState } from "react";
import Link from "next/link";
import { ThemeToggle } from "./ThemeToggle";
import { Wifi, Clock, AlertTriangle } from "lucide-react";
import clsx from "clsx";

interface HeaderProps {
  isConnected: boolean;
  rssi?: number | null;
  lastUpdated?: number | null; // timestamp in ms
}

/**
 * REFACTOR [D4.1]: SmartFlow Global Header
 * Features sticky positioning, brand wordmark, and connection health telemetry.
 */
export default function Header({ isConnected, rssi, lastUpdated }: HeaderProps) {
  const [relativeTime, setRelativeTime] = useState<string>("just now");

  useEffect(() => {
    if (!lastUpdated) return;

    const updateTime = () => {
      const seconds = Math.floor((Date.now() - lastUpdated) / 1000);
      if (seconds < 5) setRelativeTime("just now");
      else if (seconds < 60) setRelativeTime(`${seconds}s ago`);
      else setRelativeTime(`${Math.floor(seconds / 60)}m ago`);
    };

    updateTime();
    const interval = setInterval(updateTime, 5000);
    return () => clearInterval(interval);
  }, [lastUpdated]);

  const getRssiColor = (val?: number | null) => {
    if (val === undefined || val === null || val === 0) return "text-[var(--text-muted)]";
    if (val >= -60) return "text-sf-teal";
    if (val >= -75) return "text-sf-amber";
    return "text-sf-red";
  };

  return (
    <>
      <header className="sticky top-0 z-50 w-full border-b border-[var(--card-border)] bg-[var(--card-bg)]/80 backdrop-blur-md">
        <div className="mx-auto flex h-14 max-w-screen-lg items-center justify-between px-4">
          {/* Left: Wordmark */}
          <Link href="/" className="flex items-center gap-2 transition-opacity hover:opacity-80">
            <span className="font-sans text-xl font-bold tracking-tight text-sf-blue">
              SmartFlow
            </span>
          </Link>

          {/* Right: Status Cluster */}
          <div className="flex items-center gap-4">
            <div className="hidden items-center gap-3 md:flex">
              {/* RSSI Badge */}
              {rssi !== undefined && rssi !== 0 && (
                <div className={clsx("flex items-center gap-1.5 font-mono text-xs font-medium", getRssiColor(rssi))}>
                  <Wifi size={14} strokeWidth={2.5} />
                  <span>{rssi} dBm</span>
                </div>
              )}

              {/* Last Updated */}
              {lastUpdated && (
                <div className="flex items-center gap-1.5 font-mono text-xs text-[var(--text-muted)]">
                  <Clock size={14} strokeWidth={2.5} />
                  <span>{relativeTime}</span>
                </div>
              )}
            </div>

            {/* Connection Dot & Toggle */}
            <div className="flex items-center gap-3">
              <div 
                className={clsx(
                  "h-2.5 w-2.5 rounded-full shadow-sm transition-colors duration-500",
                  isConnected ? "bg-sf-teal shadow-sf-teal/20" : "bg-sf-amber animate-pulse shadow-sf-amber/20"
                )}
                title={isConnected ? "Firebase Connected" : "Reconnecting..."}
              />
              <ThemeToggle />
            </div>
          </div>
        </div>
      </header>

      {/* Offline Banner: Safety-critical notification */}
      {!isConnected && (
        <div className="w-full bg-sf-amber-light border-b border-sf-amber/20 px-4 py-2 text-center text-xs font-medium text-sf-amber-dark animate-fade-in">
          <div className="mx-auto flex max-w-screen-lg items-center justify-center gap-2">
            <AlertTriangle size={14} />
            <span>
              Reconnecting to SmartFlow... Showing last known data
              {lastUpdated ? ` from ${relativeTime}` : ""}.
            </span>
          </div>
        </div>
      )}
    </>
  );
}
