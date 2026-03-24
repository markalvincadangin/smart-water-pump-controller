"use client";

import { useEffect, useMemo, useState } from "react";
import clsx from "clsx";
import { CheckCircle2, Info, AlertTriangle, XCircle, X } from "lucide-react";
import { onToast, type ToastKind, type ToastMessage } from "@/lib/toast";

type ToastItem = Required<Pick<ToastMessage, "title">> &
  ToastMessage & {
    id: string;
    createdAt: number;
    kind: ToastKind;
    timeoutMs: number;
  };

const DEFAULT_TIMEOUT_MS = 3200;

function newId() {
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

export default function ToastHost() {
  const [items, setItems] = useState<ToastItem[]>([]);

  useEffect(() => {
    return onToast((msg) => {
      const item: ToastItem = {
        id: msg.id ?? newId(),
        createdAt: Date.now(),
        kind: msg.kind ?? "info",
        title: msg.title,
        detail: msg.detail,
        timeoutMs: msg.timeoutMs ?? DEFAULT_TIMEOUT_MS,
      };

      setItems((prev) => {
        // Keep newest first, cap at 3
        const next = [item, ...prev].slice(0, 3);
        return next;
      });
    });
  }, []);

  useEffect(() => {
    if (!items.length) return;

    const timers = items.map((t) =>
      window.setTimeout(() => {
        setItems((prev) => prev.filter((x) => x.id !== t.id));
      }, t.timeoutMs)
    );

    return () => timers.forEach((id) => window.clearTimeout(id));
  }, [items]);

  const iconFor = useMemo(
    () => ({
      success: CheckCircle2,
      info: Info,
      warning: AlertTriangle,
      error: XCircle,
    }),
    []
  );

  if (!items.length) return null;

  return (
    <div className="fixed top-3 right-3 left-3 sm:left-auto sm:w-[380px] z-[10000] space-y-2 pt-[env(safe-area-inset-top)]">
      {items.map((t) => {
        const Icon = iconFor[t.kind];
        const border =
          t.kind === "success" ? "border-accent-green/40" :
          t.kind === "warning" ? "border-accent-amber/40" :
          t.kind === "error" ? "border-accent-red/40" :
          "border-accent-cyan/40";
        const bg =
          t.kind === "success" ? "bg-accent-green/10" :
          t.kind === "warning" ? "bg-accent-amber/10" :
          t.kind === "error" ? "bg-accent-red/10" :
          "bg-accent-cyan/10";
        const iconColor =
          t.kind === "success" ? "text-accent-green" :
          t.kind === "warning" ? "text-accent-amber" :
          t.kind === "error" ? "text-accent-red" :
          "text-accent-cyan";

        return (
          <div
            key={t.id}
            className={clsx(
              "card rounded-lg border border-l-[3px] shadow-lg backdrop-blur",
              border,
              bg
            )}
            role="status"
            aria-live="polite"
          >
            <div className="flex items-start gap-3 p-3">
              <div className={clsx("shrink-0 mt-0.5", iconColor)}>
                <Icon size={16} />
              </div>
              <div className="flex-1 min-w-0">
                <p className="text-xs font-mono text-text-primary font-semibold break-words">
                  {t.title}
                </p>
                {t.detail && (
                  <p className="text-[10px] font-mono text-text-muted mt-0.5 break-words">
                    {t.detail}
                  </p>
                )}
              </div>
              <button
                type="button"
                onClick={() => setItems((prev) => prev.filter((x) => x.id !== t.id))}
                className="shrink-0 p-1 rounded text-text-muted hover:text-text-primary hover:bg-surface-3"
                aria-label="Dismiss"
              >
                <X size={14} />
              </button>
            </div>
          </div>
        );
      })}
    </div>
  );
}

