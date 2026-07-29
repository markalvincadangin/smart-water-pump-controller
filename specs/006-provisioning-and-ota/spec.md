---
status: current
version: 0.3
last-reviewed: 2026-07-28
source: auto-generated
---

# Feature Specification: Provisioning & Wireless Flashing

**Feature Branch**: `006-provisioning-and-ota`

**Created**: 2026-07-28

**Status**: Draft

**Input**: User description: "make the firmware setup (provision) work on our app, just like how the cctv and printers are set up using their respective apps. Next is the wireless uploading/flashing of the firmware to avoid constantly connecting to the usb. we can make it on the android app and this dev environment"

## Clarifications

### Session 2026-07-28
- Q: What is the desired scope of the firmware-upload functionality for this iteration? → A: Limit it to a local development network; exclude mobile-app-triggered upload for now.
- Q: What physical recovery mechanism should be planned while the hardware is not yet available? → A: Reserve a dedicated non-boot maintenance input. Defer installation until the final wiring is confirmed. A deliberate long press performs a local provisioning reset without transferring ownership.
- Q: What security posture should local development OTA use? → A: Require an OTA password stored outside version control; retain local-network OTA for development.
- Q: What must the physical provisioning reset erase? → A: Clear local Wi-Fi and provisioning state only; retain existing cloud ownership. Ownership transfer is a separate authenticated owner action.
- Q: How must the device establish a stable online identity? → A: Use a per-device credential/bootstrap flow issued by the backend; do not use a new anonymous identity on every boot.
- Q: How should cable-free diagnostics be exposed? → A: Use a local-network live diagnostic stream during development and concise remote diagnostics in production; do not expose a permanent deployed-device console.
- Q: When may the mobile app report provisioning success? → A: Only after it receives the final provisioning-complete signal following Wi-Fi connection and online registration.
- Q: What identity owns a provisioned device? → A: A durable authenticated SmartFlow user account. The ESP32 keeps its separate stable device identity; anonymous sessions may support a short local trial only and may not become a permanent device owner.
- Q: How is ownership established and changed? → A: A backend-authorized, atomic claim verifies local possession during setup, records an audit trail, and supports explicit owner release/transfer. A Wi-Fi or physical reset never transfers ownership.
- Q: How is a lost-owner situation handled in the initial release? → A: Only the current owner may release or transfer a device. Lost-owner recovery is deferred to a future documented support policy; local reset, BLE pairing, and a device ID never override existing ownership.
- Q: When may an email/password account claim or control hardware? → A: Only after email verification. Google sign-in is eligible immediately as a provider-verified durable identity.
- Q: What happens when an owner wants to delete their account? → A: Self-service account deletion is blocked while the account owns a device. The owner must explicitly transfer or release every owned device first.
- Q: Does the initial release support shared device access? → A: No. Each device has exactly one authoritative owner; household sharing and technician roles are deferred to a future feature.
- Q: What confirmation is required for ownership transfer? → A: The current owner confirms the transfer, and the intended recipient signs in and completes a fresh BLE-local possession proof before ownership changes.
- Q: How long may temporary ownership pairing remain active? → A: Exactly five minutes from backend issuance; it cannot be extended and a new owner-authorized request is required after expiry.
- Q: How are frozen legacy-ownership conflicts resolved? → A: Only a trusted operator uses a per-device administrative resolution workflow with a documented evidence reference and immutable audit event; neither the app nor device may resolve a conflict.

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | Firmware upload and recovery cause restarts; the pump must default to OFF during and after them. |
| II. Dry-Run Lockout | Yes | Wi-Fi recovery preserves dry-run configuration and lockout state. |
| III. Overflow Protection | Yes | Wi-Fi recovery preserves overflow configuration and lockout state. |
| IV. TOR Independence | No | Hardware independence remains untouched. |
| V. Sensor Freshness / E-Stop | Yes | Maintenance processing must not block `reset_stop` or `clear_error` evaluation. |
| VI. Backward Compatibility | Yes | Identity and maintenance data additions are additive. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Account-Backed Device Provisioning (Priority: P1)

As a signed-in user, I want to use the mobile app to supply local Wi-Fi credentials and securely claim a new SmartFlow device so it comes online under my account, mirroring standard IoT onboarding flows.

**Why this priority**: Without this, users cannot onboard new hardware onto their Wi-Fi networks without hardcoding credentials in the firmware.

**Independent Test**: Sign in with a durable account, complete setup prompts, and observe the device become online under that account after one successful, auditable claim.

**Acceptance Scenarios**:

