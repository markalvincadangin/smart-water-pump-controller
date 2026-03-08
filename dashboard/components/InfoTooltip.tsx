"use client";

import { useState, useRef, useEffect, useCallback, useId } from "react";
import { createPortal } from "react-dom";
import { HelpCircle, X } from "lucide-react";

interface InfoTooltipProps {
  content: string | React.ReactNode;
  side?: "top" | "bottom" | "left" | "right";
  maxWidth?: string;
  className?: string;
}

const GAP = 8;
const PAD = 16;

/**
 * Info tooltip — click to toggle. Renders via portal to avoid clipping.
 * Desktop: hover to open, click outside to close. Mobile: tap to toggle.
 */
export default function InfoTooltip({ content, side = "top", maxWidth = "320px", className = "" }: InfoTooltipProps) {
  const [open, setOpen] = useState(false);
  const id = useId();
  const tooltipId = `tooltip-${id.replace(/:/g, "-")}`;
  const triggerRef = useRef<HTMLButtonElement>(null);
  const popoverRef = useRef<HTMLDivElement>(null);

  const close = useCallback(() => setOpen(false), []);

  useEffect(() => {
    if (!open) return;
    function onClickOutside(e: MouseEvent) {
      const t = e.target as Node;
      if (triggerRef.current?.contains(t) || popoverRef.current?.contains(t)) return;
      close();
    }
    function onEscape(e: KeyboardEvent) {
      if (e.key === "Escape") close();
    }
    document.addEventListener("mousedown", onClickOutside);
    document.addEventListener("keydown", onEscape);
    return () => {
      document.removeEventListener("mousedown", onClickOutside);
      document.removeEventListener("keydown", onEscape);
    };
  }, [open, close]);

  const position = useCallback(() => {
    const trigger = triggerRef.current;
    const popover = popoverRef.current;
    if (!trigger || !popover) return;

    const tr = trigger.getBoundingClientRect();
    const pr = popover.getBoundingClientRect();
    const vw = window.innerWidth;
    const vh = window.innerHeight;

    let top = 0;
    let left = tr.left + tr.width / 2 - pr.width / 2;

    if (vw < 640) {
      popover.style.position = "fixed";
      popover.style.left = `${PAD}px`;
      popover.style.right = `${PAD}px`;
      popover.style.width = `calc(100vw - ${PAD * 2}px)`;
      popover.style.top = "50%";
      popover.style.transform = "translateY(-50%)";
      popover.style.maxWidth = "none";
      return;
    }

    if (side === "top" && tr.top - pr.height - GAP >= PAD) {
      top = tr.top - pr.height - GAP;
    } else if (side === "bottom" && tr.bottom + pr.height + GAP <= vh - PAD) {
      top = tr.bottom + GAP;
    } else if (side === "right" && tr.right + pr.width + GAP <= vw - PAD) {
      top = tr.top + tr.height / 2 - pr.height / 2;
      left = tr.right + GAP;
    } else if (side === "left" && tr.left - pr.width - GAP >= PAD) {
      top = tr.top + tr.height / 2 - pr.height / 2;
      left = tr.left - pr.width - GAP;
    } else if (tr.bottom + pr.height + GAP <= vh - PAD) {
      top = tr.bottom + GAP;
    } else if (tr.top - pr.height - GAP >= PAD) {
      top = tr.top - pr.height - GAP;
    } else {
      top = Math.max(PAD, Math.min(vh - pr.height - PAD, tr.top + tr.height / 2 - pr.height / 2));
    }

    left = Math.max(PAD, Math.min(vw - pr.width - PAD, left));

    popover.style.position = "fixed";
    popover.style.top = `${top}px`;
    popover.style.left = `${left}px`;
    popover.style.transform = "none";
    popover.style.width = "";
    popover.style.right = "";
    popover.style.maxWidth = maxWidth;
  }, [side, maxWidth]);

  useEffect(() => {
    if (!open || !popoverRef.current || !triggerRef.current) return;
    const runPosition = () => {
      requestAnimationFrame(() => {
        requestAnimationFrame(() => position());
      });
    };
    runPosition();
    const ro = new ResizeObserver(runPosition);
    ro.observe(popoverRef.current);
    window.addEventListener("resize", runPosition);
    return () => {
      ro.disconnect();
      window.removeEventListener("resize", runPosition);
    };
  }, [open, position]);

  const body = typeof document !== "undefined" ? document.body : null;

  return (
    <>
      <button
        ref={triggerRef}
        type="button"
        onClick={(e) => {
          e.preventDefault();
          e.stopPropagation();
          setOpen((o) => !o);
        }}
        onMouseEnter={() => typeof window !== "undefined" && window.innerWidth >= 640 && setOpen(true)}
        className={`inline-flex items-center justify-center w-8 h-8 sm:w-5 sm:h-5 rounded-full shrink-0
          text-text-muted hover:text-accent-cyan hover:bg-accent-cyan/10
          focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1
          transition-colors touch-manipulation ${className}`}
        aria-label="More information"
        aria-expanded={open}
        aria-describedby={open ? tooltipId : undefined}
      >
        <HelpCircle size={16} className="sm:w-3.5 sm:h-3.5" strokeWidth={2} />
      </button>
      {open &&
        body &&
        createPortal(
          <>
            {/* Mobile: visible backdrop, tap to close. Desktop: invisible, pointer-events-none so hover on trigger doesn't get interrupted */}
            <div
              className="fixed inset-0 z-[9998] bg-black/50 sm:bg-transparent sm:pointer-events-none"
              aria-hidden="true"
              onClick={() => { if (typeof window !== "undefined" && window.innerWidth < 640) close(); }}
            />
            <div
              ref={popoverRef}
              id={tooltipId}
              role="tooltip"
              className="z-[9999] px-4 py-3 rounded-xl bg-surface-2 border border-surface-4 text-text-primary text-sm font-mono leading-relaxed
                shadow-xl shadow-black/50 min-w-[200px] max-w-[min(320px,calc(100vw-32px))] tooltip-enter"
              style={{
                position: "fixed",
                left: "50%",
                top: "50%",
                transform: "translate(-50%, -50%)",
                maxWidth: typeof window !== "undefined" && window.innerWidth < 640 ? "none" : maxWidth,
              }}
            >
              <div className="flex items-start gap-2">
                <div className="flex-1 min-w-0">
                  {typeof content === "string" ? content : content}
                </div>
                <button
                  type="button"
                  onClick={close}
                  className="sm:hidden shrink-0 p-1 rounded text-text-muted hover:text-text-primary hover:bg-surface-3"
                  aria-label="Close"
                >
                  <X size={16} />
                </button>
              </div>
            </div>
          </>,
          body
        )}
    </>
  );
}
