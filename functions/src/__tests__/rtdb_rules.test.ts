import * as fs from "fs";
import * as path from "path";

describe("RTDB ownership boundary", () => {
  const rules = JSON.parse(fs.readFileSync(path.resolve(__dirname, "../../../database.rules.json"), "utf8"));
  const device = rules.rules.devices.$deviceId;

  it("denies direct ownership and ownership-audit mutation", () => {
    expect(device.ownership[".write"]).toBe(false);
    expect(device.ownershipAudit[".write"]).toBe(false);
  });

  it("enforces safety-sensitive settings validation", () => {
    const settings = device.settings;
    expect(settings[".validate"]).toContain("pump_start_level_pct");
    expect(settings[".validate"]).toContain("pump_stop_level_pct");
    expect(settings[".validate"]).toContain("dry_run_threshold_lpm");
    expect(settings[".validate"]).toContain("max_pump_runtime_min");
    expect(settings.pump_start_level_pct[".validate"]).toContain(">= 0");
    expect(settings.pump_start_level_pct[".validate"]).toContain("<= 100");
    expect(settings.pump_stop_level_pct[".validate"]).toContain("newData.val() > newData.parent().child('pump_start_level_pct').val()");
    expect(settings.dry_run_threshold_lpm[".validate"]).toContain(">= 0.1");
    expect(settings.dry_run_threshold_lpm[".validate"]).toContain("<= 10.0");
    expect(settings.max_pump_runtime_min[".validate"]).toContain(">= 30");
    expect(settings.max_pump_runtime_min[".validate"]).toContain("<= 120");
  });

  it("enforces desired mode and countdown bounds", () => {
    const desired = device.shadow.desired[".validate"];
    expect(desired).toContain("MANUAL");
    expect(desired).toContain("COUNTDOWN");
    expect(desired).toContain("AUTO");
    expect(desired).toContain("countdown_duration_min");
    expect(desired).toContain(">= 1");
    expect(desired).toContain("<= 120");
  });

  it("allows only the matching device principal to publish telemetry", () => {
    expect(device.telemetry[".write"]).toContain("auth.token.role === 'device'");
    expect(device.telemetry[".write"]).toContain("auth.token.deviceId === $deviceId");
  });

  it("prevents raw pairing proofs while allowing a device verifier", () => {
    expect(device.pairing.current[".write"]).toContain("!newData.child('rawProof').exists()");
    expect(device.pairing.current[".write"]).toContain("auth.token.deviceId === $deviceId");
  });
});