1. **Given** an unprovisioned device is powered on and available for setup, **When** a user opens the mobile app without an eligible durable account, **Then** the app presents sign-in, account creation, or email verification before cloud ownership can begin.
2. **Given** a signed-in user submits valid Wi-Fi credentials, **When** the device joins the network and completes online registration, **Then** the app proceeds only after it receives the provisioning-complete signal.
3. **Given** the device is unclaimed and the signed-in user proves local possession during setup, **When** the ownership claim completes, **Then** exactly one durable user is recorded as owner and the user can access the device.
4. **Given** the device joins Wi-Fi but cannot complete online registration or ownership claim, **When** setup reaches its bounded timeout, **Then** the app shows a retryable failure rather than success.
5. **Given** the device has sent the terminal BLE `provisioned` status and has intentionally stopped BLE to join SmartFlow Cloud, **When** the first claim attempt is not yet available, **Then** the app displays a bounded cloud-registration wait and retries the authenticated atomic claim without returning to BLE scanning.
6. **Given** the bounded cloud-registration wait expires while the pairing proof remains valid, **When** the user chooses to retry, **Then** the app retries cloud registration with the in-memory proof; it offers a separate explicit action to restart BLE provisioning.

---

### User Story 2 - Durable Account Access and Ownership Lifecycle (Priority: P1)

As a SmartFlow owner, I want to sign in again on another phone and retain access to my claimed hardware, while preventing another person from taking ownership through a reset or a guessed device ID.

**Why this priority**: A pump controller needs durable, accountable ownership. Temporary device-local or anonymous identities create stranded devices and unsafe ownership ambiguity.

**Independent Test**: The owner signs in on a second phone and sees the same device; an unauthenticated, anonymous, non-owner, or locally reset device cannot change ownership.

**Acceptance Scenarios**:

1. **Given** the single recorded owner signs in with a supported durable method, **When** they open SmartFlow on another phone, **Then** their claimed devices are available without re-provisioning.
2. **Given** a device is already claimed, **When** another user attempts to claim it, **Then** the app receives an ownership-specific result and the existing owner association remains unchanged.
3. **Given** the owner intentionally transfers a device, **When** the owner confirms and the intended recipient completes a fresh BLE-local possession proof, **Then** the backend records the event and only that recipient becomes the next owner; a lost-owner recovery path is not available in this release.
4. **Given** the owner intentionally transfers or releases a device, **When** the owner confirms the request, **Then** the device turns the pump OFF and enters a bounded ownership-pairing mode without clearing Wi-Fi or ownership.
5. **Given** the owner intentionally releases a device, **When** a nearby eligible user completes the temporary release pairing, **Then** the backend records the replacement claim; if the pairing expires or is cancelled, the original owner remains unchanged.
6. **Given** local Wi-Fi enrollment is reset, **When** the device returns to onboarding, **Then** its existing owner association remains unchanged.
7. **Given** an owner attempts to delete their account while it owns a device, **When** deletion is requested, **Then** the app requires transfer or release of every owned device before account deletion can proceed.

---

### User Story 3 - Wireless Firmware Upload in Dev Environment (Priority: P2)

As a developer, I want to wirelessly upload newly compiled firmware from my development environment to a device on my local network.

**Why this priority**: Significantly speeds up the development feedback loop when the device is deployed near the water tank and away from the developer's desk.

**Independent Test**: A local-network upload with the configured credential succeeds without physical interaction.

**Acceptance Scenarios**:

1. **Given** the developer has compiled new firmware, **When** they select the local-network upload target, **Then** the compiled firmware is uploaded directly to the device.
2. **Given** an upload request lacks the configured credential or has the wrong credential, **When** it reaches the device, **Then** the request is rejected and the running firmware remains available.

---

### User Story 4 - Owner Wi-Fi Recovery (Priority: P2)

As the current device owner, I want to request Wi-Fi reprovisioning through the app so that I can recover a moved or changed network without transferring ownership or disabling pump protections.

**Independent Test**: An owner request turns the pump OFF, produces an audit record, clears only local network enrollment, and returns the device to onboarding; non-owner, expired, and replayed requests have no effect.

**Acceptance Scenarios**:

1. **Given** the current owner confirms Wi-Fi recovery, **When** the device accepts the valid short-lived request, **Then** it turns the pump OFF, retains safety latches and ownership, and restarts into onboarding mode.
2. **Given** a non-owner, expired request, malformed request, or replayed request, **When** it reaches the device, **Then** local Wi-Fi, cloud ownership, and pump state are unchanged and a bounded rejection result is recorded.
3. **Given** a safety latch is active, **When** maintenance processing runs, **Then** the existing safety-reset controls remain reachable and the latch is not cleared by recovery.

---

### Edge Cases

