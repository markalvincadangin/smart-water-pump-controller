# Portfolio Readiness Research

## Positioning

**Decision**: Present SmartFlow as a “field-deployed IoT water-pump controller prototype” built and installed by a sole developer for a real household system.

**Rationale**: Accurate and credible without implying certification, commercial scale, or independently measured production reliability.

**Rejected**: “industrial-grade” and unqualified “production” overstate available evidence; “demo” understates an operating installation.

## Authorship and AI Assistance

**Decision**: State sole ownership and end-to-end responsibility. Do not feature routine AI assistance unless an application asks, attribution is legally required, or the AI workflow is relevant to the role.

**Rationale**: Tools do not replace authorship; the owner remains responsible for requirements, decisions, integration, validation, deployment, and results. Direct questions should be answered honestly.

## Location Privacy

**Decision**: Use “Iloilo, Philippines” or “a residential installation in the Philippines.” Exclude exact address, landmarks, coordinates, network names, device identifiers, accounts, and revealing images.

**Rationale**: Regional context proves field deployment without unnecessarily identifying the household.

## Public-Repository Safety

**Decision**: Keep the repository private until a current-tree and history-aware audit passes and the owner explicitly approves publication.

**Rationale**: GitHub documents that post-publication cleanup may require history rewriting and cannot remove data from existing clones or forks. Active secrets must be revoked or rotated, not merely deleted. Public repositories should use available secret scanning, push protection, dependency alerts, and a security policy.

## Current Architecture Source

**Decision**: Use `docs/specs/`, the constitution, current source, and executable tests as evidence. Treat `archive/` and `docs/archive/` as history.

**Rationale**: The README and active documents still describe a removed web dashboard, while the supported client is native Android.

## Visual Evidence

**Decision**: Curate one enclosure hero image and two to four Android screenshots for overview, controls/safety, provisioning/ownership, and diagnostics. Store processed copies under `docs/assets/portfolio/`.

**Rationale**: A focused gallery is persuasive and lowers disclosure risk while preserving originals.

## CI Scope

**Decision**: Replace obsolete web deployment with validation for Android and Cloud Functions. Do not automate a retired or unverified deployment target.

## GitHub Metadata

**Decision**: Prepare a concise description and relevant topics; clear the obsolete Vercel homepage unless a current URL is supplied. Keep external changes behind review.
