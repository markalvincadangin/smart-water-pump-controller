"use client";

import Link from "next/link";

export default function Error({
  error,
  reset,
}: {
  error: Error & { digest?: string };
  reset: () => void;
}) {
  return (
    <div className="min-h-screen flex items-center justify-center p-4 sm:p-6">
      <div className="card card-glow-red max-w-md w-full p-6 sm:p-8">
        <p className="text-xs font-mono text-accent-red uppercase tracking-widest mb-2">Something went wrong</p>
        <h1 className="font-display text-xl sm:text-2xl font-bold text-text-primary">Dashboard error</h1>
        <p className="text-xs font-mono text-text-muted mt-3 break-words">
          {error?.message || "An unexpected error occurred."}
        </p>
        <div className="flex gap-3 mt-5">
          <button
            type="button"
            onClick={reset}
            className="flex-1 px-4 py-2.5 rounded-xl bg-accent-cyan/20 border border-accent-cyan/40 text-accent-cyan font-mono text-sm font-semibold hover:bg-accent-cyan/30 transition-colors"
          >
            Retry
          </button>
          <Link
            href="/"
            className="flex-1 px-4 py-2.5 rounded-xl border border-surface-4 text-text-secondary font-mono text-sm text-center hover:bg-surface-3 transition-colors"
          >
            Home
          </Link>
        </div>
        {error?.digest && (
          <p className="text-[10px] font-mono text-text-muted mt-4">Ref: {error.digest}</p>
        )}
      </div>
    </div>
  );
}

