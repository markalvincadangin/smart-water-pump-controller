"use client";

import clsx from "clsx";
import type { PumpStatus } from "@/lib/types";
import CollapsibleSection from "@/components/CollapsibleSection";

interface DashboardSystemInfoProps {
  status?: PumpStatus | null;
}

interface InfoItem {
  label: string;
  value: string;
  color?: string;
}

function formatBytes(bytes: number): string {
  if (bytes >= 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  if (bytes >= 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${bytes} B`;
}

function wifiQuality(rssi: number): { label: string; color: string } {
  if (rssi >= -50) return { label: "Excellent", color: "text-accent-green" };
  if (rssi >= -60) return { label: "Good", color: "text-accent-green" };
  if (rssi >= -70) return { label: "Fair", color: "text-accent-amber" };
  return { label: "Weak", color: "text-accent-red" };
}

function InfoGroup({ title, items }: { title: string; items: InfoItem[] }) {
  if (items.length === 0) return null;
  return (
    <div>
      <p className="text-[9px] font-mono text-text-muted uppercase tracking-widest mb-1.5">{title}</p>
      <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-1.5 sm:gap-2">
        {items.map(({ label, value, color }) => (
          <div key={label} className="px-2.5 py-2 rounded-lg bg-surface-2 border border-surface-3 min-w-0">
            <p className="text-[9px] font-mono text-text-muted uppercase tracking-wider leading-tight">{label}</p>
            <p className={clsx("text-xs font-mono mt-0.5 break-words leading-snug", color ?? "text-text-secondary")}>{value}</p>
          </div>
        ))}
      </div>
    </div>
  );
}

export default function DashboardSystemInfo({ status }: DashboardSystemInfoProps) {
  const overviewItems: InfoItem[] = [
    { label: "Controller", value: "ESP32" },
    { label: "Sensors", value: "Ultrasonic · Flow" },
    { label: "Protection", value: "Dry-run · Overflow · Sensor" },
    { label: "Sync", value: "Real-time (Firebase)" },
  ];

  const telemetryItems: InfoItem[] = [];
  if (status) {
    if (status.total_pump_cycles != null) {
      telemetryItems.push({ label: "Pump cycles", value: String(status.total_pump_cycles) });
    }
    if (status.total_pump_run_min != null) {
      const hrs = Math.floor(status.total_pump_run_min / 60);
      const min = status.total_pump_run_min % 60;
      telemetryItems.push({ label: "Total runtime", value: hrs > 0 ? `${hrs}h ${min}m` : `${min}m` });
    }
    if (status.level_sensor_health_pct != null) {
      const pct = status.level_sensor_health_pct;
      telemetryItems.push({
        label: "Sensor health",
        value: `${pct}%`,
        color: pct >= 90 ? "text-accent-green" : pct >= 60 ? "text-accent-amber" : "text-accent-red",
      });
    }
    if (status.level_last_valid_age_sec != null) {
      telemetryItems.push({
        label: "Level data age",
        value: status.level_last_valid_age_sec < 60
          ? `${status.level_last_valid_age_sec}s`
          : `${Math.floor(status.level_last_valid_age_sec / 60)}m ${status.level_last_valid_age_sec % 60}s`,
      });
    }
  }

  const connectivityItems: InfoItem[] = [];
  if (status) {
    if (status.wifi_rssi != null && status.wifi_rssi !== 0) {
      const q = wifiQuality(status.wifi_rssi);
      connectivityItems.push({ label: "WiFi", value: `${status.wifi_rssi} dBm (${q.label})`, color: q.color });
    }
    if (status.uptime_minutes != null) {
      const d = Math.floor(status.uptime_minutes / 1440);
      const h = Math.floor((status.uptime_minutes % 1440) / 60);
      const m = status.uptime_minutes % 60;
      connectivityItems.push({ label: "Uptime", value: d > 0 ? `${d}d ${h}h ${m}m` : h > 0 ? `${h}h ${m}m` : `${m}m` });
    }
    if (status.last_boot_reason) {
      connectivityItems.push({ label: "Boot reason", value: status.last_boot_reason });
    }
    if (status.firebase_consecutive_failures != null && status.firebase_consecutive_failures > 0) {
      connectivityItems.push({
        label: "Firebase errors",
        value: `${status.firebase_consecutive_failures} consecutive`,
        color: "text-accent-amber",
      });
    }
    if (status.firebase_last_error) {
      connectivityItems.push({ label: "Last FB error", value: status.firebase_last_error, color: "text-accent-amber" });
    }
  }

  const sensorItems: InfoItem[] = [];
  if (status) {
    if (status.ultrasonic_last_good_cm != null) {
      sensorItems.push({ label: "Last distance", value: `${status.ultrasonic_last_good_cm.toFixed(1)} cm` });
    }
    if (status.ultrasonic_cycles_ok != null) {
      sensorItems.push({ label: "Good readings", value: String(status.ultrasonic_cycles_ok) });
    }
    if (status.ultrasonic_cycles_timeout != null) {
      sensorItems.push({
        label: "Failed readings",
        value: String(status.ultrasonic_cycles_timeout),
        color: status.ultrasonic_cycles_timeout > 0 ? "text-accent-amber" : undefined,
      });
    }
    if (status.flow_discard_max_sane != null && status.flow_discard_max_sane > 0) {
      sensorItems.push({ label: "Flow discards", value: String(status.flow_discard_max_sane), color: "text-accent-amber" });
    }
    if (status.flow_stuck_high_events != null && status.flow_stuck_high_events > 0) {
      sensorItems.push({ label: "Flow stuck events", value: String(status.flow_stuck_high_events), color: "text-accent-amber" });
    }
    if (status.flow_volume_added_l != null && status.flow_volume_added_l > 0) {
      sensorItems.push({ label: "Volume added", value: `${status.flow_volume_added_l.toFixed(1)} L` });
    }
  }

  const memoryItems: InfoItem[] = [];
  if (status) {
    if (status.free_heap_bytes != null) {
      memoryItems.push({ label: "Free heap", value: formatBytes(status.free_heap_bytes) });
    }
    if (status.min_free_heap_observed_bytes != null) {
      memoryItems.push({
        label: "Min observed",
        value: formatBytes(status.min_free_heap_observed_bytes),
        color: status.min_free_heap_observed_bytes < 20000 ? "text-accent-red" : undefined,
      });
    }
    if (status.min_free_heap_bytes != null) {
      memoryItems.push({ label: "Min free (ESP)", value: formatBytes(status.min_free_heap_bytes) });
    }
    if (status.max_alloc_heap_bytes != null) {
      memoryItems.push({ label: "Max alloc block", value: formatBytes(status.max_alloc_heap_bytes) });
    }
  }

  return (
    <CollapsibleSection title="System Info" subtitle="Controller diagnostics and telemetry">
      <div className="space-y-3">
        <InfoGroup title="Overview" items={overviewItems} />
        {telemetryItems.length > 0 && <InfoGroup title="Pump Telemetry" items={telemetryItems} />}
        {connectivityItems.length > 0 && <InfoGroup title="Connectivity" items={connectivityItems} />}
        {sensorItems.length > 0 && <InfoGroup title="Sensors" items={sensorItems} />}
        {memoryItems.length > 0 && <InfoGroup title="Memory" items={memoryItems} />}
      </div>
    </CollapsibleSection>
  );
}
