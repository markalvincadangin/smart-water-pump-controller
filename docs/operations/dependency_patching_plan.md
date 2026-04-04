# Dependency and Security Maintenance Plan

This document defines the ongoing process for dependency risk management across dashboard and functions.

## Scope

- `dashboard/` npm dependencies
- `functions/` npm dependencies
- Build and deploy integrity after upgrades

## Update cadence

- Weekly: audit and patch review
- Monthly: routine dependency refresh
- Immediate: critical/high advisory with known exploit path

## Standard workflow

1. Create maintenance branch.
2. Run audits:

```bash
cd dashboard && npm audit
cd ../functions && npm audit
```

3. Upgrade lowest-risk packages first.
4. Re-run build and tests after each logical batch.
5. Commit lockfiles with clear change notes.

## Validation gates

Dashboard:

```bash
cd dashboard
npm ci
npm run lint
npm run test
npm run build
```

Functions:

```bash
cd functions
npm ci
npm run test
npm run build
```

## Release decision rules

- Block release on unresolved critical vulnerabilities in runtime production dependencies.
- Track medium/low issues in backlog with owner and target date.
- For dev-only advisories, document impact and mitigation rationale.

## Emergency patching

If urgent fix is needed:

1. Patch dependency.
2. Validate with full build/test.
3. Deploy through normal CI path.
4. Monitor runtime logs and health endpoint.
5. Prepare rollback path in advance.

## Documentation requirements

Each dependency patch PR should include:

- Advisory IDs and severity
- Affected package chain
- Why selected version is safe
- Test/build evidence
- Rollback note
