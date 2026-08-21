# Research Decisions

## Decision: Preserve history instead of publishing only a snapshot

**Rationale**: The owner wants the original repository and its authentic engineering history for the portfolio.

**Alternative considered**: Continue using the sanitized snapshot repository. It is safer and faster but has little development history.

## Decision: Rewrite only an isolated clone

**Rationale**: History rewriting changes commit identifiers and can irreversibly discard refs if performed without recovery artifacts.

**Alternative considered**: Delete sensitive files only from the current branch. Historical versions would remain available.

## Decision: Verify privately before replacing the original remote

**Rationale**: This allows scans, builds, CI, and visual review without exposing incomplete sanitation.

**Alternative considered**: Force-push directly to the original private repository. This provides a weaker rollback and comparison path.
