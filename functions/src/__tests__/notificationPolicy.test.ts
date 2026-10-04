import { DERIVED_NOTIFICATION_POLICIES, NOTIFICATION_POLICIES } from "../notificationPolicy";

describe("notification policy registry", () => {
  it("defines canonical safety policies", () => {
    expect(NOTIFICATION_POLICIES.EVT_DRY_RUN_LOCKOUT).toMatchObject({
      preferenceKey: "dryRunAlert",
      severity: "critical",
      dndCritical: true,
      throttleKey: "dryRun",
    });
    expect(NOTIFICATION_POLICIES.EVT_MAX_RUNTIME_EXCEEDED).toMatchObject({
      preferenceKey: "maxRuntimeAlert",
      severity: "critical",
      dndCritical: true,
      throttleKey: "maxRuntime",
    });
  });

  it("defines derived alert policies", () => {
    expect(DERIVED_NOTIFICATION_POLICIES.LOW_TANK.preferenceKey).toBe("lowLevelAlert");
    expect(DERIVED_NOTIFICATION_POLICIES.PUMP_STARTED.preferenceKey).toBe("pumpStartedAlert");
  });
});
