"use client";

const ITEMS = [
  { label: "Controller", value: "ESP32" },
  { label: "Sensors", value: "Level · Flow" },
  { label: "Protection", value: "Overload · No-flow shutdown" },
  { label: "Sync", value: "Real-time" },
] as const;

export default function DashboardSystemInfo() {
  return (
    <div className="grid grid-cols-2 md:grid-cols-4 gap-2 sm:gap-3 min-w-0">
      {ITEMS.map(({ label, value }) => (
        <div key={label} className="card p-3 border-surface-3 min-w-0">
          <p className="text-[10px] font-mono text-text-muted uppercase tracking-widest">
            {label}
          </p>
          <p className="text-xs font-mono text-text-secondary mt-1 break-words">{value}</p>
        </div>
      ))}
    </div>
  );
}

