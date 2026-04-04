# Dashboard Dependency Patching Plan
## Resolving 7 High-Severity npm Audit Findings (PRR Gate 2)

This plan addresses the high-severity vulnerabilities reported by `npm audit` in the dashboard without breaking the build. Execute in a branch and run `npm run validate:build` and `npm run test` after each step.

---

## Summary of High-Severity Issues (as of PRR)

| Severity | Package / Chain | Issue |
|----------|-----------------|--------|
| High | Next.js 10–15.x | DoS via Image Optimizer `remotePatterns`; DoS via RSC deserialization |
| High | flatted | Unbounded recursion DoS in `parse()` |
| High | serialize-javascript (via @ducanh2912/next-pwa → workbox) | RCE via RegExp.flags / Date.prototype.toISOString |
| (Medium/low in chain) | cookie, tmp, yauzl (via @lhci/cli / lighthouse) | Cookie out-of-bounds; tmp symlink; yauzl off-by-one |

---

## Step 1: Upgrade Next.js (Minimal Non-Breaking)

**Goal:** Move to a patched Next.js line without jumping to Next 16 (breaking).

- **Action:** Upgrade to the latest 14.x patch that includes security fixes:
  - `npm install next@14.2.35` (or latest 14.x, e.g. 14.2.x) and `eslint-config-next@14.2.35`.
- **Check:** Release notes for 14.2.35 (or chosen 14.x) for DoS fixes. If the advisory says "fixed in 15.x", consider a one-step upgrade to 15.x and run full regression (build + manual smoke).
- **Verification:** `npm run build` and `npm run test` pass.

---

## Step 2: Patch flatted (Direct Dependency or Resolve)

**Goal:** Bump flatted to ≥3.4.0.

- **Action:** If flatted appears as a direct dependency, `npm install flatted@latest`. If it is transitive, add an override in `package.json`:
  ```json
  "overrides": {
    "glob": "^11.0.0",
    "flatted": ">=3.4.0"
  }
  ```
- **Verification:** `npm run build` and `npm run test`; no new runtime errors.

---

## Step 3: PWA / Workbox (serialize-javascript)

**Goal:** Reduce RCE risk from serialize-javascript used by workbox (inside @ducanh2912/next-pwa).

- **Option A (preferred):** Upgrade `@ducanh2912/next-pwa` to a version that pulls in a patched workbox/serialize-javascript. Check:
  - `npm info @ducanh2912/next-pwa versions`
  - Install latest 10.x: `npm install @ducanh2912/next-pwa@latest`
  - Run `npm audit` again; if serialize-javascript is still high, add override:
    ```json
    "serialize-javascript": ">=7.0.3"
    ```
- **Option B:** If upgrades break PWA build, consider temporarily disabling PWA in production until the maintainer updates deps, or switch to a different PWA plugin that uses patched workbox.
- **Verification:** `npm run build` (PWA assets generate); `npm run test`; manual check of service worker in browser.

---

## Step 4: LHCI / Lighthouse (cookie, tmp, yauzl)

**Goal:** These are dev-only (test/lighthouse). Lowest risk to production runtime.

- **Action 1:** Upgrade LHCI to a version that uses patched lighthouse/puppeteer:
  - `npm install @lhci/cli@latest --save-dev`
  - If `npm audit fix --force` suggests downgrading to @lhci/cli@0.1.0, do **not** use that; prefer staying on a recent 0.14.x and accepting dev-only medium/low, or removing `validate:lighthouse` from the default `validate` script until LHCI updates.
- **Action 2:** Add overrides to force patched transitive deps (optional, only if audit still reports high/critical in this chain):
  ```json
  "overrides": {
    "glob": "^11.0.0",
    "flatted": ">=3.4.0",
    "serialize-javascript": ">=7.0.3",
    "cookie": ">=0.7.0",
    "tmp": ">=0.2.4",
    "yauzl": ">=3.2.1"
  }
  ```
  Run `npm install` and then `npm run build` and `npm run test` to ensure no breakage.

- **Verification:** `npm audit` shows no high/critical, or only low in dev deps; build and tests pass.

---

## Step 5: Lockfile and CI

- After all changes: `npm install` and commit `package.json` and `package-lock.json`.
- In CI, run `npm ci && npm run test && npm run build` and optionally `npm audit --audit-level=high` (fail on high or critical).

---

## Rollback

If any step breaks the build or tests, revert that step’s `package.json` / `package-lock.json` and document the remaining risk (e.g. “Next.js DoS mitigated by 14.2.x; serialize-javascript dev-only in workbox”). Re-run the rollback runbook if a bad deploy was made.

---

## Order of Operations (Quick Reference)

1. Next.js 14.x patch (or 15.x if required by advisory).
2. flatted override or upgrade.
3. @ducanh2912/next-pwa upgrade and/or serialize-javascript override.
4. LHCI upgrade or overrides for cookie/tmp/yauzl; optionally relax `validate` script.
5. Commit lockfile and enable `npm audit --audit-level=high` in CI.
