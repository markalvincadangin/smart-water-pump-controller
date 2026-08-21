# Portfolio Readiness Validation Guide

## Prerequisites

JDK 21, configured Android SDK, Node.js 22/npm, and authenticated GitHub CLI. Production Firebase credentials and pump hardware are not routine prerequisites.

## Working Tree

```powershell
git status --short
git diff -- README.md .github CONTRIBUTING.md DEPLOYMENT_SAFETY.md SECURITY.md docs .specify specs/010-portfolio-readiness
```

Preserve all unrelated pre-existing changes.

## Retired Web References

```powershell
rg -n -i 'Next\.js|Vercel|dashboard/' README.md CONTRIBUTING.md DEPLOYMENT_SAFETY.md SECURITY.md .github .specify/memory docs/specs docs/operations firmware/README.md
```

No current-state dependency may remain. Generic Android-dashboard wording is allowed; archives are excluded.

## README Links

Parse every relative Markdown link and confirm its target exists; manually verify anchors and external links. Expected: zero broken links.

## Android

```powershell
./gradlew.bat test
./gradlew.bat assembleDebug
```

Expected: tests and debug build pass with JDK 21 and an Android SDK. Report environmental blockers precisely.

## Cloud Functions

```powershell
Set-Location functions
npm ci
npm run build
npm test
```

Expected: install, compilation, and tests pass on Node.js 22.

## Media

Inspect original and processed images for readability and accounts, notifications, device IDs, Wi-Fi names, tokens, addresses, coordinates, or identifying labels. Confirm alt text and captions.

## Publication Safety

Scan without printing discovered secret values. Review GitHub security settings where accessible. Revoke or rotate an exposed live secret before considering history cleanup. Expected: zero open blockers before recommending public visibility.

## Repository Metadata

```powershell
gh repo view --json nameWithOwner,url,description,homepageUrl,visibility,repositoryTopics,licenseInfo,defaultBranchRef
```

Expected: metadata proposal is ready, obsolete homepage is addressed, license display is understood, and visibility remains private pending approval.

## Two-Minute Review

An unfamiliar reviewer should correctly answer: what problem it solves; what the developer built; its prototype status; three difficult engineering aspects; and what proves real-world deployment.
