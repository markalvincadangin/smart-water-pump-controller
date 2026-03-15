/**
 * Production health check: 200 OK only when Firebase client is successfully initialized.
 * Use for liveness/readiness probes and post-deploy verification.
 * Server-side: ensures env config is present and Firebase app can be initialized.
 */
import { getApps, getApp, initializeApp } from "firebase/app";
import { NextResponse } from "next/server";

const PLACEHOLDER_PROJECT = "YOUR_PROJECT_ID";
const PLACEHOLDER_API_KEY = "YOUR_API_KEY";

function isFirebaseInitialized(): boolean {
  try {
    let app = getApps().length ? getApp() : null;
    if (!app) {
      const projectId = process.env.NEXT_PUBLIC_FIREBASE_PROJECT_ID;
      const apiKey = process.env.NEXT_PUBLIC_FIREBASE_API_KEY;
      if (!projectId || !apiKey || projectId === PLACEHOLDER_PROJECT || apiKey === PLACEHOLDER_API_KEY) {
        return false;
      }
      app = initializeApp({
        apiKey: process.env.NEXT_PUBLIC_FIREBASE_API_KEY,
        authDomain: process.env.NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN,
        databaseURL: process.env.NEXT_PUBLIC_FIREBASE_DATABASE_URL,
        projectId: process.env.NEXT_PUBLIC_FIREBASE_PROJECT_ID,
        storageBucket: process.env.NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET,
        messagingSenderId: process.env.NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID,
        appId: process.env.NEXT_PUBLIC_FIREBASE_APP_ID,
      });
    }
    const options = app.options;
    const projectId = options.projectId;
    const apiKey = options.apiKey;
    if (!projectId || !apiKey) return false;
    if (projectId === PLACEHOLDER_PROJECT || apiKey === PLACEHOLDER_API_KEY) return false;
    return true;
  } catch {
    return false;
  }
}

export async function GET() {
  const firebaseOk = isFirebaseInitialized();
  const status = firebaseOk ? 200 : 503;
  const body = {
    status: firebaseOk ? "ok" : "degraded",
    firebase: firebaseOk ? "initialized" : "not_initialized",
    timestamp: new Date().toISOString(),
  };
  return NextResponse.json(body, { status });
}
