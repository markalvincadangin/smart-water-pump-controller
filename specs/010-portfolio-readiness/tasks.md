# Tasks: Portfolio Readiness

**Branch**: `[010-portfolio-readiness]` | **Spec**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md)

## Phase 1: Shared Setup

- [x] T001 Confirm constitution and publication gates in `specs/010-portfolio-readiness/plan.md`
- [x] T002 Inventory current architecture and claims across `docs/specs/`, `app/`, `firmware/`, and `functions/`
- [x] T003 Record media privacy findings for `docs/assets/portfolio/originals/`

## Phase 2: Foundational Audit

- [x] T004 Audit active documentation for retired Next.js/Vercel paths in `README.md`, `.github/`, `CONTRIBUTING.md`, `DEPLOYMENT_SAFETY.md`, `SECURITY.md`, `.specify/memory/`, `docs/operations/`, and `firmware/README.md`
- [x] T005 Audit local links and public-release risks without exposing values in `README.md` and tracked configuration
- [x] T006 Define evidence-backed README claims in `specs/010-portfolio-readiness/research.md`

## Phase 3: Understand the Project Quickly (US1)

- [x] T007 [US1] Rewrite the opening, problem, ownership, deployment status, and highlights in `README.md`
- [x] T008 [US1] Add a concise current architecture and engineering-challenges narrative to `README.md`
- [x] T009 [US1] Validate the two-minute recruiter review against `README.md`

## Phase 4: Verify Engineering Depth (US2)

- [x] T010 [US2] Reconcile safety, firmware, RS-485, Firebase, Android, and Functions claims in `README.md` with `docs/specs/`
- [x] T011 [US2] Move operational detail out of the root narrative by linking current documents from `README.md`
- [x] T012 [US2] Correct active governance and contributor references in `CONTRIBUTING.md` and `.specify/memory/constitution.md`

## Phase 5: See the System in Use (US3)

- [x] T013 [US3] Select privacy-safe enclosure, controls, and provisioning assets from `docs/assets/portfolio/originals/`
- [x] T014 [US3] Add accessible captions and image references to `README.md`
- [x] T015 [US3] Exclude the visible device-ID and unavailable-telemetry screenshots from `README.md`

## Phase 6: Build and Navigate the Current Repository (US4)

- [x] T016 [US4] Replace retired dashboard CI with Android and Functions validation in `.github/workflows/validate.yml`
- [x] T017 [US4] Correct current setup, structure, stack, validation, and documentation links in `README.md`
- [x] T018 [US4] Correct native-app terminology in `DEPLOYMENT_SAFETY.md` and `SECURITY.md`
- [x] T019 [US4] Validate Android build/tests and Functions build/tests using commands in `specs/010-portfolio-readiness/quickstart.md`

## Phase 7: Safe Public Presentation (US5)

- [x] T020 [US5] Verify repository metadata and document proposed values without changing visibility
- [x] T021 [US5] Run link, retired-reference, diff, and secret-pattern checks for the public candidate
- [x] T022 [US5] Prepare accurate GitHub description, topics, homepage disposition, and resume bullets in `specs/010-portfolio-readiness/publication-review.md`

## Phase 8: Polish

- [x] T023 Recheck all README claims, visual privacy, Markdown rendering, and link targets in `README.md`
- [x] T024 Mark completed work and record validation limitations in `specs/010-portfolio-readiness/tasks.md`
- [x] T025 Replace the open-source license with an all-rights-reserved portfolio notice in `LICENSE` and `README.md`
- [x] T026 Align personal-project vulnerability reporting and support expectations in `SECURITY.md`
- [x] T027 Remove external-contribution permission ambiguity in `CONTRIBUTING.md` and package metadata

## Dependencies

- Phase 1 precedes all work; Phase 2 precedes README changes.
- US1 and US2 establish the narrative before US3 assets and US4 setup/CI.
- US5 depends on all repository edits and validation results.
- Publication and external metadata changes remain outside these tasks until explicit owner approval.

## Independent Tests

- **US1**: An unfamiliar reviewer correctly summarizes the project in two minutes.
- **US2**: Every prominent claim maps to current code, specs, tests, or qualified field observation.
- **US3**: Every linked image is useful, captioned, and free of identified sensitive content.
- **US4**: All paths exist and supported software validation commands run or report exact environment blockers.
- **US5**: Publication review has no concealed blocker and visibility remains private.

## Validation limitations

- Git history contains earlier revisions of credential-bearing paths, but no history-aware secret scanner is installed locally. The publication review therefore keeps visibility private and requires a redacted history scan plus rotation of any still-valid credential.
- Physical pump, mains-voltage, sensor-loss, and overload tests were not repeated for this documentation feature.
- Android validation ran on the locally available JDK 25; CI is configured for the repository-required JDK 21.
