# Recovery and Publication Artifacts

## Complete History Backup

- Contains all original branches, tags, and objects.
- Immutable during the rewrite.
- Must pass an integrity check.

## Working-State Snapshot

- Contains tracked modifications, staged changes, and untracked non-ignored files.
- Stored outside the active repository and publication candidate.

## Rewrite Rules

- Excluded paths and identifier replacements derived from the complete audit.
- Versioned with the private migration evidence, not the public repository.

## Publication Candidate

- Retains approved engineering history.
- Contains no excluded content across any retained ref.
- Moves from drafted to verified only after all gates pass.
