/**
 * Gold Standard: pump_system/control/mode data contract.
 * Ensures only valid strings (AUTO, MANUAL, COUNTDOWN) are processed.
 */
import {
  VALID_CONTROL_MODES,
  isValidControlMode,
  sanitizeControlMode,
} from "@/lib/controlContract";

describe("controlContract", () => {
  describe("VALID_CONTROL_MODES", () => {
    it("contains exactly three supported policy modes", () => {
      expect(VALID_CONTROL_MODES).toHaveLength(3);
      expect(VALID_CONTROL_MODES).toContain("AUTO");
      expect(VALID_CONTROL_MODES).toContain("COUNTDOWN");
      expect(VALID_CONTROL_MODES).toContain("MANUAL");
    });

    it("does not contain invalid or unsupported modes", () => {
      expect(VALID_CONTROL_MODES).not.toContain("OFF");
      expect(VALID_CONTROL_MODES).not.toContain("FORCE_ON");
      expect(VALID_CONTROL_MODES).not.toContain("FORCE_OFF");
      expect(VALID_CONTROL_MODES).not.toContain("");
    });
  });

  describe("isValidControlMode", () => {
    it("returns true for each valid mode", () => {
      expect(isValidControlMode("AUTO")).toBe(true);
      expect(isValidControlMode("COUNTDOWN")).toBe(true);
      expect(isValidControlMode("MANUAL")).toBe(true);
    });

    it("returns false for invalid strings", () => {
      expect(isValidControlMode("auto")).toBe(false);
      expect(isValidControlMode("AUTOMATIC")).toBe(false);
      expect(isValidControlMode("")).toBe(false);
      expect(isValidControlMode("OFF")).toBe(false);
      expect(isValidControlMode("FORCE_ON")).toBe(false);
      expect(isValidControlMode("FORCE_OFF")).toBe(false);
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
