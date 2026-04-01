// lib/firebase.ts
// ─────────────────────────────────────────────────────────────────────────────
// Firebase init. Dashboard uses Google Auth; ESP32 uses Email/Password Auth (see firmware secrets.h).
// Firebase Console → Project Settings → General → Your apps → SDK setup
// ─────────────────────────────────────────────────────────────────────────────

import { initializeApp, getApps, getApp } from "firebase/app";
import { getDatabase } from "firebase/database";
import { getAuth } from "firebase/auth";

function normalizeDatabaseUrl(rawUrl?: string): string {
  const fallback = "https://YOUR_PROJECT-default-rtdb.firebaseio.com";
  if (!rawUrl) return fallback;
  const url = rawUrl.trim();
  try {
    const parsed = new URL(url);
    const host = parsed.hostname.toLowerCase();
    if (host.endsWith(".firebaseio.com")) return parsed.toString();
    if (host.endsWith(".firebasedatabase.app")) {
      const projectHost = host.split(".")[0];
      return `https://${projectHost}.firebaseio.com`;
    }
    return fallback;
  } catch {
    return fallback;
  }
}

const firebaseConfig = {
  apiKey:            process.env.NEXT_PUBLIC_FIREBASE_API_KEY     ?? "YOUR_API_KEY",
  authDomain:        process.env.NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN ?? "YOUR_PROJECT.firebaseapp.com",
  databaseURL:       normalizeDatabaseUrl(process.env.NEXT_PUBLIC_FIREBASE_DATABASE_URL),
  projectId:         process.env.NEXT_PUBLIC_FIREBASE_PROJECT_ID  ?? "YOUR_PROJECT_ID",
  storageBucket:     process.env.NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET ?? "YOUR_PROJECT.appspot.com",
  messagingSenderId: process.env.NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID ?? "YOUR_SENDER_ID",
  appId:             process.env.NEXT_PUBLIC_FIREBASE_APP_ID      ?? "YOUR_APP_ID",
};

// Prevent duplicate app initialization in Next.js hot-reload
const app = getApps().length ? getApp() : initializeApp(firebaseConfig);

export const db   = getDatabase(app);
export const auth = getAuth(app);
