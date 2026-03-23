import { getRankedAlerts } from "@/lib/alertRanking";
import type { PumpStatus } from "@/lib/types";

const baseStatus: PumpStatus = {
  water_level_percent: 50,
  is_running: false,
  flow_rate_lpm: 0,
  is_error: false,
  is_overflow_error: false,
  wifi_rssi: -60,
  last_boot_reason: "Power-on",
};

describe("alertRanking", () => {
  it("shows flow bypass warning when bypass_flow_sensor is active", () => {
    const alerts = getRankedAlerts(
      { ...baseStatus, bypass_flow_sensor: true },
      true
    );
    expect(alerts.some((a) => a.id === "maintenance_flow")).toBe(true);
  });
});