- What happens when onboarding is interrupted? (App should gracefully time out and allow retry).
- How does the system handle a failed local upload? (The device resumes normal operation.)
- What happens when an anonymous trial session expires or is deleted? (It cannot retain ownership; the user must sign in with a durable account before claiming.)
- What happens when a device is already claimed? (The result explains that ownership is protected and directs the user to the current owner; no reset or self-service recovery may override it in this release.)
- What happens when a phone loses connectivity after device pairing but before the claim completes? (The device remains unclaimed; the short-lived local-possession proof expires and the user can retry setup.)
- What happens when an owner deletes their account? (Deletion is blocked until every owned device has an explicit release or transfer.)
- What happens when temporary ownership pairing expires or is cancelled? (BLE stops, the pump remains OFF, Wi-Fi and ownership remain unchanged, and the owner must explicitly re-enable normal operation.)

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The mobile app MUST route a signed-in user without a claimed device to setup instead of the dashboard.
- **FR-002**: The mobile app MUST securely transmit Wi-Fi credentials and an explicit setup confirmation to the device through the onboarding channel.
- **FR-003**: The development environment MUST provide a selectable local-network firmware-upload target.
- **FR-003a**: Local development firmware upload MUST reject unauthenticated requests and use a credential that is not committed to version control.
- **FR-004**: The final hardware design MUST reserve a dedicated maintenance input that cannot interfere with normal startup.
- **FR-005**: Once physically installed, only a deliberate long press of the maintenance control MAY reset local Wi-Fi credentials and setup state; shorter presses MUST not reset setup.
- **FR-006**: A physical setup reset MUST preserve the device identity and existing owner association; it MUST not transfer ownership or delete remote data.
- **FR-007**: Device online registration MUST use a stable per-device identity issued by the backend. A new anonymous identity on every restart MUST not authorize device access.
- **FR-008**: Development devices MAY expose a local-network live diagnostic stream. Production devices MUST expose a bounded RTDB diagnostic snapshot containing `freeHeap`, `wifiRSSI`, and `restartReason`, plus no more than 50 WARN/ERROR event records; production devices MUST NOT require or expose a permanently available live console.
- **FR-009**: The mobile app MUST report setup success only after the final provisioning-complete signal following successful Wi-Fi connection and online registration.
- **FR-009a**: After final BLE provisioning, the app MUST show cloud-registration progress and retry only the authenticated atomic claim for a bounded 90-second window. It MUST NOT automatically restart BLE scanning while the ESP32 is expected to be online.
- **FR-009b**: After that bounded window, the app MUST offer an explicit cloud-claim retry while the in-memory pairing proof remains valid and a separate explicit restart-provisioning action. Raw pairing proofs MUST never be persisted or logged.
- **FR-010**: Each device bootstrap credential MUST be unique, kept out of source control and remote application data, and support operator revocation.
- **FR-011**: Before a maintenance request changes network enrollment, the pump MUST be safely off; maintenance MUST not block safety-reset processing or clear a safety latch.
- **FR-012**: Cloud device ownership MUST belong to a durable authenticated user account. An anonymous or guest session MUST NOT create or retain a permanent device-owner association.
- **FR-013**: The app MUST offer durable account-creation/sign-in paths and preserve a signed-in owner's device access across app reinstalls and additional phones. Email/password accounts MUST verify their email before claim or remote control; Google sign-in is eligible immediately.
- **FR-014**: A device claim MUST require an authenticated, non-anonymous user and proof of local possession created during the active setup session; device ID knowledge alone MUST NOT be sufficient.
- **FR-015**: The backend MUST make device ownership changes atomically, record a bounded audit event, and return a distinct result for already-claimed, expired-proof, and unauthorized attempts.
- **FR-016**: Mobile clients MUST NOT directly set, remove, or transfer the authoritative device-owner marker in remote application data.
- **FR-017**: Ownership release or transfer MUST require explicit authorization from the current owner. Lost-owner recovery is out of scope for this release. Resetting local enrollment MUST NOT release or transfer ownership.
- **FR-018**: The device bootstrap credential and the user's authentication credentials MUST remain separate. The firmware MUST NOT receive, store, or transmit a user's password or federated sign-in credential.
- **FR-019**: Self-service account deletion MUST be blocked while the account owns a device. The owner MUST explicitly release or transfer every owned device before the account may be deleted.
- **FR-020**: Each device MUST have exactly one authoritative owner in this release. Household sharing, technician roles, and multiple concurrent owners are out of scope.
- **FR-021**: An ownership transfer MUST require explicit confirmation by the current owner and a fresh local-possession proof from the intended durable-account recipient before ownership changes.
- **FR-022**: Existing claimed devices MUST be migrated to the authoritative ownership model without changing a valid owner. Conflicting legacy ownership records MUST be frozen for operator review; the system MUST NOT guess, release, or transfer ownership. A trusted operator-only, per-device resolution workflow MUST require an evidence reference and write an immutable audit event before it may resolve a frozen conflict; it MUST NOT be callable by the Android app or firmware.
- **FR-023**: An ownership transfer or release MUST start an owner-authorized local pairing mode that expires exactly five minutes after backend issuance and cannot be extended. Before enabling that mode, firmware MUST call `setPump(false)`; expiry or cancellation MUST stop pairing and preserve Wi-Fi, ownership, and safety latches.
- **FR-024**: A destructive test-environment reset MAY delete all RTDB data and Firebase Auth users only after it writes an ignored local RTDB backup and requires explicit acknowledgement that this workflow cannot restore Firebase Auth users. Operators MUST record any required test-account identifiers before applying the reset. It MUST then reseed the explicitly requested non-secret active device-registry record so a configured test ESP32 can bootstrap; it MUST NOT restore ownership, pairing, telemetry, user data, Functions, OAuth settings, Secret Manager secrets, or ignored local firmware configuration.

