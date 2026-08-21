# Tasks: Original Repository Publication

## Phase 1: Recovery foundation

- [ ] T001 Create and integrity-check a complete bundle under a private temporary backup path
- [ ] T002 Create a working-state snapshot including tracked modifications and untracked non-ignored files
- [ ] T003 Record branch, tag, remote, and working-tree inventory in private migration evidence

## Phase 2: Audit and rewrite rules

- [x] T004 [US1] Run a redacted full-history secret scan and classify findings
- [x] T005 [US1] Inventory sensitive, obsolete, private-media, and internal-tooling paths across history
- [x] T006 [US1] Define removal and identifier-replacement rules from the audit evidence

## Phase 3: Rewritten candidate

- [x] T007 [US2] Rewrite a disposable clone using the approved rules
- [x] T008 [US2] Verify retained branches, tags, commits, and expected project components
- [x] T009 [US2] Run full-history secret, excluded-path, identifier, and link scans
- [x] T010 [US2] Run Functions build and all tests
- [x] T011 [US2] Run Android unit tests and debug assembly

## Phase 4: Private verification and original migration

- [x] T012 [US3] Push the rewritten candidate to a private verification repository
- [x] T013 [US3] Require successful GitHub CI and review repository metadata
- [x] T014 [US3] Replace the original private remote history only after all verification gates pass
- [x] T015 [US3] Re-run GitHub CI and security checks on the original private repository

## Phase 5: Owner publication gate

- [ ] T016 Document residual risks and request explicit approval before changing visibility
