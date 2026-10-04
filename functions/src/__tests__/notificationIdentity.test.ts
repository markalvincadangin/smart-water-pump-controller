import { DEFAULT_DEVICE_DISPLAY_NAME, MAX_DEVICE_DISPLAY_NAME_LENGTH, resolveDeviceDisplayName } from "../notificationIdentity";

describe("resolveDeviceDisplayName", () => {
  it("uses a valid custom display name", () => {
    expect(resolveDeviceDisplayName({ displayName: "Main Tank Pump" }, "SF-A1B2C3")).toBe("Main Tank Pump");
  });

  it("falls back when metadata is missing or the name is blank", () => {
    expect(resolveDeviceDisplayName(null, "SF-A1B2C3")).toBe(DEFAULT_DEVICE_DISPLAY_NAME);
    expect(resolveDeviceDisplayName({}, "SF-A1B2C3")).toBe(DEFAULT_DEVICE_DISPLAY_NAME);
    expect(resolveDeviceDisplayName({ displayName: "   " }, "SF-A1B2C3")).toBe(DEFAULT_DEVICE_DISPLAY_NAME);
  });

  it("does not expose the internal device ID as the display name", () => {
    expect(resolveDeviceDisplayName({ displayName: "SF-A1B2C3" }, "SF-A1B2C3")).toBe(DEFAULT_DEVICE_DISPLAY_NAME);
  });

  it("removes control characters, normalizes whitespace, and bounds length", () => {
    const value = resolveDeviceDisplayName(
      { displayName: "  Main\n\tTank\r Pump " + "x".repeat(100) },
      "SF-A1B2C3"
    );
    expect(value).toBe("Main Tank Pump " + "x".repeat(MAX_DEVICE_DISPLAY_NAME_LENGTH - "Main Tank Pump ".length));
    expect(value.length).toBe(MAX_DEVICE_DISPLAY_NAME_LENGTH);
  });

  it("rejects non-string metadata values", () => {
    expect(resolveDeviceDisplayName({ displayName: 123 }, "SF-A1B2C3")).toBe(DEFAULT_DEVICE_DISPLAY_NAME);
  });
});
