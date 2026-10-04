const DEFAULT_DEVICE_DISPLAY_NAME = "SmartFlow Pump";
const MAX_DEVICE_DISPLAY_NAME_LENGTH = 64;

export function resolveDeviceDisplayName(metadata: unknown, deviceId?: string): string {
  const raw = metadata && typeof metadata === "object"
    ? (metadata as Record<string, unknown>).displayName
    : undefined;

  if (typeof raw !== "string") return DEFAULT_DEVICE_DISPLAY_NAME;

  const normalized = raw
    .replace(/[\u0000-\u001F\u007F]/g, "")
    .replace(/\s+/g, " ")
    .trim()
    .slice(0, MAX_DEVICE_DISPLAY_NAME_LENGTH);

  if (!normalized || (deviceId && normalized.toLowerCase().includes(deviceId.toLowerCase()))) {
    return DEFAULT_DEVICE_DISPLAY_NAME;
  }

  return normalized;
}

export { DEFAULT_DEVICE_DISPLAY_NAME, MAX_DEVICE_DISPLAY_NAME_LENGTH };
