/**
 * Ranked alerts per FIRMWARE_DASHBOARD_DESIGN_v2 §11.3.
 * Order: controller offline, dry-run, overflow, auto-maintenance, maintenance,
 * level sensor error, flow sensor error, sleeping.
 */

import type { PumpStatus } from "./types";
import { getFaultDisplay } from "./faultCodes";

export type AlertSeverity = "red" | "amber" | "blue";

export interface RankedAlert {
  id: string;
  rank: number;
  severity: AlertSeverity;
  title: string;
  description: string;
  /** Optional recovery line; fault-based alerts use getFaultDisplay recovery. */
  recovery?: string;
}

const RANK = {
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

  if (!esp32Online) {
    alerts.push({
      id: "controller_offline",
      rank: RANK.controller_offline,
      severity: "red",
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
      title: fault?.title ?? "Dry-run lockout",
      description: fault?.message ?? "No water flow detected. Pump has been stopped.",
      recovery: fault?.recovery,
    });
  }

  if (st.is_overflow_error) {
    const fault = getFaultDisplay("OVERFLOW", st.last_fault_message);
    alerts.push({
      id: "overflow",
      rank: RANK.overflow,
      severity: "red",
      title: fault?.title ?? "Max runtime exceeded",
      description: fault?.message ?? "Max runtime exceeded. Check tank sensor.",
      recovery: fault?.recovery,
    });
  }

  if (st.auto_bypass_active) {
    alerts.push({
      id: "auto_maintenance",
      rank: RANK.auto_maintenance,
      severity: "amber",
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
      title: "Level sensor offline",
      description: "Ultrasonic sensor is not reporting. Level may be wrong.",
      recovery: "Check sensor wiring. Enable bypass in Device settings for interim operation.",
    });
  }

  if (st.is_flow_sensor_error) {
    alerts.push({
      id: "flow_sensor",
      rank: RANK.flow_sensor,
      severity: "amber",
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
      title: "Sleeping",
      description: "Quiet hours active. Auto mode paused until end time.",
      recovery: "Manual run and low-water override still work.",
    });
  }

  return alerts.sort((a, b) => a.rank - b.rank);
}
