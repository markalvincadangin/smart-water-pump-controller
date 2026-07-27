// lib/controlContract.ts
// v3.0 data contract: pump_system/control/mode — must match firmware exactly.

import type { PumpControl } from "./types";

/** Valid control mode strings accepted by firmware. */
export const VALID_CONTROL_MODES: readonly PumpControl["mode"][] = [
  "AUTO",
  "COUNTDOWN",
  "MANUAL",
] as const;

export type ValidControlMode = (typeof VALID_CONTROL_MODES)[number];

/**
 * Returns true only if the value is a valid control mode string.
 * Used to validate payloads before writing to RTDB.
 */
export function isValidControlMode(value: unknown): value is ValidControlMode {
  return (
    typeof value === "string" &&
    VALID_CONTROL_MODES.includes(value as ValidControlMode)
  );
}

/**
 * Sanitizes a mode string for write: returns the valid mode or null if invalid.
 */
export function sanitizeControlMode(value: unknown): ValidControlMode | null {
  if (!isValidControlMode(value)) return null;
  return value;
}
