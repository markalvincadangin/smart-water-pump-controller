/**
 * Smart Water Pump Controller — Cloud Functions
 * Sends email notifications for high-risk events: dry-run, low tank, pump started.
 *
 * Setup: firebase functions:secrets:set RESEND_API_KEY
 * Optional: create .env in functions/ with RESEND_FROM_EMAIL=...
 */

import * as admin from "firebase-admin";
import { onValueWritten } from "firebase-functions/v2/database";
import { defineSecret } from "firebase-functions/params";
import { logger } from "firebase-functions";
import { Resend } from "resend";

admin.initializeApp();

const db = admin.database();

const resendApiKey = defineSecret("RESEND_API_KEY");

// Throttle: minimum seconds between same alert type
const THROTTLE_SEC = 15 * 60; // 15 minutes

interface PumpStatus {
  water_level_percent: number;
  is_running: boolean;
  flow_rate_lpm: number;
  is_error: boolean;
  is_overflow_error?: boolean;  // Phase 2: max runtime exceeded
}

interface NotificationConfig {
  enabled?: boolean;
  email?: string;
  dryRunAlert?: boolean;
  lowLevelAlert?: boolean;
  lowLevelThreshold?: number;
  pumpStartedAlert?: boolean;
  overflowAlert?: boolean;  // Phase 2: max runtime overflow
}

interface LastSent {
  dryRun?: number;
  lowLevel?: number;
  pumpStarted?: number;
  overflow?: number;
}

async function canSend(uid: string, type: keyof LastSent): Promise<boolean> {
  const lastRef = db.ref(`pump_system/config/notification_last_sent/${uid}`);
  const snap = await lastRef.get();
  const last: LastSent = snap.val() || {};
  const now = Math.floor(Date.now() / 1000);
  const lastTime = last[type] ?? 0;
  if (now - lastTime < THROTTLE_SEC) return false;
  return true;
}

async function recordSent(uid: string, type: keyof LastSent): Promise<void> {
  const lastRef = db.ref(`pump_system/config/notification_last_sent/${uid}`);
  await lastRef.update({ [type]: Math.floor(Date.now() / 1000) });
}

async function sendEmail(
  apiKey: string,
  to: string,
  subject: string,
  html: string
): Promise<boolean> {
  const resend = new Resend(apiKey);
  const from = process.env.RESEND_FROM_EMAIL || "Smart Water Pump <onboarding@resend.dev>";

  try {
    const { error } = await resend.emails.send({
      from,
      to,
      subject,
      html,
    });
    if (error) {
      logger.error("Resend error:", error);
      return false;
    }
    return true;
  } catch (err) {
    logger.error("Email send failed:", err);
    return false;
  }
}

async function getActiveNotificationConfigs(): Promise<Array<{ uid: string; config: NotificationConfig }>> {
  const snap = await db.ref("pump_system/config/notifications_by_user").get();
  const all = snap.val() as Record<string, NotificationConfig> | null;
  if (!all) return [];

  return Object.entries(all)
    .map(([uid, cfg]) => ({ uid, config: cfg || {} }))
    .filter(({ config }) => config.enabled && !!config.email);
}

export const onStatusChange = onValueWritten(
  {
    ref: "/pump_system/status",
    region: "asia-southeast1",
    secrets: [resendApiKey],
  },
  async (event) => {
    const after = event.data.after.val() as PumpStatus | null;
    const before = event.data.before?.val() as PumpStatus | null;

    if (!after) return;

    const configs = await getActiveNotificationConfigs();
    if (!configs.length) return;

    const apiKey = resendApiKey.value();
    if (!apiKey) {
      logger.warn("RESEND_API_KEY not set — skipping email");
      return;
    }

    for (const { uid, config } of configs) {
      const threshold = config.lowLevelThreshold ?? 20;

      // 1. Dry-Run Lockout (highest priority)
      if (after.is_error && (config.dryRunAlert ?? true)) {
        if (await canSend(uid, "dryRun")) {
          const sent = await sendEmail(
            apiKey,
            config.email as string,
            "⚠ Smart Water Pump — Dry-Run Lockout Active",
            `
            <h2>Dry-Run Protection Triggered</h2>
            <p>The pump has shut down due to no flow detected for 30 seconds. This protects the motor from running dry.</p>
            <ul>
              <li><strong>Flow:</strong> ${after.flow_rate_lpm.toFixed(1)} LPM</li>
              <li><strong>Tank level:</strong> ${after.water_level_percent}%</li>
            </ul>
            <p><strong>Action:</strong> Check the pump and water source. Acknowledge the error in the dashboard to resume.</p>
          `
          );
          if (sent) await recordSent(uid, "dryRun");
        }
      }

      // 1b. Overflow (max runtime exceeded) — Phase 2
      if (after.is_overflow_error && (config.overflowAlert ?? true)) {
        if (await canSend(uid, "overflow")) {
          const sent = await sendEmail(
            apiKey,
            config.email as string,
            "⚠ Smart Water Pump — Overflow Protection Triggered",
            `
            <h2>Max Runtime / Overflow Protection Triggered</h2>
            <p>The pump has shut down because it ran longer than the configured max runtime without reaching stop level. This may indicate tank overflow, sensor failure, or a stuck fill.</p>
            <ul>
              <li><strong>Tank level:</strong> ${after.water_level_percent}%</li>
              <li><strong>Flow:</strong> ${after.flow_rate_lpm.toFixed(1)} LPM</li>
            </ul>
            <p><strong>Action:</strong> Check the tank and ultrasonic sensor. Acknowledge the error in the dashboard to resume.</p>
          `
          );
          if (sent) await recordSent(uid, "overflow");
        }
      }

      // 2. Low tank level warning
      if (after.water_level_percent <= threshold && (config.lowLevelAlert ?? true)) {
        if (await canSend(uid, "lowLevel")) {
          const sent = await sendEmail(
            apiKey,
            config.email as string,
            `⚠ Smart Water Pump — Low Tank Level (${after.water_level_percent}%)`,
            `
            <h2>Low Tank Level Warning</h2>
            <p>Water level is at <strong>${after.water_level_percent}%</strong> (threshold: ${threshold}%).</p>
            <ul>
              <li><strong>Pump:</strong> ${after.is_running ? "Running" : "Stopped"}</li>
              <li><strong>Flow:</strong> ${after.flow_rate_lpm.toFixed(1)} LPM</li>
            </ul>
            <p>Monitor the system. In AUTO mode, the pump starts when level drops to 30%.</p>
          `
          );
          if (sent) await recordSent(uid, "lowLevel");
        }
      }

      // 3. Pump just started (transition from off → on)
      if ((config.pumpStartedAlert ?? true) && after.is_running && before && !before.is_running) {
        if (await canSend(uid, "pumpStarted")) {
          const sent = await sendEmail(
            apiKey,
            config.email as string,
            "▶ Smart Water Pump — Pump Started",
            `
            <h2>Pump Started</h2>
            <p>The pump is now running.</p>
            <ul>
              <li><strong>Tank level:</strong> ${after.water_level_percent}%</li>
              <li><strong>Flow:</strong> ${after.flow_rate_lpm.toFixed(1)} LPM</li>
            </ul>
          `
          );
          if (sent) await recordSent(uid, "pumpStarted");
        }
      }
    }
  }
);
