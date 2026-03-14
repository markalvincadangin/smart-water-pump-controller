"use client";

import { useState, useRef, useEffect } from "react";
import { MoreVertical, Settings, Bell, RotateCw, LogOut } from "lucide-react";
import clsx from "clsx";

interface OverflowMenuProps {
  userEmail: string;
  isAdmin: boolean;
  esp32Online: boolean;
  onOpenDeviceConfig: () => void;
  onOpenNotifications: () => void;
  onRequestReboot: () => void;
  onSignOut: () => void;
}

export default function OverflowMenu({
  userEmail,
  isAdmin,
  esp32Online,
  onOpenDeviceConfig,
  onOpenNotifications,
  onRequestReboot,
  onSignOut,
}: OverflowMenuProps) {
  const [open, setOpen] = useState(false);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    function handleClick(e: MouseEvent) {
      if (ref.current && !ref.current.contains(e.target as Node)) setOpen(false);
    }
    document.addEventListener("click", handleClick);
    return () => document.removeEventListener("click", handleClick);
  }, [open]);

  return (
    <div className="relative md:hidden" ref={ref}>
      <button
        type="button"
        onClick={() => setOpen((o) => !o)}
        className="min-h-[44px] min-w-[44px] flex items-center justify-center p-2 rounded-lg border border-surface-3 text-text-secondary
                 hover:border-accent-cyan/40 hover:text-accent-cyan transition-colors touch-manipulation
                 focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1"
        aria-label="Menu"
        aria-expanded={open}
      >
        <MoreVertical size={20} />
      </button>

      {open && (
        <div className="absolute right-0 top-full mt-1 py-2 min-w-[200px] rounded-xl border border-surface-4 bg-surface-2 shadow-lg z-50">
          <div className="px-3 py-2 border-b border-surface-3">
            <p className="text-[10px] font-mono text-text-muted uppercase tracking-wider">Signed in</p>
            <p className="text-xs font-mono text-text-primary truncate">{userEmail}</p>
            {isAdmin && (
              <span className="inline-block mt-1 text-[10px] font-mono text-accent-cyan border border-accent-cyan/30 rounded px-1.5 py-0.5">Admin</span>
            )}
          </div>
          <div className="py-1">
            <button
              type="button"
              onClick={() => { onOpenDeviceConfig(); setOpen(false); }}
              className="w-full flex items-center gap-2 px-3 py-2.5 text-left text-sm font-mono text-text-primary hover:bg-surface-3"
            >
              <Settings size={16} />
              Device settings
            </button>
            <button
              type="button"
              onClick={() => { onOpenNotifications(); setOpen(false); }}
              className="w-full flex items-center gap-2 px-3 py-2.5 text-left text-sm font-mono text-text-primary hover:bg-surface-3"
            >
              <Bell size={16} />
              Notifications
            </button>
            {isAdmin && (
              <button
                type="button"
                onClick={() => { onRequestReboot(); setOpen(false); }}
                disabled={!esp32Online}
                title={esp32Online ? "Restart controller" : "Controller must be online to restart"}
                className={clsx(
                  "w-full flex items-center gap-2 px-3 py-2.5 text-left text-sm font-mono hover:bg-surface-3",
                  esp32Online ? "text-text-primary" : "text-text-muted opacity-60"
                )}
              >
                <RotateCw size={16} />
                Restart controller
              </button>
            )}
            <button
              type="button"
              onClick={() => { onSignOut(); setOpen(false); }}
              className="w-full flex items-center gap-2 px-3 py-2.5 text-left text-sm font-mono text-accent-red hover:bg-surface-3"
            >
              <LogOut size={16} />
              Sign out
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
