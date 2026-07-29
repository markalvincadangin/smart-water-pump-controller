import * as fs from "fs";
import * as path from "path";

describe("RTDB ownership boundary", () => {
  const rules = JSON.parse(fs.readFileSync(path.resolve(__dirname, "../../../database.rules.json"), "utf8"));
  const device = rules.rules.devices.$deviceId;

  it("denies direct ownership and ownership-audit mutation", () => {
    expect(device.ownership[".write"]).toBe(false);
    expect(device.ownershipAudit[".write"]).toBe(false);
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
