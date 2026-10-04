export type NotificationPreferenceKey = "dryRunAlert" | "maxRuntimeAlert" | "lowLevelAlert" | "pumpStartedAlert";
export type NotificationSeverity = "critical" | "warning" | "advisory";

export interface NotificationPolicy {
  preferenceKey: NotificationPreferenceKey;
  severity: NotificationSeverity;
  dndCritical: boolean;
  throttleKey: "dryRun" | "maxRuntime" | "lowLevel" | "pumpStarted";
}

/**
 * Backend-owned delivery policy. Android EventRegistry remains a local
 * presentation mapping and must not decide whether an FCM push is sent.
 * Only events with an entry here are eligible for event-driven push delivery.
 */
export const NOTIFICATION_POLICIES: Record<string, NotificationPolicy> = {
  EVT_DRY_RUN_LOCKOUT: { preferenceKey: "dryRunAlert", severity: "critical", dndCritical: true, throttleKey: "dryRun" },
  EVT_MAX_RUNTIME_EXCEEDED: { preferenceKey: "maxRuntimeAlert", severity: "critical", dndCritical: true, throttleKey: "maxRuntime" },
};

export const DERIVED_NOTIFICATION_POLICIES: Record<string, NotificationPolicy> = {
  LOW_TANK: { preferenceKey: "lowLevelAlert", severity: "warning", dndCritical: false, throttleKey: "lowLevel" },
  PUMP_STARTED: { preferenceKey: "pumpStartedAlert", severity: "advisory", dndCritical: false, throttleKey: "pumpStarted" },
};
