# Publication Security Hardening Results

## Dependency remediation

Baseline Cloud Functions audit:

- 1 critical
- 6 high
- 12 moderate
- 2 low

Applied without forced breaking changes:

- `firebase-admin` 13.10.0
- `firebase-functions` 7.3.2
- `ts-jest` 29.4.12
- `@types/node` 22.20.1
- npm's non-forced transitive audit remediation

Final audit:

- 0 critical
- 0 high
- 9 moderate
- 0 low

The remaining findings are in the Firebase Functions / Firebase Admin / Google Cloud dependency chain (two direct packages and seven transitive packages). npm's forced proposal would downgrade `firebase-admin` to 10.3.0, a breaking and unsafe remediation, so it was rejected. These findings are accepted temporarily as upstream/version-constrained and should be monitored.

Validation:

- TypeScript build passed.
- 26 of 26 Jest tests passed across 3 suites.
- Local runtime was Node.js 24 while the package and CI target Node.js 22; npm emitted an engine warning but validation passed.

## History scan

Tool: Gitleaks 8.30.1 installed through WinGet.

- 116 commits scanned
- Approximately 9.18 MB scanned
- 45 generic API-key heuristic matches
- 0 provider-specific rule categories reported
- Reports were written only to the system temporary directory and deleted after classification
- No secret values were printed or committed

All 45 generic matches map to historical hardcoded Firebase authentication identifiers in:

- `database.rules.json`
- historical `docs/FIRMWARE_CONFIG_FROM_DATABASE.md`
- historical `docs/PRODUCTION_READINESS_REVIEW.md`

The current `database.rules.json` did not produce the same matches. The historical values are identifiers rather than passwords or private keys, but they disclose former authorization details and should not be included in a public portfolio history.

## Publication decision

Do not rewrite the private development repository's 116-commit history automatically. It contains substantial private evolution and the current working tree contains unrelated in-progress changes.

Recommended publication architecture:

1. Keep this repository private as the development source of truth.
2. Finish and review the current portfolio candidate.
3. Create a separate sanitized public portfolio repository from a reviewed snapshot, without private Git history, ignored files, original sensitive screenshots, or internal identifiers.
4. Apply the proprietary notice to that repository.
5. Enable available GitHub security features on the public repository.
6. Do not publish until the owner explicitly approves the exact snapshot and metadata.

## External settings

Current private repository state:

- Dependabot alerts disabled
- Secret scanning disabled
- Code scanning disabled
- Private vulnerability reporting unavailable/not enabled
- Visibility remains private

No repository visibility or external security setting was changed during this feature.
