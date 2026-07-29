export const DEVICE_ID = "SF-TEST01";
export const OWNER_UID = "durable-owner";
export const OTHER_UID = "durable-other";

export const ownershipFixtures = {
  unclaimed: {
    devices: { [DEVICE_ID]: { metadata: { deviceAuthUid: `device:${DEVICE_ID}` } } },
  },
  claimed: {
    devices: { [DEVICE_ID]: { ownership: { ownerUid: OWNER_UID, state: "claimed" }, metadata: { claimedByUid: OWNER_UID } } },
    users: { [OWNER_UID]: { devices: { [DEVICE_ID]: true } } },
  },
  consistentLegacyOwner: {
    devices: { [DEVICE_ID]: { metadata: { claimedByUid: OWNER_UID } } },
    users: { [OWNER_UID]: { devices: { [DEVICE_ID]: true } } },
  },
  missingLegacyOwner: {
    devices: { [DEVICE_ID]: { metadata: {} } },
    users: {},
  },
  conflictingLegacyOwner: {
    devices: { [DEVICE_ID]: { metadata: { claimedByUid: OWNER_UID } } },
    users: { [OTHER_UID]: { devices: { [DEVICE_ID]: true } } },
  },
  anonymousUser: { uid: "guest", isAnonymous: true, emailVerified: false },
  durableUser: { uid: OWNER_UID, isAnonymous: false, emailVerified: true, provider: "google.com" },
} as const;
