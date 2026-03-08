"use client";

import { useState, useEffect } from "react";
import { Download, X } from "lucide-react";

/**
 * Shows "Install app" banner on mobile when the PWA is installable but not installed.
 * Dismisses permanently for the session when closed.
 */
export default function InstallPrompt() {
  const [show, setShow] = useState(false);
  const [deferredPrompt, setDeferredPrompt] = useState<BeforeInstallPromptEvent | null>(null);

  useEffect(() => {
    const handler = (e: Event) => {
      e.preventDefault();
      setDeferredPrompt(e as BeforeInstallPromptEvent);
      setShow(true);
    };
    window.addEventListener("beforeinstallprompt", handler);
    return () => window.removeEventListener("beforeinstallprompt", handler);
  }, []);

  useEffect(() => {
    if (window.matchMedia("(display-mode: standalone)").matches) {
      setShow(false);
    }
  }, []);

  async function handleInstall() {
    if (!deferredPrompt) return;
    deferredPrompt.prompt();
    const { outcome } = await deferredPrompt.userChoice;
    if (outcome === "accepted") setShow(false);
    setDeferredPrompt(null);
  }

  if (!show) return null;

  return (
    <div className="fixed bottom-4 left-3 right-3 z-40 md:left-6 md:right-6 md:max-w-md md:mx-auto pb-[env(safe-area-inset-bottom)]">
      <div className="flex items-center gap-3 p-3 sm:p-4 rounded-xl bg-surface-2 border border-accent-cyan/30 shadow-lg card-glow-cyan">
        <div className="p-2 rounded-lg bg-accent-cyan/10 shrink-0">
          <Download size={18} className="text-accent-cyan" />
        </div>
        <div className="flex-1 min-w-0">
          <p className="text-xs font-mono text-text-primary font-medium">Install app</p>
          <p className="text-[10px] font-mono text-text-muted mt-0.5">Add to home screen for quick access</p>
        </div>
        <div className="flex gap-2 shrink-0">
          <button
            onClick={handleInstall}
            className="px-4 py-2.5 sm:py-1.5 min-h-[44px] sm:min-h-0 rounded-lg bg-accent-cyan/20 border border-accent-cyan/40 text-accent-cyan text-xs font-mono font-medium touch-manipulation"
          >
            Install
          </button>
          <button
            onClick={() => setShow(false)}
            className="p-2.5 sm:p-1.5 min-h-[44px] min-w-[44px] sm:min-h-0 sm:min-w-0 flex items-center justify-center rounded-lg text-text-muted hover:text-text-primary hover:bg-surface-3 touch-manipulation"
            aria-label="Dismiss"
          >
            <X size={14} />
          </button>
        </div>
      </div>
    </div>
  );
}

interface BeforeInstallPromptEvent extends Event {
  prompt: () => Promise<void>;
  userChoice: Promise<{ outcome: "accepted" | "dismissed" }>;
}
