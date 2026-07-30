/**
 * Smart Water Pump Controller — Cloud Functions
 * Sends FCM push notifications for high-risk events: dry-run, overflow, low tank, pump started.
 *
 * No email dependencies. Requires Firebase Cloud Messaging enabled on the project.
 * Users must store their FCM token at: /users/{uid}/notification_prefs/fcmTokens/{tokenId}
 */

import * as admin from "firebase-admin";
import { onValueCreated, onValueWritten } from "firebase-functions/v2/database";
import { logger } from "firebase-functions";
import { canSend, recordSent } from "./notifications";

export {
  bootstrapDevice,
  setDeviceBootstrapState,
  requestWifiReprovision,
  claimDevice,
  startOwnershipTransfer,
  releaseDevice,
  cancelOwnershipPairing,
  checkAccountDeletionEligibility,
} from "./device_bootstrap";

admin.initializeApp();

// Firebase injects the RTDB URL only in the Functions runtime. Resolve the
// database lazily so local export discovery and deployment do not fail first.
const db = () => admin.database();

const MAX_DEVICE_EVENT_RECORDS = 50;

/**
 * Retains a bounded, server-authoritative diagnostic history for every device.
 *
 * Firmware can append WARN/ERROR records while disconnected/reconnecting, but
 * pruning belongs to a trusted backend principal: it cannot be bypassed by a
 * device request failure and it repairs histories created by older firmware.
 */
export const retainDeviceEvents = onValueCreated(
  {
    ref: "/devices/{deviceId}/events/{eventId}",
    region: "asia-southeast1",
  },
  async (event) => {
    const eventsRef = db().ref(`devices/${event.params.deviceId}/events`);

    await eventsRef.transaction((current: unknown) => {
      if (!current || typeof current !== "object" || Array.isArray(current)) {
        return current;
      }

      const entries = Object.entries(current as Record<string, unknown>);
      if (entries.length <= MAX_DEVICE_EVENT_RECORDS) {
        return current;
      }

      // RTDB push IDs sort chronologically, so keeping the greatest keys keeps
      // the newest records without trusting device-provided timestamps.
      const newestEntries = entries
        .sort(([left], [right]) => left.localeCompare(right))
        .slice(-MAX_DEVICE_EVENT_RECORDS);
      return Object.fromEntries(newestEntries);
    }, undefined, false);
  }
);

interface NotificationConfig {
  enabled?: boolean;
  fcmTokens?: Record<string, string>;  // tokenId -> FCM token
  dryRunAlert?: boolean;
  lowLevelAlert?: boolean;
  lowLevelThreshold?: number;
  pumpStartedAlert?: boolean;
  overflowAlert?: boolean;
}

async function sendPush(
  tokens: string[],
  title: string,
  body: string,
  tag: string
): Promise<void> {
  if (tokens.length === 0) return;
  const messaging = admin.messaging();
  const base = {
    notification: { title, body },
    data: { tag },
    android: { priority: "high" as const, notification: { channelId: "pump_alerts" } },
    apns: { payload: { aps: { sound: "default", badge: 1 } } },
  };
  try {
    if (tokens.length === 1) {
      await messaging.send({ ...base, token: tokens[0] });
    } else {
      const result = await messaging.sendEachForMulticast({ ...base, tokens });
      if (result.failureCount > 0) {
        result.responses.forEach((r: any, i: number) => {
          if (!r.success) logger.warn("FCM send failed for token", i, r.error?.message);
        });
      }
    }
  } catch (err) {
    logger.error("Push send failed:", err);
  }
}

async function getActiveNotificationConfigs(): Promise<Array<{ uid: string; config: NotificationConfig }>> {
  const snap = await db().ref("users").get();
  const allUsers = snap.val() as Record<string, { notification_prefs?: NotificationConfig }> | null;
  if (!allUsers) return [];

  return Object.entries(allUsers)
    .map(([uid, data]) => ({ uid, config: data.notification_prefs || {} }))
    .filter(({ config }) => {
      if (!config.enabled) return false;
      return config.fcmTokens && Object.keys(config.fcmTokens).length > 0;
    });
}

const getFcmTokens = (config: NotificationConfig): string[] =>
  config.fcmTokens ? Object.values(config.fcmTokens).filter(Boolean) : [];

export const onDeviceUpdated = onValueWritten(
  {
    ref: "/devices/{deviceId}",
    region: "asia-southeast1",
  },
  async (event: any) => {
    const after = event.data.after.val();
    const before = event.data.before?.val();

    if (!after) return;

    // Extract data from V2 schema
    const waterLevel = after.telemetry?.waterLevel ?? 0;
    const flowRate = after.telemetry?.flowRate ?? 0;
    const isRunning = after.shadow?.reported?.is_running ?? false;
    const wasRunning = before?.shadow?.reported?.is_running ?? false;

    // Check for recent error events
    const eventsMap = after.events as Record<string, any> | undefined;
    const latestEvent = eventsMap
      ? Object.values(eventsMap).sort((a, b) => b.timestamp - a.timestamp)[0]
      : null;
    const isDryRunError = latestEvent?.code === "DRY_RUN";
    const isOverflowError = latestEvent?.code === "OVERFLOW";

    const configs = await getActiveNotificationConfigs();
    if (!configs.length) return;

    for (const { uid, config } of configs) {
      // Check if user owns this device
      const userDevicesSnap = await db().ref(`users/${uid}/devices/${event.params.deviceId}`).get();
      if (!userDevicesSnap.exists()) continue;

      const threshold = config.lowLevelThreshold ?? 20;
      const tokens = getFcmTokens(config);
      if (tokens.length === 0) continue;

      // 1. Dry-Run Lockout
      if (isDryRunError && (config.dryRunAlert ?? true)) {
        if (await canSend(db(), uid, "dryRun")) {
          await sendPush(
            tokens,
            "⚠ Dry-Run Lockout",
            `No flow detected. Tank: ${waterLevel}%. Check pump and water source.`,
            "dryRun"
          );
          await recordSent(db(), uid, "dryRun");
        }
      }

      // 2. Overflow Protection
      if (isOverflowError && (config.overflowAlert ?? true)) {
        if (await canSend(db(), uid, "overflow")) {
          await sendPush(
            tokens,
            "⚠ Overflow Protection",
            `Max runtime exceeded. Tank: ${waterLevel}%. Check tank and sensor.`,
            "overflow"
          );
          await recordSent(db(), uid, "overflow");
        }
      }

      // 3. Low tank level
      if (waterLevel <= threshold && (config.lowLevelAlert ?? true)) {
        if (await canSend(db(), uid, "lowLevel")) {
          await sendPush(
            tokens,
            `⚠ Low Tank (${waterLevel}%)`,
            `Water at ${waterLevel}% (threshold: ${threshold}%). Pump: ${isRunning ? "Running" : "Stopped"}.`,
            "lowLevel"
          );
          await recordSent(db(), uid, "lowLevel");
        }
      }

      // 4. Pump just started
      if ((config.pumpStartedAlert ?? true) && isRunning && !wasRunning) {
        if (await canSend(db(), uid, "pumpStarted")) {
          await sendPush(
            tokens,
            "▶ Pump Started",
            `Tank: ${waterLevel}%, Flow: ${flowRate.toFixed(1)} LPM`,
            "pumpStarted"
          );
          await recordSent(db(), uid, "pumpStarted");
        }
      }
    }
  }
);
