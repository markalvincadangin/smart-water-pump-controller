/**
 * Maps last_fault_code from controller status to user-facing message and recovery.
 * See docs/FIRMWARE_DASHBOARD_DESIGN_v2.md §4.2.
 */

export interface FaultDisplay {
  title: string;
  message: string;
  recovery: string;
}

const FAULT_MAP: Record<string, FaultDisplay> = {
  COMM_LOSS: {
    title: "Remote sensor link",
    message: "Remote sensor data is stale or the RS-485 link is unstable. Pump stopped for safety.",
    recovery: "Check tank node power and RS-485 wiring/termination. Wait for the link to become stable, then try again.",
  },
  STALE_LEVEL: {
    title: "Stale sensor data",
    message: "Level data is not fresh. Pump stopped for safety.",
    recovery: "Check the remote sensor node and wait for fresh data, or enable bypass only for supervised maintenance.",
  },
  DRY_RUN: {
    title: "Dry-run",
    message: "Dry-run detected. Check water supply.",
    recovery: "Tap \"Clear Error\" after resolving.",
  },
  OVERFLOW: {
    title: "Max runtime",
    message: "Max runtime exceeded. Check tank sensor.",
    recovery: "Tap \"Clear Error\" after inspecting.",
  },
  E_STOP: {
    title: "Emergency stop",
    message: "Emergency stop is latched. Pump is stopped and locked out.",
    recovery: "Resolve the hazard, then press \"Reset Stop\" (only when safe).",
  },
  LEVEL_SENSOR: {
    title: "Level sensor",
    message: "Level sensor offline.",
    recovery: "Auto-clears on recovery; enable bypass for interim.",
  },
  FLOW_SENSOR: {
    title: "Flow sensor",
    message: "Flow sensor reading abnormal.",
    recovery: "Auto-clears on recovery.",
  },
  SAFE_MODE: {
    title: "Safe mode",
    message: "Controller in safe mode. Power cycle to recover.",
    recovery: "Full power cycle.",
  },
};

/**
 * Returns user-facing title, message, and recovery for a fault code.
 * If code is unknown or empty, returns null. Optional firmware message can override the default message.
 */
export function getFaultDisplay(
  code: string | undefined | null,
  firmwareMessage?: string | null
): FaultDisplay | null {
  if (!code || code === "") return null;
  const entry = FAULT_MAP[code];
  if (!entry) {
    return {
      title: code,
      message: firmwareMessage?.trim() ? firmwareMessage : "An unknown fault was reported by the controller.",
      recovery: "Check wiring, sensors, and controller logs. If this persists, report the fault code.",
    };
  }
  return {
    ...entry,
    message: firmwareMessage?.trim() ? firmwareMessage : entry.message,
  };
}
