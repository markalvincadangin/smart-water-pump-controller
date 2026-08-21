# Implementation Plan: Publication Security Hardening

## Summary

Upgrade Cloud Functions dependencies in controlled groups, validate each group, run a redacted Gitleaks history scan, classify findings, and update the publication review without changing repository visibility.

## Constitution Gate

Pass: no firmware or RTDB contract change; backend behavior and tests must remain compatible.

## Technical Context

- Node.js 22, TypeScript, Firebase Functions v7
- npm lockfile and Jest test suite
- Gitleaks full-history scan with redaction
- Private GitHub repository; external settings are read-only in this phase

## Sequence

1. Capture baseline audit/build/test evidence.
2. Apply compatible Firebase dependency upgrades first.
3. Re-audit; consider the smallest major upgrade only if critical/high findings remain.
4. Build and test after every accepted group.
5. Install/run Gitleaks without committing the binary or report.
6. Classify findings and update publication blockers.

## Validation

```powershell
cd functions
npm audit
npm run build
npm test -- --runInBand
```

Repository visibility must remain private.
