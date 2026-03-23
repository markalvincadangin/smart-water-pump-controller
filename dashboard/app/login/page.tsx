// app/login/page.tsx
"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { signInWithGoogle } from "@/lib/auth";
import { onAuthStateChanged } from "firebase/auth";
import { auth } from "@/lib/firebase";
import Logo from "@/components/Logo";

export default function LoginPage() {
  const router = useRouter();
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    const unsub = onAuthStateChanged(auth, (user) => {
      if (user) {
        router.replace("/");
      }
    });
    return () => unsub();
  }, [router]);

  async function handleSignIn() {
    setError(null);
    setLoading(true);
    const ok = await signInWithGoogle();
    setLoading(false);
    if (ok) {
      router.replace("/");
    } else {
      setError("Sign-in failed or access denied.");
    }
  }

  return (
    <div className="min-h-screen min-h-[100dvh] flex items-center justify-center bg-surface p-4 sm:p-6
                 pt-[env(safe-area-inset-top)] pb-[env(safe-area-inset-bottom)]
                 pl-[max(1rem,env(safe-area-inset-left))] pr-[max(1rem,env(safe-area-inset-right))]">
      <div className="card card-glow-cyan p-6 sm:p-10 flex flex-col items-center gap-6 w-full max-w-[22rem] sm:max-w-xs mx-auto min-w-0">
        <div className="flex flex-col items-center gap-4">
          <div className="w-16 h-16 sm:w-20 sm:h-20 flex items-center justify-center rounded-2xl bg-accent-cyan/10 border border-accent-cyan/20">
            <Logo size="lg" />
          </div>
          <div>
            <p className="text-xs font-mono text-text-muted uppercase tracking-widest text-center mb-1">
              SmartFlow
            </p>
            <h1 className="font-display text-xl sm:text-2xl font-bold text-text-primary text-center">
              Control Dashboard
            </h1>
          </div>
        </div>

        <button
          onClick={handleSignIn}
          disabled={loading}
          className="w-full flex items-center justify-center gap-3 px-4 py-4 min-h-[48px] rounded-xl
                     bg-white text-gray-800 font-medium text-sm
                     hover:bg-gray-100 disabled:opacity-50 disabled:cursor-not-allowed
                     transition-colors touch-manipulation active:scale-[0.99]
                     focus:outline-none focus:ring-2 focus:ring-accent-cyan/50 focus:ring-offset-2 focus:ring-offset-surface-1"
        >
          <svg width="18" height="18" viewBox="0 0 48 48">
            <path fill="#EA4335" d="M24 9.5c3.54 0 6.71 1.22 9.21 3.6l6.85-6.85C35.9 2.38 30.47 0 24 0 14.62 0 6.51 5.38 2.56 13.22l7.98 6.19C12.43 13.72 17.74 9.5 24 9.5z"/>
            <path fill="#4285F4" d="M46.98 24.55c0-1.57-.15-3.09-.38-4.55H24v9.02h12.94c-.58 2.96-2.26 5.48-4.78 7.18l7.73 6c4.51-4.18 7.09-10.36 7.09-17.65z"/>
            <path fill="#FBBC05" d="M10.53 28.59c-.48-1.45-.76-2.99-.76-4.59s.27-3.14.76-4.59l-7.98-6.19C.92 16.46 0 20.12 0 24c0 3.88.92 7.54 2.56 10.78l7.97-6.19z"/>
            <path fill="#34A853" d="M24 48c6.48 0 11.93-2.13 15.89-5.81l-7.73-6c-2.18 1.48-4.97 2.31-8.16 2.31-6.26 0-11.57-4.22-13.47-9.91l-7.98 6.19C6.51 42.62 14.62 48 24 48z"/>
          </svg>
          {loading ? "Signing in..." : "Sign in with Google"}
        </button>

        {error && (
          <p className="text-xs text-accent-red text-center font-mono">{error}</p>
        )}

        <p className="text-xs text-text-muted text-center font-mono">
          Authorized users only
        </p>
      </div>
    </div>
  );
}
