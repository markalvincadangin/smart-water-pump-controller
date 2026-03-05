// components/NotificationSettings.tsx
"use client";

import { useState, useEffect } from "react";
import { Bell, X } from "lucide-react";
import { useNotificationConfig } from "@/lib/useNotificationConfig";
import type { NotificationConfig } from "@/lib/types";

interface NotificationSettingsProps {
  userUid: string | null;
  userEmail: string | null;
  onClose: () => void;
}

export default function NotificationSettings({ userUid, userEmail, onClose }: NotificationSettingsProps) {
  const { config, loading, saveConfig } = useNotificationConfig(userUid);
  const [form, setForm] = useState<NotificationConfig>({
    enabled: false,
    email: "",
    dryRunAlert: true,
    lowLevelAlert: true,
    lowLevelThreshold: 20,
    pumpStartedAlert: true,
  });
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [saveSuccess, setSaveSuccess] = useState(false);

  useEffect(() => {
    if (config) {
      setForm(config);
      if (!config.email && userEmail) {
        setForm((f) => ({ ...f, email: userEmail }));
      }
    }
  }, [config, userEmail]);

  async function handleSave() {
    setSaveError(null);
    setSaving(true);
    try {
      await saveConfig(form);
      setSaveSuccess(true);
      setTimeout(() => onClose(), 800);
    } catch (err: unknown) {
      let msg = "Failed to save. Check your connection and try again.";
      if (err && typeof err === "object" && "code" in err) {
        const code = (err as { code?: string }).code;
        if (code === "PERMISSION_DENIED") msg = "Permission denied. Deploy database rules: firebase deploy --only database";
        else if (code === "UNAVAILABLE") msg = "Database unavailable. Check your connection.";
        else msg = ("message" in err && typeof (err as { message?: string }).message === "string")
          ? (err as { message: string }).message
          : msg;
      } else if (err instanceof Error) {
        msg = err.message;
      }
      setSaveError(msg);
    } finally {
      setSaving(false);
    }
  }

  if (loading) {
    return (
      <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50">
        <div className="card p-8 max-w-md w-full mx-4">
          <div className="animate-pulse text-text-muted text-sm font-mono">Loading…</div>
        </div>
      </div>
    );
  }

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
      <div className="card card-glow-cyan p-6 max-w-md w-full max-h-[90vh] overflow-y-auto">
        <div className="flex items-center justify-between mb-6">
          <div className="flex items-center gap-3">
            <div className="p-2 rounded-lg bg-accent-cyan/10">
              <Bell size={20} className="text-accent-cyan" />
            </div>
            <div>
              <h2 className="font-display font-semibold text-text-primary">
                Notifications
              </h2>
              <p className="text-xs font-mono text-text-muted">
                Email alerts for high-risk events
              </p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-2 rounded-lg text-text-muted hover:text-text-primary hover:bg-surface-3 transition-colors"
            aria-label="Close"
          >
            <X size={20} />
          </button>
        </div>

        <div className="space-y-4">
          <label className="flex items-center gap-3 cursor-pointer">
            <input
              type="checkbox"
              checked={form.enabled}
              onChange={(e) => setForm((f) => ({ ...f, enabled: e.target.checked }))}
              className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-cyan focus:ring-accent-cyan/50"
            />
            <span className="text-sm font-mono text-text-primary">Enable email notifications for your account</span>
          </label>

          <div>
            <label className="block text-xs font-mono text-text-muted uppercase tracking-widest mb-1.5">
              Email address
            </label>
            <input
              type="email"
              value={form.email}
              onChange={(e) => setForm((f) => ({ ...f, email: e.target.value }))}
              placeholder="you@example.com"
              className="w-full px-4 py-2.5 rounded-xl bg-surface-2 border border-surface-4 text-text-primary
                         font-mono text-sm placeholder:text-text-muted
                         focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:border-transparent"
            />
          </div>

          <div className="pt-2 border-t border-surface-3">
            <p className="text-xs font-mono text-text-muted mb-3">Alert types</p>
            <div className="space-y-2.5">
              <label className="flex items-center gap-3 cursor-pointer">
                <input
                  type="checkbox"
                  checked={form.dryRunAlert}
                  onChange={(e) => setForm((f) => ({ ...f, dryRunAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-red focus:ring-accent-red/50"
                />
                <span className="text-sm font-mono text-text-primary">Dry-Run lockout (no flow while pump running)</span>
              </label>
              <label className="flex items-center gap-3 cursor-pointer">
                <input
                  type="checkbox"
                  checked={form.lowLevelAlert}
                  onChange={(e) => setForm((f) => ({ ...f, lowLevelAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-amber focus:ring-accent-amber/50"
                />
                <span className="text-sm font-mono text-text-primary">Low tank level warning</span>
              </label>
              <div className="pl-7 flex items-center gap-2">
                <span className="text-xs text-text-muted">Threshold:</span>
                <select
                  value={form.lowLevelThreshold}
                  onChange={(e) => setForm((f) => ({ ...f, lowLevelThreshold: parseInt(e.target.value, 10) }))}
                  className="px-2 py-1 rounded bg-surface-2 border border-surface-4 text-text-primary text-xs font-mono"
                >
                  {[10, 15, 20, 25, 30].map((n) => (
                    <option key={n} value={n}>{n}%</option>
                  ))}
                </select>
              </div>
              <label className="flex items-center gap-3 cursor-pointer">
                <input
                  type="checkbox"
                  checked={form.pumpStartedAlert}
                  onChange={(e) => setForm((f) => ({ ...f, pumpStartedAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-green focus:ring-accent-green/50"
                />
                <span className="text-sm font-mono text-text-primary">Pump started</span>
              </label>
            </div>
          </div>

          <p className="text-[10px] font-mono text-text-muted pt-2">
            Alerts are throttled to once per 15 minutes per type. Requires Cloud Functions and Resend API.
          </p>
        </div>

        {saveError && (
          <p className="mt-4 p-3 rounded-lg bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs font-mono">
            {saveError}
          </p>
        )}

        <div className="flex gap-3 mt-6">
          <button
            onClick={onClose}
            disabled={saving}
            className="flex-1 px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm
                       hover:bg-surface-3 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
          >
            Cancel
          </button>
          <button
            onClick={handleSave}
            disabled={saving || !form.email}
            className="flex-1 px-4 py-2.5 rounded-xl bg-accent-cyan/20 border border-accent-cyan/40
                       text-accent-cyan font-mono text-sm font-semibold
                       hover:bg-accent-cyan/30 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
          >
            {saving ? "Saving…" : saveSuccess ? "Saved" : "Save"}
          </button>
        </div>
      </div>
    </div>
  );
}
