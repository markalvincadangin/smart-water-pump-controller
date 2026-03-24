// components/TankVisual.tsx — Part 7.8 tank visualization (5-stop water gradient)
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
  levelLastValidAgeSec?: number;
  /** Part 9.2 — false when level sample is older than controller freshness window */
  levelFresh?: boolean;
}

function waterFillClass(level: number, isError: boolean, estimate: boolean): string {
  if (isError) return "bg-[rgb(var(--c-status-error)/0.72)]";
  if (estimate) return "bg-[rgb(var(--c-status-warn)/0.45)]";
  if (level <= 20) return "bg-[rgb(var(--c-water-low)/0.85)]";
  if (level <= 40) return "bg-[rgb(var(--c-water-orange)/0.88)]";
  if (level <= 60) return "bg-[rgb(var(--c-water-mid)/0.88)]";
  if (level < 90) return "bg-[rgb(var(--c-water-high)/0.88)]";
  return "bg-[rgb(var(--c-water-full)/0.88)]";
}

export default function TankVisual({
  level,
  isRunning,
  isError,
  levelEstimateActive = false,
  pumpStartLevel,
  pumpStopLevel,
  levelLastValidAgeSec,
  levelFresh = true,
}: TankVisualProps) {
  const fillRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (fillRef.current) {
      fillRef.current.style.height = `${level}%`;
    }
  }, [level]);

  const isStaleAge = typeof levelLastValidAgeSec === "number" && levelLastValidAgeSec > 300;
  const levelStale = levelFresh === false || isStaleAge;

  const fillClass = waterFillClass(level, isError, levelEstimateActive);

  const glowClass =
    isError
      ? "shadow-[0_0_40px_rgb(var(--c-status-error)/0.35)]"
      : "shadow-[0_0_48px_rgb(var(--c-brand-glow)/0.22)]";

  return (
    <div className="flex flex-col items-center gap-4">
      {/* Part 6.5 — desktop 200×320, mobile 160×260 */}
      <div
        className={clsx(
          "relative w-[160px] h-[260px] md:w-[200px] md:h-[320px] transition-shadow duration-300",
          glowClass
        )}
      >
        <div
          className={clsx(
            "absolute inset-0 overflow-hidden rounded-xl border-2 md:rounded-xl",
            isError ? "border-accent-red/50" : "border-surface-4",
            "bg-surface-2"
          )}
        >
          <div
            ref={fillRef}
            className={clsx(
              "absolute bottom-0 left-0 right-0 overflow-hidden",
              "transition-[height] duration-[800ms] ease-[cubic-bezier(0.4,0,0.2,1)]",
              "transition-colors duration-[600ms] ease-out",
              fillClass,
              levelEstimateActive && "border-t-2 border-dashed border-accent-amber/60",
              levelStale && "opacity-75"
            )}
            style={{ height: `${level}%` }}
          >
            {/* Water surface motion — Part 7.8 */}
            {isRunning && !isError && (
              <div
                className="pointer-events-none absolute inset-x-0 top-0 h-3 animate-tank-wave opacity-40"
                style={{
                  background:
                    "linear-gradient(90deg, transparent, rgb(255 255 255 / 0.12), transparent)",
                }}
              />
            )}
            {/* §7.8 — scanline removed; only sine wave surface animation per spec */}
          </div>

          {typeof pumpStartLevel === "number" && (
            <div
              className="pointer-events-none absolute left-0 right-0 z-10"
              style={{ bottom: `${pumpStartLevel}%` }}
            >
              <div className="w-full border-t border-dashed border-accent-amber/55" />
              <span className="absolute right-1 -top-3 font-mono text-xs leading-none text-text-unit">
                ON {pumpStartLevel}%
              </span>
            </div>
          )}
          {typeof pumpStopLevel === "number" && (
            <div
              className="pointer-events-none absolute left-0 right-0 z-10"
              style={{ bottom: `${pumpStopLevel}%` }}
            >
              <div className="w-full border-t border-dashed border-accent-green/55" />
              <span className="absolute right-1 top-0.5 font-mono text-xs leading-none text-text-unit">
                OFF {pumpStopLevel}%
              </span>
            </div>
          )}

          {[0, 25, 50, 75, 100].map((tick) => (
            <div
              key={tick}
              className="absolute left-0 right-0 flex items-center border-t border-white/[0.06]"
              style={{ bottom: `${tick}%` }}
            >
              <span className="pl-1 font-mono text-xs text-text-muted/70">{tick}</span>
            </div>
          ))}
        </div>

        <div
          className={clsx(
            "absolute -top-3 left-2 right-2 h-3 rounded-t-md border-x-2 border-t-2 md:left-4 md:right-4 md:h-3",
            isError ? "border-accent-red/50" : "border-surface-4",
            "bg-surface-3"
          )}
        />
        <div className="absolute -right-3 top-1/3 h-2 w-3 rounded-r border border-surface-4 bg-surface-3 md:-right-5 md:h-2 md:w-5" />
      </div>

      <div className="text-center">
        <div className="flex flex-wrap items-center justify-center gap-2">
          <div
            className={clsx(
              "font-mono text-hero font-semibold tabular-nums leading-none tracking-tight",
              isError
                ? "text-accent-red"
                : levelEstimateActive
                  ? "text-accent-amber"
                  : level <= 20
                    ? "text-accent-red"
                    : level <= 40
                      ? "text-[rgb(var(--c-water-orange))]"
                      : level <= 60
                        ? "text-accent-amber"
                        : level >= 90
                          ? "text-accent-green"
                          : "text-accent-cyan"
            )}
          >
            {(levelEstimateActive || isStaleAge) && !isError ? "~" : ""}
            {level}
            <span className="ml-0.5 text-metric font-semibold opacity-80">%</span>
          </div>
          {levelFresh === false && (
            <span className="rounded border border-accent-amber/40 bg-accent-amber/10 px-2 py-0.5 font-mono text-xs font-semibold uppercase tracking-wide text-accent-amber">
              Stale
            </span>
          )}
        </div>
        <div className="mt-2 max-w-[18rem] font-mono text-xs uppercase tracking-wider text-text-secondary">
          {levelEstimateActive
            ? "Flow estimate · check level sensor"
            : isError
              ? "Error — pump stopped"
              : isStaleAge
                ? "Level reading stale"
                : level <= 20
                  ? "Critical low"
                  : level <= 40
                    ? "Low level"
                    : level >= 90
                      ? "Near full"
                      : "Normal range"}
        </div>
      </div>
    </div>
  );
}
