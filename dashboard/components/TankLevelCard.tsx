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
 * REFACTOR [D4.2]: Tank Level Visualization
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
    <div className="card p-6 flex flex-col items-center gap-6 relative overflow-hidden transition-all duration-300">
      <h3 className="card-header self-start">Water Level</h3>

      <div className="flex w-full items-center justify-center gap-8 md:gap-12">
        {/* SVG Tank */}
        <div className="relative h-[200px] w-[100px]" role="img" aria-label={`Water level: ${level}%`}>
          <svg
            viewBox="0 0 80 160"
            className="h-full w-full drop-shadow-sm"
            preserveAspectRatio="xMidYMid meet"
          >
            {/* Outer Shell */}
            <rect
              x="2"
              y="2"
              width="76"
              height="156"
              rx="12"
              className="fill-none stroke-[var(--card-border)] stroke-[1.5]"
            />
            
            {/* Water Fill */}
            <rect
              x="4"
              y={158 - (1.54 * level)} // Adjust for bottom margin + stroke
              width="72"
              height={1.54 * level}
              rx="10"
              className={clsx(
                "transition-[height,y] duration-700 ease-in-out",
                getFillColor()
              )}
            />

            {/* Threshold Markers */}
            {stopLevel !== undefined && (
              <line
                x1="2"
                y1={158 - (1.54 * stopLevel)}
                x2="78"
                y2={158 - (1.54 * stopLevel)}
                className="stroke-sf-teal/30 stroke-1 stroke-dash-2"
                strokeDasharray="4 2"
              />
            )}
            {startLevel !== undefined && (
              <line
                x1="2"
                y1={158 - (1.54 * startLevel)}
                x2="78"
                y2={158 - (1.54 * startLevel)}
                className="stroke-sf-amber/40 stroke-1 stroke-dash-2"
                strokeDasharray="4 2"
              />
            )}
          </svg>
          
          {/* Internal level label overlays if desired, or kept external for clarity */}
        </div>

        {/* Data Column */}
        <div className="flex flex-col items-start justify-center">
          <div className="flex items-baseline gap-1">
            {isEstimate && <span className={clsx("text-2xl font-bold italic mr-1", textColorClass)}>~</span>}
            <span className={clsx("text-5xl font-bold tracking-tight", textColorClass)}>
              {level}
            </span>
            <span className="text-xl font-semibold opacity-40">%</span>
          </div>
          
          <div className="mt-1 font-mono text-sm text-[var(--text-secondary)] font-medium">
            {isEstimate && addedVolumeL !== undefined ? (
              <span className="text-sf-amber">Flow estimate · +{addedVolumeL.toFixed(1)} L</span>
            ) : distanceCm !== undefined ? (
              <span>{distanceCm.toFixed(1)} cm from sensor</span>
            ) : (
              <span className="opacity-50">Checking distance...</span>
            )}
          </div>

          {/* Marker Labels */}
          <div className="mt-4 flex flex-col gap-1.5 font-mono text-[10px] uppercase tracking-wider font-semibold opacity-60">
             <div className="flex items-center gap-2">
                <div className="h-0.5 w-6 border-t border-dashed border-sf-teal" />
                <span>Stop {stopLevel}%</span>
             </div>
             <div className="flex items-center gap-2">
                <div className="h-0.5 w-6 border-t border-dashed border-sf-amber" />
                <span>Start {startLevel}%</span>
             </div>
          </div>
        </div>
      </div>

      {/* Footer: Health Highlights */}
      <div className="w-full mt-4 pt-4 border-t border-[var(--card-border)]/50 flex items-center justify-between gap-4">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-1.5">
            <div className={clsx("h-2 w-2 rounded-full", isFresh ? "bg-sf-teal shadow-sf-teal/30 shadow-sm" : "bg-sf-amber")} />
            <span className="text-[10px] font-bold uppercase tracking-widest text-[var(--text-secondary)]">
              {isFresh ? "Fresh" : "Stale"}
            </span>
          </div>
          <div className="flex items-center gap-1.5">
            <div className={clsx("h-2 w-2 rounded-full", !isSensorError ? "bg-sf-teal shadow-sf-teal/30 shadow-sm" : "bg-sf-red animate-pulse")} />
            <span className="text-[10px] font-bold uppercase tracking-widest text-[var(--text-secondary)]">
              {isSensorError ? "Sensor Error" : "Sensor OK"}
            </span>
          </div>
        </div>
      </div>
    </div>
  );
}
