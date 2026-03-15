/**
 * Gold Standard: pump_system/control/mode data contract.
 * Ensures only valid strings (AUTO, FORCE_ON, FORCE_OFF, COUNTDOWN) are processed.
 */
import {
  VALID_CONTROL_MODES,
  isValidControlMode,
  sanitizeControlMode,
} from "@/lib/controlContract";

describe("controlContract", () => {
  describe("VALID_CONTROL_MODES", () => {
    it("contains exactly four modes per v3.0 spec", () => {
      expect(VALID_CONTROL_MODES).toHaveLength(4);
      expect(VALID_CONTROL_MODES).toContain("AUTO");
      expect(VALID_CONTROL_MODES).toContain("FORCE_ON");
      expect(VALID_CONTROL_MODES).toContain("FORCE_OFF");
      expect(VALID_CONTROL_MODES).toContain("COUNTDOWN");
    });

    it("does not contain invalid or legacy modes", () => {
      expect(VALID_CONTROL_MODES).not.toContain("OFF");
      expect(VALID_CONTROL_MODES).not.toContain("MANUAL");
      expect(VALID_CONTROL_MODES).not.toContain("");
    });
  });

  describe("isValidControlMode", () => {
    it("returns true for each valid mode", () => {
      expect(isValidControlMode("AUTO")).toBe(true);
      expect(isValidControlMode("FORCE_ON")).toBe(true);
      expect(isValidControlMode("FORCE_OFF")).toBe(true);
      expect(isValidControlMode("COUNTDOWN")).toBe(true);
    });

    it("returns false for invalid strings", () => {
      expect(isValidControlMode("auto")).toBe(false);
      expect(isValidControlMode("AUTOMATIC")).toBe(false);
      expect(isValidControlMode("")).toBe(false);
      expect(isValidControlMode("OFF")).toBe(false);
      expect(isValidControlMode("MANUAL")).toBe(false);
    });

    it("returns false for non-string values", () => {
      expect(isValidControlMode(null)).toBe(false);
      expect(isValidControlMode(undefined)).toBe(false);
      expect(isValidControlMode(123)).toBe(false);
      expect(isValidControlMode({})).toBe(false);
      expect(isValidControlMode(true)).toBe(false);
    });
  });

  describe("sanitizeControlMode", () => {
    it("returns the value when valid", () => {
      expect(sanitizeControlMode("AUTO")).toBe("AUTO");
      expect(sanitizeControlMode("COUNTDOWN")).toBe("COUNTDOWN");
    });

    it("returns null when invalid", () => {
      expect(sanitizeControlMode("invalid")).toBeNull();
      expect(sanitizeControlMode(null)).toBeNull();
      expect(sanitizeControlMode(undefined)).toBeNull();
    });
  });
});
