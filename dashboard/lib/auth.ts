// lib/auth.ts
// ─────────────────────────────────────────────────────────────────────────────
// Firebase Auth helpers for Google sign-in. Used by dashboard and AuthGuard.
// ESP32 uses Email/Password; dashboard uses Google OAuth.
// ─────────────────────────────────────────────────────────────────────────────

import {
  GoogleAuthProvider,
  signInWithPopup,
  signOut as firebaseSignOut,
} from "firebase/auth";
import { auth } from "./firebase";

/** Comma-separated Firebase UIDs allowed to access the dashboard. */
const AUTHORIZED_UIDS = (process.env.NEXT_PUBLIC_AUTHORIZED_UIDS ?? "")
  .split(",")
  .map((s) => s.trim())
  .filter(Boolean);

export async function signInWithGoogle(): Promise<boolean> {
  try {
    const provider = new GoogleAuthProvider();
    const result = await signInWithPopup(auth, provider);

    if (AUTHORIZED_UIDS.length > 0) {
      const uid = result.user.uid;
      if (!AUTHORIZED_UIDS.includes(uid)) {
        await firebaseSignOut(auth);
        return false;
      }
    }
    return true;
  } catch (err) {
    console.error("[Auth] Google sign-in failed:", err);
    return false;
  }
}

export async function signOut(): Promise<void> {
  try {
    await firebaseSignOut(auth);
  } catch (err) {
    console.error("[Auth] Sign-out failed:", err);
  }
}

export function isUidAuthorized(uid: string): boolean {
  if (AUTHORIZED_UIDS.length === 0) return true;
  return AUTHORIZED_UIDS.includes(uid);
}
