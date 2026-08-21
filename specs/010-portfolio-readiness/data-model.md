# Portfolio Readiness Data Model

No runtime data model changes are introduced.

## Portfolio Claim

| Field | Validation |
|-------|------------|
| Statement | Specific and free of unsupported superlatives |
| Category | Problem, ownership, architecture, feature, safety, deployment, or outcome |
| Evidence | Current code, canonical spec, executable test, approved image, or qualified owner observation |
| Confidence | Verified, qualified, or excluded |
| Disposition | Keep, rewrite, move to detailed docs, or remove |

## Visual Asset

| Field | Validation |
|-------|------------|
| Source and public copy | Preserve original; use descriptive public filename |
| Purpose, caption, alt text | Supports a portfolio section and is accessible |
| Privacy status | Pending, approved, redacted, or rejected |
| Disclosure findings | Accounts, IDs, locations, networks, credentials, notifications, or labels must be absent/remediated |

## Publication Finding

| Field | Validation |
|-------|------------|
| Scope | Tree, history indicator, image, metadata, link, or license |
| Severity | Blocker, warning, or informational |
| Description | State risk without reproducing sensitive values |
| Remediation and status | Required for blockers/warnings; publication requires zero open blockers |

## Repository Metadata Proposal

Description and topics must be accurate; homepage must be current or blank; license detection must match the committed license; CI must validate current components; visibility remains private until approval.

## State Transition

```text
Draft → evidence/privacy review → internally approved
      → zero blockers → owner publication decision
      → explicit authorization → public portfolio repository
```

Any new blocker returns the repository to internal review.