---

## Firmware Behavior *(if applicable)*

### State Machine Impact
- The development upload service starts only after network connection.
- The physical reset path is deferred until the maintenance control is installed; no unwired input is treated as a reset request.

### Safety Invariants
- [x] All fault paths leave the pump safely OFF.
- [x] Relay state changes use the single approved control boundary.
- [x] Timed behavior is safe across device uptime rollover.

---

## Dashboard UX *(if applicable)*

### User-Facing Changes
- **Navigation**: Route an unclaimed user to setup before normal dashboard access.
- **Account gate**: Present sign-in or account creation before a user can claim hardware for cloud control.
- **Ownership feedback**: Explain protected-existing-owner, expired setup proof, and successful claim outcomes without exposing another user's identity.
- **Access scope**: Show owner-only management; do not present household or technician invitations in this release.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Unprovisioned devices can be successfully claimed and connected to Wi-Fi via the Android app in under 2 minutes.
- **SC-002**: A local-network firmware upload completes successfully without physical interaction.
- **SC-003**: An OTA upload without the configured password is rejected, while an upload with the configured password completes successfully.
- **SC-004**: After a physical setup reset, the device becomes available for onboarding and the original owner remains associated with it remotely.
- **SC-005**: A provisioned device can restart and continue authorized online updates without changing its device identity or authorization principal.
- **SC-006**: During development, a device on the same local network exposes live diagnostics without USB; a production-configured device exposes the required diagnostic snapshot and no more than 50 retained WARN/ERROR events through RTDB without an open live console.
- **SC-007**: A device that connects to Wi-Fi but fails online registration is reported as a setup failure, not as a successful setup.
- **SC-007a**: A device that has left BLE after terminal provisioning remains on a cloud-registration screen for up to 90 seconds; the app neither falsely reports success nor falls back to indefinite BLE scanning during that interval.
- **SC-008**: A revoked device bootstrap secret is rejected, and a maintenance request received while a safety latch is active leaves that latch intact and control reset paths reachable.
- **SC-009**: A user who signs in with the same durable account on a second phone can access their claimed device without repeating Wi-Fi provisioning.
- **SC-010**: In end-to-end claim tests, 100% of successful claims create exactly one owner mapping and one corresponding audit event; no anonymous session can become the owner.
- **SC-011**: A claim attempted with only a device ID, an expired local-possession proof, or by a non-owner of an already claimed device changes no ownership data.
- **SC-012**: An owner who attempts account deletion while owning a device receives a clear blocked result and no ownership data changes.
- **SC-013**: Ownership validation rejects any attempt to add a second owner or shared-access member to a device in this release.
- **SC-014**: A remote transfer attempt without a fresh recipient local-possession proof leaves ownership unchanged.
- **SC-015**: A migration test preserves every consistent legacy owner mapping and flags every conflicting legacy mapping without changing its owner.
- **SC-016**: An expired or cancelled ownership-pairing session stops BLE availability, preserves the original owner, and leaves the pump OFF.
- **SC-017**: An operator resolution test can resolve only a specified frozen device with a documented evidence reference, writes one immutable audit event, and cannot be invoked by an app user or device principal.

## Assumptions

- We assume the device has sufficient update-storage capacity for safe local-network firmware replacement.
- Google sign-in and verified email/password are the initial durable account methods. Other providers can be added without changing device ownership semantics.
- A guest experience, if retained, is restricted to local setup discovery and must be upgraded to a durable account before cloud claim or remote pump control.
- The initial release supports one authoritative owner per device. Account-loss recovery requires a future, separately specified support policy.
- Ownership pairing requires the device to be online so it can receive the owner-authorized request; it is not a substitute for offline physical recovery.
