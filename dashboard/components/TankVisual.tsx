// components/TankVisual.tsx
"use client";

import { useEffect, useRef } from "react";
import clsx from "clsx";

interface TankVisualProps {
  level: number;
  isRunning: boolean;
  isError: boolean;
  levelEstimateActive?: boolean;
  pumpStartLevel?: number;
  pumpStopLevel?: number;
}

export default function TankVisual({ level, isRunning, isError, levelEstimateActive = false, pumpStartLevel, pumpStopLevel }: TankVisualProps) {
  const fillRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (fillRef.current) {
      fillRef.current.style.height = `${level}%`;
    }
  }, [level]);

  const fillColor =
    isError            ? "bg-accent-red/70"
    : levelEstimateActive ? "bg-accent-amber/50"
    : level <= 20      ? "bg-accent-amber/80"
    : level >= 90      ? "bg-accent-cyan/80"
    :                    "bg-accent-green/70";

  const glowColor =
    isError ? "shadow-[0_0_30px_rgba(255,59,92,0.4)]"
    : isRunning && !isError ? "shadow-[0_0_30px_rgba(0,255,136,0.35)]"
    : "";

  return (
    <div className="flex flex-col items-center gap-3 sm:gap-4">
      <div className="relative w-28 h-44 sm:w-32 sm:h-52">
        {/* Tank body */}
        <div className={clsx(
          "absolute inset-0 rounded-b-2xl rounded-t-lg border-2 overflow-hidden",
          isError ? "border-accent-red/50" : "border-surface-4",
          "bg-surface-2"
        )}>
          {/* Water fill */}
          <div
            ref={fillRef}
            className={clsx(
              "absolute bottom-0 left-0 right-0 transition-all duration-1000 ease-out",
              fillColor,
              glowColor,
              levelEstimateActive && "border-t-2 border-dashed border-amber-400/60"
            )}
            style={{ height: `${level}%` }}
          >
            {isRunning && !isError && (
              <div className="absolute top-0 left-0 right-0 h-2 bg-white/10 animate-pulse" />
            )}
            {isRunning && !isError && (
              <div
                className="absolute left-0 right-0 h-8 bg-gradient-to-b from-white/10 to-transparent"
                style={{ animation: "scanline 2s linear infinite" }}
              />
            )}
          </div>

          {/* Pump start/stop reference lines */}
          {typeof pumpStartLevel === "number" && (
            <div
              className="absolute left-0 right-0 z-10 pointer-events-none"
              style={{ bottom: `${pumpStartLevel}%` }}
            >
              <div className="w-full border-t border-dashed border-accent-amber/50" />
              <span className="absolute right-1 -top-3 text-[7px] sm:text-[8px] font-mono text-accent-amber/70 leading-none">
                ON {pumpStartLevel}%
              </span>
            </div>
          )}
          {typeof pumpStopLevel === "number" && (
            <div
              className="absolute left-0 right-0 z-10 pointer-events-none"
              style={{ bottom: `${pumpStopLevel}%` }}
            >
              <div className="w-full border-t border-dashed border-accent-green/50" />
              <span className="absolute right-1 top-0.5 text-[7px] sm:text-[8px] font-mono text-accent-green/70 leading-none">
                OFF {pumpStopLevel}%
              </span>
            </div>
          )}

          {/* Level tick marks */}
          {[0, 25, 50, 75, 100].map((tick) => (
            <div
              key={tick}
              className="absolute left-0 right-0 border-t border-white/5 flex items-center"
              style={{ bottom: `${tick}%` }}
            >
              <span className="text-[8px] sm:text-[9px] font-mono text-text-muted/60 pl-0.5 sm:pl-1">{tick}</span>
            </div>
          ))}
        </div>

        {/* Tank cap */}
        <div className={clsx(
          "absolute -top-3 left-2 right-2 sm:left-4 sm:right-4 h-2.5 sm:h-3 rounded-t-md border-t-2 border-x-2",
          isError ? "border-accent-red/50" : "border-surface-4",
          "bg-surface-3"
        )} />

        {/* Pipe */}
        <div className="absolute -right-3 sm:-right-5 top-1/3 w-3 sm:w-5 h-1.5 sm:h-2 bg-surface-3 border border-surface-4 rounded-r" />
      </div>

      {/* Level readout */}
      <div className="text-center">
        <div className={clsx(
          "text-2xl sm:text-4xl font-display font-bold tabular-nums leading-none",
          isError            ? "text-accent-red"
          : levelEstimateActive ? "text-accent-amber"
          : level <= 20      ? "text-accent-amber"
          :                    "text-gradient-cyan"
        )}>
          {levelEstimateActive ? "~" : ""}{level}<span className="text-lg sm:text-xl font-normal opacity-70">%</span>
        </div>
        <div className="text-[10px] sm:text-xs font-mono text-text-secondary mt-1 uppercase tracking-wider">
          {levelEstimateActive
            ? "Estimated · ±5%"
            : isError
              ? "Error — pump stopped"
              : level <= 20
                ? "Low Water"
                : level >= 90
                  ? "Nearly Full"
                  : "Normal"}
        </div>
      </div>
    </div>
  );
}
