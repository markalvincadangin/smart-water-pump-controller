/**
 * Ranked alerts per FIRMWARE_DASHBOARD_DESIGN_v2 §11.3.
 * Order: controller offline, dry-run, overflow, auto-maintenance, maintenance,
 * level sensor error, flow sensor error, sleeping.
 */

import type { PumpStatus } from "./types";
import { getFaultDisplay } from "./faultCodes";

export type AlertSeverity = "red" | "amber" | "blue";
export type AlertTier = "critical" | "warning" | "info";

export interface RankedAlert {
  id: string;
  rank: number;
  severity: AlertSeverity;
  tier: AlertTier;
  title: string;
  description: string;
  /** Optional recovery line; fault-based alerts use getFaultDisplay recovery. */
  recovery?: string;
}

const RANK = {
  force_on_override: 0,    // v5.0: highest priority — absolute safety bypass
  controller_offline: 1,
  dry_run: 2,
  overflow: 3,
  auto_maintenance: 4,
  maintenance: 5,
  level_sensor: 6,
  flow_sensor: 7,
  sleeping: 8,
} as const;

/**
 * Returns active alerts in display order (highest priority first).
 * Call when connected; when disconnected, show connection banner separately.
 */
export function getRankedAlerts(
  status: PumpStatus | null | undefined,
  esp32Online: boolean
): RankedAlert[] {
  const alerts: RankedAlert[] = [];

  // v4.0: FORCE_ON override — highest priority alert
  if (status?.run_mode === "FORCE_ON") {
    alerts.push({
      id: "force_on_override",
      rank: RANK.force_on_override,
      severity: "red",
      tier: "critical",
      title: "⚡ FORCE ON — All Safety Bypassed",
      description: "Pump is running in absolute override. All protections are disabled.",
      recovery: "Exit to AUTO or use FORCE OFF via the Mode selector.",
    });
  }

  if (!esp32Online) {
    alerts.push({
      id: "controller_offline",
      rank: RANK.controller_offline,
      severity: "red",
      tier: "critical",
      title: "Controller offline",
      description: "No status update from the pump controller. Check power and network.",
      recovery: "Wait for the controller to come back online or check wiring and WiFi.",
    });
  }

  const st = status;
  if (!st) return alerts;

  // Dry-run: is_error but not overflow (overflow has its own alert)
  if (st.is_error && !st.is_overflow_error) {
    const fault = getFaultDisplay(st.last_fault_code, st.last_fault_message);
    alerts.push({
      id: "dry_run",
      rank: RANK.dry_run,
      severity: "red",
      tier: "critical",
      title: fault?.title ?? "Dry-run lockout",
      description: fault?.message ?? "No water flow detected. Pump has been stopped to protect the motor.",
      recovery:
        "1) Check water source and inlet pipe\n2) Restore normal flow or fix any blockage/leaks\n3) Tap Clear Error (or Clear Error & Restart in MANUAL) only when water is available",
    });
  }

  if (st.is_overflow_error) {
    const fault = getFaultDisplay("OVERFLOW", st.last_fault_message);
    alerts.push({
      id: "overflow",
      rank: RANK.overflow,
      severity: "red",
      tier: "critical",
      title: fault?.title ?? "Max runtime exceeded",
      description: fault?.message ?? "Max runtime exceeded. Pump was stopped to prevent possible overflow or overheating.",
      recovery:
        "1) Confirm tank level and overflow/relief path\n2) Inspect level sensor and max runtime settings\n3) Tap Clear Error only after the underlying issue is resolved",
    });
  }

  if (st.auto_bypass_active) {
    alerts.push({
      id: "auto_maintenance",
      rank: RANK.auto_maintenance,
      severity: "amber",
      tier: "warning",
      title: "Auto-Maintenance active",
      description: "Level sensor offline. System switched to flow-only mode automatically.",
      recovery: "Supervise pump. Bypass clears when the sensor recovers or you turn it off in settings.",
    });
  }

  if (st.bypass_level_sensor && !st.auto_bypass_active) {
    alerts.push({
      id: "maintenance",
      rank: RANK.maintenance,
      severity: "amber",
      tier: "warning",
      title: "Maintenance active",
      description: "Level sensor bypassed. Flow guard only. Supervise pump.",
      recovery: "Turn off bypass in Device settings when done.",
    });
  }

  if (st.is_level_sensor_error) {
    alerts.push({
      id: "level_sensor",
      rank: RANK.level_sensor,
      severity: "amber",
      tier: "warning",
      title: "Level sensor offline",
      description: "Ultrasonic sensor is not reporting. Level may be wrong.",
      recovery: "Check sensor wiring. Enable bypass in Device settings for interim operation.",
    });
  }

  if (st.is_flow_sensor_error) {
    alerts.push({
      id: "flow_sensor",
      rank: RANK.flow_sensor,
      severity: "blue",
      tier: "info",
      title: "Flow sensor abnormal",
      description: "Flow sensor reading is abnormal. Check wiring or replace sensor.",
      recovery: "Auto-clears when the sensor recovers.",
    });
  }

  if (st.is_sleeping) {
    alerts.push({
      id: "sleeping",
      rank: RANK.sleeping,
      severity: "blue",
      tier: "info",
      title: "Sleeping",
      description: "Quiet hours active. Auto mode paused until end time.",
      recovery: "Manual run and low-water override still work.",
    });
  }

  return alerts.sort((a, b) => a.rank - b.rank);
}
