# Implementation Plan: Portfolio Readiness

**Branch**: `[010-portfolio-readiness]` | **Date**: 2026-08-21 | **Spec**: [spec.md](./spec.md)

## Summary

Rebuild SmartFlow's public presentation around its current field-deployed ESP32/ESP8266, Firebase, Cloud Functions, and native Android architecture. Begin with a claim-and-publication audit, rewrite the root narrative, incorporate privacy-reviewed visuals, remove active Next.js/Vercel drift from current documentation and CI, validate runnable components and links, and prepare—but do not publish—accurate GitHub metadata and resume bullets.

## Constitution Gate

| Principle | Status | Evidence |
|-----------|--------|----------|
| I. Fail Toward Pump OFF | Pass | Wording is checked against the constitution and operational spec; no relay behavior changes. |
| II. Dry-Run Lockout | Pass | README links to canonical persistent-lockout behavior rather than redefining it. |
| III. Overflow Protection | Pass | Summary covers runtime lockout across supported modes without changing behavior. |
| IV. TOR Independence | Pass | Hardware overload remains a distinct independent protection layer. |
| V. Sensor Freshness / E-Stop | Pass | Summaries are sourced from current specifications and code. |
| VI. Backward Compatibility | N/A | No RS-485 or RTDB contract changes. |

The gate remains satisfied after design because changes are limited to documentation, media, CI validation, and repository presentation.

## Technical Context

**Affected**: `README.md`, Android/build evidence in `app/`, Functions validation in `functions/`, `.github/workflows/`, current governance and documentation, and curated portfolio media. Firmware and sensor runtime behavior are evidence sources only.

**Firebase RTDB impact**: None. **RS-485 impact**: None.

**Validation environment**: Windows/PowerShell; Gradle with JDK 21 and Android SDK; Node.js 22/npm; authenticated GitHub CLI. Physical hardware and production credentials are not required for routine validation.

**External-state constraint**: Repository visibility stays private. Metadata and visibility changes require a reviewed proposal; publication requires explicit owner authorization.

## Project Structure

```text
specs/010-portfolio-readiness/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/portfolio-publication.md
└── tasks.md

README.md
.github/workflows/deploy.yml
CONTRIBUTING.md
DEPLOYMENT_SAFETY.md
SECURITY.md
.specify/memory/constitution.md
docs/README.md
docs/specs/
docs/operations/
docs/assets/portfolio/
```

## Phase 0: Audit & Research

1. Inventory current hardware, firmware, cloud, mobile, safety, provisioning, ownership, notifications, diagnostics, and validation capabilities.
2. Build a claim matrix for prominent README statements: evidence, confidence, disposition, and canonical link.
3. Treat `archive/` and `docs/archive/` as historical; do not rewrite history merely to remove keywords.
4. Audit public-release risks in tracked files, history indicators, images, metadata, and external links without printing secret values.
5. Locate owner-provided screenshots and enclosure photo; otherwise record the asset handoff needed.
6. Record GitHub visibility, description, homepage, topics, license detection, and Actions state.
7. Output: [research.md](./research.md).

## Phase 1: Design

1. Define recruiter-facing hierarchy: outcome, field context, ownership, evidence, architecture, challenges, safety, visuals, stack, validation, setup, and deeper documentation.
2. Define claim qualifications: field-deployed prototype, owner-observed, code-verified, test-verified, and hardware-validation limits.
3. Define image selection, redaction, naming, optimization, captions, and alt text.
4. Separate portfolio summary from canonical operational detail to prevent duplication drift.
5. Define CI for Android and Functions; exclude deployment until a current destination is verified.
6. Define proposed metadata and explicit publication gate.
7. Outputs: [data-model.md](./data-model.md), [publication contract](./contracts/portfolio-publication.md), and [quickstart.md](./quickstart.md).

## Phase 2: Planned Implementation Sequence

1. Complete claim, secret/privacy, link, media, and metadata audits.
2. Prepare approved media copies without altering originals.
3. Rewrite `README.md` as the portfolio case study.
4. Correct stale current-state references in governance, contribution, safety, security, specs, firmware guide, and active operations docs; preserve archives.
5. Replace obsolete web deployment automation with current Android and Functions validation.
6. Run links, retired-reference checks, Android and Functions validation, and privacy/secret checks.
7. Present proposed GitHub metadata, publication findings, and resume bullets.
8. Apply external metadata or visibility only after explicit approval.

## Validation Strategy

- Map every prominent claim to code, canonical specs, tests, images, or qualified owner observation.
- Resolve all root README links and manually check anchors/external links.
- Classify current-scope `Next.js`, `Vercel`, `PWA`, and `dashboard/` hits; permit generic Android-dashboard wording.
- Run `./gradlew.bat test` and `./gradlew.bat assembleDebug` with JDK 21 when configured.
- Run `npm ci`, `npm run build`, and `npm test` under `functions/`.
- Inspect each selected image before and after processing.
- Scan for likely secrets without echoing values; report automated-detection limits.
- Compare git diffs carefully and preserve unrelated user changes.

## Complexity Tracking

No constitution violations or architectural exceptions are required.
