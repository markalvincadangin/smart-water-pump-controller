/**
 * Firebase Cloud Messaging (FCM) — Push notifications to phone/browser
 * Like YouTube, Facebook, etc. — alerts appear on device even when app is closed.
 *
 * Setup:
 * 1. Firebase Console → Project Settings → Cloud Messaging → Web Push certificates → Generate key pair
 * 2. Add NEXT_PUBLIC_FIREBASE_VAPID_KEY to .env.local
 */

const VAPID_KEY = process.env.NEXT_PUBLIC_FIREBASE_VAPID_KEY;

/** Generate a simple device ID for this browser/device */
export function getFcmDeviceId(): string {
  const key = "pump_fcm_device_id";
  let id = typeof localStorage !== "undefined" ? localStorage.getItem(key) : null;
  if (!id) {
    id = `web_${Date.now()}_${Math.random().toString(36).slice(2, 11)}`;
    if (typeof localStorage !== "undefined") localStorage.setItem(key, id);
  }
  return id;
}

/** Check if FCM is supported (HTTPS, service worker, Push API) */
export async function isPushSupported(): Promise<boolean> {
  if (typeof window === "undefined") return false;
  try {
    const { isSupported } = await import("firebase/messaging");
    const supported = await isSupported();
    return supported && !!VAPID_KEY;
  } catch {
    return false;
  }
}

/** Request notification permission and get FCM token. Returns null if denied or unsupported. */
export async function requestPushToken(): Promise<string | null> {
  if (typeof window === "undefined") return null;
  if (!VAPID_KEY) {
    console.warn("[FCM] NEXT_PUBLIC_FIREBASE_VAPID_KEY not set");
    return null;
  }

  try {
    const { getMessaging, getToken, isSupported } = await import("firebase/messaging");
    const { getApp } = await import("firebase/app");

    const supported = await isSupported();
    if (!supported) {
      console.warn("[FCM] Not supported (HTTPS, service worker, or Push API missing)");
      return null;
    }

    const app = getApp();
    const messaging = getMessaging(app);
    const token = await getToken(messaging, { vapidKey: VAPID_KEY });
    return token || null;
  } catch (err) {
    console.error("[FCM] getToken failed:", err);
    return null;
  }
}
