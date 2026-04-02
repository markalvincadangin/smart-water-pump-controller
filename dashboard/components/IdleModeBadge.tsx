import React from 'react';

interface IdleModeBadgeProps {
  isIdle?: boolean;
  waterLevelPercent?: number;
}

/**
 * IdleModeBadge: Shows when the pump system is in low-poll idle mode.
 * 
 * Phase 5: Displays when tank level is ≥ 90% and firmware switches to slower polling
 * to reduce power consumption and network traffic.
 */
export function IdleModeBadge({
  isIdle = false,
  waterLevelPercent = 0,
}: IdleModeBadgeProps) {
  if (!isIdle) {
    return null;
  }

  return (
    <div
      className="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-chip
                 bg-sf-blue-light border border-sf-blue/15"
      title="Idle mode active. Telemetry updates are reduced while the tank is full and the pump is off."
    >
      <svg
        className="w-4 h-4 text-sf-blue-mid"
        fill="currentColor"
        viewBox="0 0 20 20"
      >
        <path d="M10.5 1.5H9.5A8.5 8.5 0 001 10a8.5 8.5 0 008.5 8.5h1a8.5 8.5 0 008.5-8.5 8.5 8.5 0 00-8.5-8.5zm0 15H9.5A7 7 0 012.5 10a7 7 0 017-7h1a7 7 0 017 7 7 7 0 01-7 7z" />
        <path d="M10 4a1 1 0 011 1v4a1 1 0 11-2 0V5a1 1 0 011-1z" />
      </svg>
      <span className="text-xs font-medium text-sf-blue">
        Idle{typeof waterLevelPercent === "number" ? ` (${waterLevelPercent}%)` : ""}
      </span>
    </div>
  );
}

export default IdleModeBadge;
