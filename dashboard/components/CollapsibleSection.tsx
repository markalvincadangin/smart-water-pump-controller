"use client";

import { useEffect, useState } from "react";
import { ChevronDown } from "lucide-react";

interface CollapsibleSectionProps {
  /** Section title shown in the clickable header */
  title: string;
  /** Optional subtitle/description */
  subtitle?: React.ReactNode;
  /** Extra content in header (e.g. badge) */
  headerExtra?: React.ReactNode;
  children: React.ReactNode;
  /** Card wrapper className */
  className?: string;
}

/**
 * B11: Layer 4 collapsible behavior.
 * - Mobile (<768px): collapsed by default, tap header to expand.
 * - Desktop (≥768px): always expanded.
 */
export default function CollapsibleSection({
  title,
  subtitle,
  headerExtra,
  children,
  className = "",
}: CollapsibleSectionProps) {
  const [isDesktop, setIsDesktop] = useState(false);
  const [expanded, setExpanded] = useState(false);

  useEffect(() => {
    const mq = window.matchMedia("(min-width: 768px)");
    setIsDesktop(mq.matches);
    const h = () => setIsDesktop(mq.matches);
    mq.addEventListener("change", h);
    return () => mq.removeEventListener("change", h);
  }, []);

  const isOpen = isDesktop || expanded;

  return (
    <details open={isOpen} className={`min-w-0 ${className}`}>
      <summary
        onClick={(e) => {
          e.preventDefault();
          if (!isDesktop) {
            setExpanded((prev) => !prev);
          }
        }}
        className="flex items-center justify-between gap-2 cursor-pointer list-none min-h-[44px] touch-manipulation py-2 -mx-1 px-1 rounded-lg hover:bg-surface-2/50 md:hover:bg-transparent md:cursor-default"
      >
        <div className="flex flex-col sm:flex-row sm:items-center sm:gap-2 min-w-0">
          <span className="font-display font-semibold text-sm uppercase tracking-widest text-text-primary">
            {title}
          </span>
          {subtitle && (
            <span className="text-[10px] sm:text-xs font-mono text-text-muted mt-0.5 sm:mt-0">
              {subtitle}
            </span>
          )}
        </div>
        <div className="flex items-center gap-2 shrink-0">
          {headerExtra}
          {!isDesktop && (
            <ChevronDown
              size={16}
              className={`text-text-muted transition-transform duration-200 ${isOpen ? "rotate-180" : ""}`}
              aria-hidden
            />
          )}
        </div>
      </summary>
      <div className="mt-3">{children}</div>
    </details>
  );
}
