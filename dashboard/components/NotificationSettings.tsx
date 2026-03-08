// components/NotificationSettings.tsx
"use client";

import { useState, useEffect } from "react";
import { Bell, X, Smartphone } from "lucide-react";
import { useNotificationConfig } from "@/lib/useNotificationConfig";
import InfoTooltip from "./InfoTooltip";
import { isPushSupported, requestPushToken, getFcmDeviceId } from "@/lib/fcm";
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
    pushEnabled: false,
    fcmTokens: {},
    dryRunAlert: true,
    lowLevelAlert: true,
    lowLevelThreshold: 20,
    pumpStartedAlert: true,
    overflowAlert: true,
  });
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [saveSuccess, setSaveSuccess] = useState(false);
  const [pushSupported, setPushSupported] = useState<boolean | null>(null);
  const [enablingPush, setEnablingPush] = useState(false);

  useEffect(() => {
    if (config) {
      setForm((prev) => ({
        ...prev,
        ...config,
        fcmTokens: config.fcmTokens ?? {},
      }));
      if (!config.email && userEmail) {
        setForm((f) => ({ ...f, email: userEmail }));
      }
    }
  }, [config, userEmail]);

  useEffect(() => {
    isPushSupported().then(setPushSupported);
  }, []);

  async function handleEnablePush() {
    if (!userUid) return;
    setEnablingPush(true);
    setSaveError(null);
    try {
      const token = await requestPushToken();
      if (!token) {
        setSaveError("Could not enable push. Allow notifications when prompted, or try again.");
        return;
      }
      const deviceId = getFcmDeviceId();
      setForm((f) => ({
        ...f,
        pushEnabled: true,
        fcmTokens: { ...(f.fcmTokens ?? {}), [deviceId]: token },
      }));
      // Don't auto-save; user can review and save
    } catch (err) {
      setSaveError(err instanceof Error ? err.message : "Failed to enable push.");
    } finally {
      setEnablingPush(false);
    }
  }

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
    <div className="fixed inset-0 z-50 flex items-end sm:items-center justify-center p-0 sm:p-4 bg-black/50 overscroll-contain pt-[env(safe-area-inset-top)] pb-[env(safe-area-inset-bottom)] pl-[max(0.5rem,env(safe-area-inset-left))] pr-[max(0.5rem,env(safe-area-inset-right))]">
      <div className="card card-glow-cyan max-w-md w-full max-h-[95dvh] sm:max-h-[90vh] min-w-0 rounded-t-2xl sm:rounded-2xl flex flex-col overflow-hidden">
        <div className="flex items-center justify-between p-4 sm:p-6 pb-0 shrink-0">
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

        <div className="flex-1 overflow-y-auto min-h-0 px-4 sm:px-6 py-4">
          <div className="space-y-4">
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
            <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Delivery methods</p>
            <InfoTooltip content="Choose how you want to receive alerts: email, push to your phone/browser (like YouTube or Facebook), or both." />
          </div>

          <label className="flex items-center gap-3 cursor-pointer min-h-[44px] py-1">
            <input
              type="checkbox"
              checked={form.enabled}
              onChange={(e) => setForm((f) => ({ ...f, enabled: e.target.checked }))}
              className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-cyan focus:ring-accent-cyan/50"
            />
            <span className="text-sm font-mono text-text-primary">Enable email notifications</span>
            <InfoTooltip content="Alerts sent to your email inbox. Requires Resend API in Cloud Functions." side="right" />
          </label>

          <div>
            <label className="flex flex-wrap items-center gap-x-2 gap-y-1 text-xs font-mono text-text-muted uppercase tracking-widest mb-1.5">
              Email address
              <InfoTooltip content="Where to send email alerts. Use the address linked to your Google account or any other." side="right" />
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

          {/* Push notifications */}
          {pushSupported && (
            <div className="pt-2 border-t border-surface-3">
              <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-2">
                <Smartphone size={16} className="text-accent-cyan" />
                <span className="text-sm font-mono text-text-primary">Push notifications</span>
                <InfoTooltip content="Alerts sent directly to your phone or browser — like YouTube, Facebook, or Instagram. Works when the app is closed. Install the app (Add to Home Screen) for best experience." maxWidth="300px" />
              </div>
              {(() => {
                const deviceId = getFcmDeviceId();
                const hasToken = Boolean(form.fcmTokens?.[deviceId]);
                return hasToken ? (
                  <label className="flex items-center gap-3 cursor-pointer min-h-[44px] py-1">
                    <input
                      type="checkbox"
                      checked={true}
                      onChange={(e) => {
                        if (!e.target.checked) {
                          const tokens = { ...(form.fcmTokens ?? {}) };
                          delete tokens[deviceId];
                          setForm((f) => ({ ...f, fcmTokens: tokens }));
                        }
                      }}
                      className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-cyan focus:ring-accent-cyan/50"
                    />
                    <span className="text-sm font-mono text-accent-green">Push enabled on this device</span>
                  </label>
                ) : (
                  <button
                    type="button"
                    onClick={handleEnablePush}
                    disabled={enablingPush}
                    className="px-3 py-2 rounded-lg border border-accent-cyan/40 text-accent-cyan text-xs font-mono hover:bg-accent-cyan/10 disabled:opacity-50"
                  >
                    {enablingPush ? "Requesting permission…" : "Enable push on this device"}
                  </button>
                );
              })()}
            </div>
          )}

          <div className="pt-2 border-t border-surface-3">
            <div className="flex flex-wrap items-center gap-x-2 gap-y-1 mb-3">
              <p className="text-xs font-mono text-text-muted uppercase tracking-widest">Alert types</p>
              <InfoTooltip content="Select which events trigger notifications. All high-risk events are recommended." side="right" />
            </div>
            <div className="space-y-2.5">
              <label className="flex items-center gap-3 cursor-pointer min-h-[44px] py-1">
                <input
                  type="checkbox"
                  checked={form.dryRunAlert}
                  onChange={(e) => setForm((f) => ({ ...f, dryRunAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-red focus:ring-accent-red/50"
                />
                <span className="text-sm font-mono text-text-primary">Dry-Run lockout</span>
                <InfoTooltip content="Pump shut down due to no water flow (protects motor from running dry). Critical — recommend keeping on." side="right" />
              </label>
              <label className="flex items-center gap-3 cursor-pointer min-h-[44px] py-1">
                <input
                  type="checkbox"
                  checked={form.lowLevelAlert}
                  onChange={(e) => setForm((f) => ({ ...f, lowLevelAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-amber focus:ring-accent-amber/50"
                />
                <span className="text-sm font-mono text-text-primary">Low tank level warning</span>
                <InfoTooltip content="Fires when tank drops to or below your threshold %. Helps avoid running out of water." side="right" />
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
              <label className="flex items-center gap-3 cursor-pointer min-h-[44px] py-1">
                <input
                  type="checkbox"
                  checked={form.pumpStartedAlert}
                  onChange={(e) => setForm((f) => ({ ...f, pumpStartedAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-green focus:ring-accent-green/50"
                />
                <span className="text-sm font-mono text-text-primary">Pump started</span>
                <InfoTooltip content="Fires when pump turns ON. Useful to know refill has begun." side="right" />
              </label>
              <label className="flex items-center gap-3 cursor-pointer min-h-[44px] py-1">
                <input
                  type="checkbox"
                  checked={form.overflowAlert}
                  onChange={(e) => setForm((f) => ({ ...f, overflowAlert: e.target.checked }))}
                  className="w-4 h-4 rounded border-surface-4 bg-surface-2 text-accent-red focus:ring-accent-red/50"
                />
                <span className="text-sm font-mono text-text-primary">Overflow alert</span>
                <InfoTooltip content="Pump ran longer than max runtime without reaching stop level. May indicate tank overflow or sensor issue." side="right" />
              </label>
            </div>
          </div>

          <p className="text-[10px] font-mono text-text-muted pt-2">
            Alerts are throttled to once per 15 minutes per type. Email requires Resend API; push requires HTTPS and browser permission.
          </p>
          </div>

          {saveError && (
            <p className="mt-4 p-3 rounded-lg bg-accent-red/10 border border-accent-red/30 text-accent-red text-xs font-mono">
              {saveError}
            </p>
          )}
        </div>

        <div className="shrink-0 flex gap-3 p-4 sm:p-6 pt-4 border-t border-surface-3 bg-surface-1">
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
            disabled={saving || (form.enabled && !form.email && !Object.keys(form.fcmTokens ?? {}).length)}
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
