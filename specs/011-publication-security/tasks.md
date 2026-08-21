# Tasks: Publication Security Hardening

- [x] T001 Record baseline dependency and test state in `specs/011-publication-security/results.md`
- [x] T002 [US1] Upgrade compatible Firebase dependencies in `functions/package.json` and `functions/package-lock.json`
- [x] T003 [US1] Build and test Cloud Functions after compatible upgrades
- [x] T004 [US1] Review remaining critical/high advisories and apply the smallest safe remediation
- [x] T005 [US1] Rebuild, retest, and record final audit disposition
- [x] T006 [US2] Install or run Gitleaks outside tracked project content
- [x] T007 [US2] Run a redacted full-history scan and classify findings without committing secrets or reports
- [x] T008 [US3] Recheck GitHub security feature availability without changing visibility
- [x] T009 [US3] Update `specs/010-portfolio-readiness/publication-review.md` and `specs/011-publication-security/results.md`

## Dependencies

T001 precedes dependency changes. T002-T005 are sequential. T006-T007 may run after baseline. T008-T009 follow local remediation.
