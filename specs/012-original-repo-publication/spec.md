# Feature Specification: Original Repository Publication

**Created**: 2026-08-21
**Status**: In progress
**Input**: Prepare the original SmartFlow repository for portfolio use today while keeping it private until explicit approval.

## User Scenarios & Testing

### User Story 1 - Preserve all private work (Priority: P1)

As the owner, I can recover the complete repository and current working state before any history modification.

**Independent Test**: A private backup and a separate working-state snapshot can be restored without using the rewritten repository.

### User Story 2 - Retain meaningful history safely (Priority: P1)

As the owner, I can use the original repository for my portfolio with useful engineering history but without private deployment material.

**Independent Test**: Every retained ref passes a redacted full-history scan and excluded paths or identifiers cannot be recovered from it.

### User Story 3 - Approve publication (Priority: P2)

As the owner, I can review the final private repository, successful CI, metadata, and residual risks before deciding whether to make it public.

**Independent Test**: Visibility remains private and publication requires a separate explicit instruction.

### Edge Cases

- Uncommitted work is not represented by the remote repository.
- A historical match is an identifier rather than an active credential.
- Rewriting removes a meaningful commit or breaks a tag.
- A build depends on an intentionally excluded local credential.

## Requirements

- **FR-001**: The complete original repository and current working state MUST be recoverable before rewriting.
- **FR-002**: Rewriting MUST occur in an isolated clone, never the active workspace.
- **FR-003**: All branches and tags MUST be inventoried and scanned with values redacted.
- **FR-004**: Private media, deployment records, credentials, local configuration, retired web code, and internal tooling MUST be excluded from the publication history.
- **FR-005**: Meaningful Android, firmware, Functions, hardware, and documentation history SHOULD be retained when safe.
- **FR-006**: The rewritten candidate MUST pass link, secret, stale-component, build, and test checks.
- **FR-007**: The original GitHub repository MUST remain private until explicit approval.
- **FR-008**: Replacing the original remote history MUST occur only after a private verification candidate passes all gates.

## Success Criteria

- **SC-001**: Two independent recovery artifacts exist before history changes.
- **SC-002**: Zero unreviewed secret findings remain across every retained ref.
- **SC-003**: 100% of required Android and Functions validation jobs pass.
- **SC-004**: Zero excluded paths or deployment identifiers exist in retained history.
- **SC-005**: Repository visibility remains private throughout preparation.

## Assumptions

- The owner accepts changed commit hashes and replacement of existing local clones after publication cleanup.
- Firmware flashing is excluded because physical hardware is required.
- The already sanitized portfolio repository remains a private comparison baseline.
