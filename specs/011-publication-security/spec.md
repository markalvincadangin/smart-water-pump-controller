---
status: draft
version: 0.1
last-reviewed: 2026-08-21
source: hand-authored
---

# Feature Specification: Publication Security Hardening

**Feature Branch**: `[011-publication-security]`
**Created**: 2026-08-21
**Status**: Draft
**Input**: Remediate dependency and repository-security blockers before SmartFlow is considered for public portfolio visibility.

## Constitution Check

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| Fail Toward Pump OFF | Yes | Backend changes must not alter firmware control behavior or authorize unsafe commands. |
| Dry-Run / Overflow / TOR / Freshness | N/A | No firmware behavior changes. |
| Backward Compatibility | Yes | Cloud dependency upgrades must preserve callable, trigger, and RTDB behavior. |

## User Scenarios & Testing

### User Story 1 - Resolve Dependency Risk (Priority: P1)

As the owner, I can publish a backend whose known dependency advisories have been reviewed and reduced without breaking authentication, ownership, notifications, event retention, or database rules.

**Independent Test**: Cloud Functions compile and all tests pass; the audit has no unreviewed critical or high finding.

### User Story 2 - Audit Repository History (Priority: P1)

As the owner, I receive a redacted history scan that identifies secret exposure without printing secret values, plus rotation guidance for any credible finding.

**Independent Test**: A recognized history-aware scanner completes with redaction and findings are classified without committing its report.

### User Story 3 - Enable Safe Publication Controls (Priority: P2)

As the owner, I know which GitHub security settings and metadata remain before publication, without changing repository visibility automatically.

**Independent Test**: A final report distinguishes completed local hardening from external settings requiring owner approval.

### Edge Cases

- A dependency fix requires a breaking major upgrade.
- An advisory is transitive and cannot be removed without upstream changes.
- A scanner identifies a historical credential that may still be live.
- Existing unrelated working-tree changes overlap dependency files.

## Requirements

- **FR-001**: Upgrades MUST be incremental and lockfile-controlled.
- **FR-002**: Builds and all existing Functions tests MUST pass after each accepted upgrade group.
- **FR-003**: Forced audit fixes MUST NOT be applied without reviewing breaking changes.
- **FR-004**: Secret scans MUST redact values and reports MUST remain ignored/local.
- **FR-005**: Any credible exposed credential MUST be treated as requiring rotation.
- **FR-006**: Repository visibility MUST remain private.
- **FR-007**: Existing unrelated work MUST be preserved.
- **FR-008**: Final findings MUST distinguish fixed, accepted, upstream-blocked, and owner-action items.

## Success Criteria

- **SC-001**: Zero unreviewed critical or high dependency findings remain.
- **SC-002**: Functions build and 100% of existing tests pass.
- **SC-003**: A redacted full-history secret scan completes.
- **SC-004**: No sensitive scan value or report is committed.
- **SC-005**: Visibility remains private until explicit approval.

## Assumptions

- Dependency majors are accepted only when necessary and validated.
- GitHub security settings may be documented without mutating them.
- Credential rotation may require owner access to external services and can remain an explicit blocker.
