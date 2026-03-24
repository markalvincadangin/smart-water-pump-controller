"use client";

/** §8.3 — Skeleton loading with gradient shimmer sweep (not pulse). */
export default function DashboardSkeleton() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-[1.4fr_0.8fr_1fr] gap-3 sm:gap-4 animate-fade-in">
      {/* Col 1: Tank placeholder */}
      <div className="card p-4 sm:p-6 flex flex-col items-center justify-center gap-2 min-h-[260px] md:min-h-0">
        <div className="h-3 w-20 skeleton-shimmer rounded" />
        <div className="relative w-28 h-44 sm:w-32 sm:h-52 rounded-b-2xl rounded-t-lg border-2 border-surface-4 bg-surface-2 overflow-hidden">
          <div className="absolute inset-0 skeleton-shimmer" aria-hidden />
        </div>
        <div className="h-8 w-16 skeleton-shimmer rounded" />
        <div className="h-3 w-24 skeleton-shimmer rounded" />
      </div>

      {/* Col 2: Stats placeholder */}
      <div className="flex flex-col gap-2 sm:gap-3 min-w-0">
        {/* Mobile strip */}
        <div className="grid grid-cols-3 gap-2 md:hidden">
          {[1, 2, 3].map((i) => (
            <div key={i} className="flex flex-col items-center justify-center min-w-0 p-2.5 rounded-xl bg-surface-2 border border-surface-3">
              <div className="h-2 w-10 skeleton-shimmer rounded" />
              <div className="h-5 w-12 skeleton-shimmer rounded mt-1" />
            </div>
          ))}
        </div>
        {/* Desktop cards */}
        <div className="hidden md:flex flex-col gap-2 sm:gap-3 min-w-0">
          {[1, 2, 3].map((i) => (
            <div key={i} className="card p-3 sm:p-4 flex flex-col gap-2 min-w-0">
              <div className="flex justify-between">
                <div className="h-3 w-20 skeleton-shimmer rounded" />
                <div className="h-7 w-7 skeleton-shimmer rounded-lg" />
              </div>
              <div className="h-7 w-16 skeleton-shimmer rounded" />
              <div className="h-3 w-28 skeleton-shimmer rounded" />
            </div>
          ))}
        </div>
      </div>

      {/* Col 3: Controls placeholder */}
      <div className="card p-4 sm:p-5 space-y-4">
        <div className="h-4 w-24 skeleton-shimmer rounded" />
        <div className="grid grid-cols-2 gap-2">
          <div className="h-12 skeleton-shimmer rounded-xl" />
          <div className="h-12 skeleton-shimmer rounded-xl" />
        </div>
        <div className="flex gap-1">
          {[1, 2, 3, 4, 5].map((i) => (
            <div key={i} className="h-8 flex-1 skeleton-shimmer rounded-lg" />
          ))}
        </div>
        <div className="border-t border-surface-3 pt-4">
          <div className="h-4 w-16 skeleton-shimmer rounded mb-2" />
          <div className="grid grid-cols-2 gap-2">
            <div className="h-14 skeleton-shimmer rounded-xl" />
            <div className="h-14 skeleton-shimmer rounded-xl" />
          </div>
        </div>
      </div>
    </div>
  );
}
