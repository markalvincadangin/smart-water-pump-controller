# Implementation Plan: Original Repository Publication

## Technical Context

- Source: original private Git repository plus current dirty working tree
- Isolation: recoverable bundle, working-state snapshot, and disposable rewrite clone
- Validation: redacted full-history scan, path/identifier audit, Markdown link audit, Android/Functions builds and tests, private GitHub CI
- Constraint: do not rewrite the active workspace or change repository visibility

## Constitution Check

- No firmware behavior or safety contract changes are planned.
- The retained source must preserve current safety specifications and backward-compatible contracts.
- All repository mutation is gated by backups and validation.

## Design

1. Preserve the remote history as a complete bundle and preserve uncommitted files separately.
2. Inventory refs, paths, and scanner findings to produce explicit removal/replacement rules.
3. Rewrite a disposable clone and validate all retained history.
4. Push only to a private verification target and run CI.
5. Replace the original remote only after verification, retaining private visibility.
6. Request separate approval before public visibility.
