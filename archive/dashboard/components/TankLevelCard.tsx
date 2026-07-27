"use client";

import React from "react";
import clsx from "clsx";

interface TankLevelCardProps {
  level: number;
  distanceCm?: number;
  isFresh: boolean;
  isSensorError: boolean;
  isEstimate: boolean;
  addedVolumeL?: number;
  startLevel?: number;
  stopLevel?: number;
  isLoading?: boolean;
}

/**
 * Displays water level at-a-glance with high-contrast SVG fill and safety thresholds.
 */
export default function TankLevelCard({
  level,
  distanceCm,
  isFresh,
  isSensorError,
  isEstimate,
  addedVolumeL,
  startLevel = 20,
  stopLevel = 90,
  isLoading = false,
}: TankLevelCardProps) {
  if (isLoading) {
    return (
      <div className="card p-6 flex flex-col items-center gap-4 min-h-[320px] animate-pulse">
        <div className="h-6 w-32 skeleton mb-2" />
        <div className="h-40 w-24 skeleton rounded-lg" />
        <div className="h-10 w-20 skeleton mt-2" />
        <div className="h-4 w-32 skeleton mt-1" />
      </div>
    );
  }

  // Determine fill color based on level and state
  const getFillColor = () => {
    if (isSensorError) return "fill-sf-red";
    if (isEstimate) return "fill-sf-amber opacity-90";
    if (level <= 20) return "fill-sf-red";
    if (level <= 50) return "fill-sf-amber";
    return "fill-sf-teal";
  };

  const textColorClass = isSensorError 
    ? "text-sf-red" 
    : isEstimate 
      ? "text-sf-amber" 
      : level <= 20 
        ? "text-sf-red" 
        : "text-[var(--text-primary)]";

  return (
    <div className="card p-6 flex flex-col items-center gap-6 relative overflow-hidden transition-all duration-300 min-h-[320px]">
      <h3 className="text-sm font-semibold uppercase tracking-wider text-[var(--text-muted)] self-start">Water Level</h3>

      <div className="flex w-full items-center justify-center gap-8 md:gap-12 flex-1 mt-2">
        {/* SVG Tank */}
        <div className="relative h-[200px] w-[100px]" role="img" aria-label={`Water level: ${level}%`}>
          <svg
            viewBox="0 0 80 160"
            className="h-full w-full"
            preserveAspectRatio="xMidYMid meet"
          >
            <clipPath id="tank-clip">
              <rect x="4" y="4" width="72" height="152" rx="36" />
            </clipPath>

            {/* Inner Track */}
            <rect
              x="4"
              y="4"
              width="72"
              height="152"
              rx="36"
              className="fill-sf-gray-50 dark:fill-sf-gray-900"
            />
            
            {/* Water Fill */}
            <rect
              x="4"
              y={156 - (1.52 * level)}
              width="72"
              height={1.52 * level}
              clipPath="url(#tank-clip)"
              className={clsx(
                "transition-[height,y] duration-700 ease-in-out",
                getFillColor()
              )}
            />

            {/* Threshold Markers */}
            {stopLevel !== undefined && (
              <line
                x1="4"
                y1={156 - (1.52 * stopLevel)}
                x2="76"
                y2={156 - (1.52 * stopLevel)}
                className="stroke-sf-teal/50 stroke-[1.5] stroke-dash-2"
                strokeDasharray="4 2"
              />
            )}
            {startLevel !== undefined && (
              <line
                x1="4"
                y1={156 - (1.52 * startLevel)}
                x2="76"
                y2={156 - (1.52 * startLevel)}
                className="stroke-sf-amber/50 stroke-[1.5] stroke-dash-2"
                strokeDasharray="4 2"
              />
            )}

            {/* Outer Shell */}
            <rect
              x="4"
              y="4"
              width="72"
              height="152"
              rx="36"
              className="fill-none stroke-[var(--card-border)] stroke-1"
            />
          </svg>
        </div>

        {/* Data Column */}
        <div className="flex flex-col items-start justify-center">
          <div className="flex items-baseline gap-1">
            {isEstimate && <span className={clsx("text-3xl font-bold italic mr-1", textColorClass)}>~</span>}
            <span className={clsx("text-6xl font-semibold tracking-tighter tabular-nums", textColorClass)}>
              {level}
            </span>
            <span className="text-2xl font-medium text-[var(--text-muted)]">%</span>
          </div>
          
          <div className="mt-2 text-sm text-[var(--text-secondary)] font-medium">
            {isEstimate && addedVolumeL !== undefined ? (
              <span className="text-sf-amber">Flow estimate: +{addedVolumeL.toFixed(1)}L</span>
            ) : distanceCm !== undefined ? (
              <span>{distanceCm.toFixed(1)} cm clearance</span>
            ) : (
              <span className="opacity-50 text-[var(--text-muted)]">Checking distance...</span>
            )}
          </div>

          {/* Marker Labels */}
          <div className="mt-5 flex flex-col gap-2 font-mono text-xs uppercase tracking-widest font-semibold opacity-80">
             <div className="flex items-center gap-2">
                <div className="h-[1px] w-4 border-t border-dashed border-sf-teal" />
                <span className="text-sf-teal">Stop {stopLevel}%</span>
             </div>
             <div className="flex items-center gap-2">
                <div className="h-[1px] w-4 border-t border-dashed border-sf-amber" />
                <span className="text-sf-amber">Start {startLevel}%</span>
             </div>
          </div>
        </div>
      </div>

      {/* Footer: Health Highlights */}
      <div className="w-full mt-auto pt-4 border-t border-[var(--card-border)] flex items-center justify-between gap-4">
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            <div className={clsx("h-2 w-2 rounded-full", isFresh ? "bg-sf-teal" : "bg-sf-amber")} />
            <span className="text-xs font-semibold text-[var(--text-secondary)]">
              {isFresh ? "Live Data" : "Stale Data"}
            </span>
          </div>
          <div className="flex items-center gap-2">
            <div className={clsx("h-2 w-2 rounded-full", !isSensorError ? "bg-sf-teal" : "bg-sf-red animate-pulse")} />
            <span className="text-xs font-semibold text-[var(--text-secondary)]">
              {isSensorError ? "Sensor Error" : "Sensor OK"}
            </span>
          </div>
        </div>
      </div>
    </div>
  );
}
