---
status: draft
version: 0.1
last-reviewed: 2026-08-21
source: hand-authored
---

# Feature Specification: Portfolio Readiness

**Feature Branch**: `[010-portfolio-readiness]`

**Created**: 2026-08-21

**Status**: Draft

**Input**: User description: "Make SmartFlow portfolio- and resume-ready, accurately reflecting the current native Android, firmware, and cloud implementation; remove obsolete Next.js material; fix related repository issues; use available screenshots and enclosure photography; and review privacy and security before public presentation."

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | Public documentation must describe fail-safe behavior accurately and must not suggest remote or manual controls bypass mandatory shutdown behavior. No pump-control behavior is changed by this feature. |
| II. Dry-Run Lockout | Yes | Portfolio material must describe persistent dry-run lockout accurately and link to its canonical specification rather than duplicating a divergent contract. |
| III. Overflow Protection | Yes | Portfolio material must describe maximum-runtime protection across supported operating modes accurately. |
| IV. TOR Independence | Yes | The independent hardware overload layer must remain clearly distinguished from software safeguards. |
| V. Sensor Freshness / E-Stop | Yes | Architecture and feature summaries must preserve stale-data blocking and reachable emergency-stop concepts without overstating validation. |
| VI. Backward Compatibility | No | This feature changes presentation, documentation, automation, and repository metadata; it does not change RS-485 or database contracts. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Understand the Project Quickly (Priority: P1)

As a recruiter or hiring manager, I can understand within the opening portion of the project page what SmartFlow solves, what the developer personally built, where it is deployed, and which engineering disciplines it demonstrates.

**Why this priority**: The repository's primary portfolio purpose fails if a reviewer cannot quickly identify the project's value, scope, and ownership.

**Independent Test**: Give the project page to a reviewer unfamiliar with SmartFlow and verify that they can correctly state the problem, system boundaries, deployment status, developer ownership, and current user interface after a two-minute review.

**Acceptance Scenarios**:

1. **Given** a reviewer opens the repository, **When** they read the opening project summary, **Then** they see an accurate description of a field-deployed prototype with embedded controllers, a native Android app, and cloud services.
2. **Given** a reviewer wants to assess the developer's contribution, **When** they read the project overview, **Then** they can identify that one developer owned the design and implementation across hardware, firmware, mobile, cloud, testing, and deployment.
3. **Given** the project was developed with AI-assisted tools, **When** authorship is presented, **Then** the developer's ownership is clear without making development tooling the centerpiece or implying unearned work.

---

### User Story 2 - Verify Engineering Depth (Priority: P1)

As a technical reviewer, I can inspect concise, evidence-backed explanations of the architecture, safety model, communications, mobile experience, backend responsibilities, and field deployment.

**Why this priority**: Credible technical evidence distinguishes this project from a tutorial or concept mock-up.

**Independent Test**: Cross-check every technical statement and architecture label in the portfolio-facing material against current source code, canonical specifications, tests, or deployment evidence.

**Acceptance Scenarios**:

1. **Given** a technical claim appears in portfolio-facing material, **When** it is audited, **Then** it is supported by current repository evidence and does not depend on retired components.
2. **Given** a reviewer follows architecture and documentation links, **When** each link is opened, **Then** it resolves to current and relevant content.
3. **Given** the project includes physical safety controls, **When** the safety section is reviewed, **Then** it differentiates independent hardware protection, firmware safeguards, operator controls, and known limitations.

---

### User Story 3 - See the System in Use (Priority: P2)

As a portfolio reviewer, I can see curated Android screenshots and an enclosure photograph that demonstrate a real, coherent, field-installed system without exposing private household or device information.

**Why this priority**: Visual evidence makes the field deployment and product quality immediately credible.

**Independent Test**: Review all published images at repository scale and verify that they are legible, captioned, relevant, and free of secrets or unnecessary identifying details.

**Acceptance Scenarios**:

1. **Given** suitable screenshots and an enclosure photograph are available, **When** they are included, **Then** each image has a clear purpose and accessible description.
2. **Given** an image contains account details, precise location data, device identifiers, network names, tokens, or other sensitive information, **When** it is prepared for publication, **Then** that information is cropped, redacted, or the image is excluded.

---

### User Story 4 - Build and Navigate the Current Repository (Priority: P2)

As a developer evaluating the portfolio, I can follow current setup and validation guidance for the supported components without encountering commands for removed software.

**Why this priority**: Broken setup instructions and automation undermine trust in the entire project.

**Independent Test**: Execute or statically validate every documented command and automation path that can run without physical hardware or production credentials.

**Acceptance Scenarios**:

1. **Given** a developer follows project structure and setup sections, **When** they navigate the repository, **Then** every named component and path exists.
2. **Given** repository automation runs, **When** it validates software components, **Then** it targets the native mobile app and current cloud services rather than the removed web dashboard.
3. **Given** hardware is unavailable, **When** validation is reported, **Then** unverified physical behavior is explicitly distinguished from executable software checks.

---

### User Story 5 - Prepare for Safe Public Presentation (Priority: P2)

As the project owner, I can confidently make the repository portfolio-visible after a documented privacy, secret, licensing, and metadata review.

**Why this priority**: Public exposure is difficult to reverse once sensitive data enters clones, caches, or forks.

**Independent Test**: Complete a publication-readiness audit covering the working tree, tracked history indicators, images, configuration examples, repository metadata, and public-facing links before requesting any visibility change.

**Acceptance Scenarios**:

1. **Given** the repository is still private, **When** portfolio improvements are completed, **Then** visibility remains unchanged until the owner explicitly authorizes publication after reviewing the audit results.
2. **Given** sensitive or household-specific information is found, **When** remediation is planned, **Then** active credentials are rotated where necessary and risky content is excluded or generalized before publication.
3. **Given** repository metadata is reviewed, **When** it is prepared for portfolio use, **Then** the description, topics, homepage, license display, and automation status match the current project.

### Edge Cases

- Available screenshots contain personal accounts, notification content, device identifiers, Wi-Fi names, or precise household information.
- The enclosure photograph exposes labels, addresses, unsafe wiring presentation, or details that require a safety disclaimer.
- A claim is implemented in a dirty working tree but not yet represented in the committed default branch.
- A canonical specification or constitution still names the retired web dashboard even though the current app is native Android.
- A build or test requires unavailable credentials, an Android SDK, a specific JDK, PlatformIO, or physical hardware.
- A green status badge would imply passing automation even though the current workflow has not run successfully.
- Repository history contains a credential that cannot be made safe merely by deleting it from the current tree.
- Quantitative deployment metrics are unavailable; the portfolio must avoid invented uptime, cycle, reliability, or impact numbers.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Portfolio-facing material MUST identify SmartFlow as a personal, field-deployed prototype operating at the owner's residence, using location detail no more precise than necessary to establish real-world deployment.
- **FR-002**: Portfolio-facing material MUST accurately state that the owner independently designed and developed the hardware integration, firmware, native mobile application, cloud services, testing approach, documentation, and field installation, with development tools treated separately from authorship.
- **FR-003**: The project overview MUST prioritize the real-world problem, delivered system, engineering challenges, current architecture, and evidence before setup or operational detail.
- **FR-004**: All references to the retired web dashboard, its framework, its hosting service, its directory, and its commands MUST be removed or archived wherever they incorrectly describe the current system.
- **FR-005**: All portfolio-facing technical claims MUST be reconciled against current source code, canonical specifications, executable tests, repository structure, or clearly labeled field evidence.
- **FR-006**: The project page MUST include a concise architecture representation covering the tank sensor, master controller, pump-control hardware, cloud boundary, backend services, and native mobile app.
- **FR-007**: The project page MUST present the safety architecture accurately, including fail-toward-OFF behavior, dry-run lockout, overflow protection, stale sensor handling, emergency stop, and independent thermal overload protection.
- **FR-008**: The project page MUST include only verifiable deployment claims and MUST label the system as a field-deployed prototype rather than an industrial-certified or broadly production-proven product.
- **FR-009**: Available mobile screenshots and enclosure photography MUST be assessed, curated, captioned, optimized for repository viewing, and checked for sensitive information before inclusion.
- **FR-010**: Setup, project structure, testing, and validation instructions MUST target only current supported components and existing paths.
- **FR-011**: All internal links and badges in portfolio-facing material MUST resolve correctly and represent observable current status.
- **FR-012**: Repository automation MUST validate the supported mobile and cloud components and MUST not depend on removed components.
- **FR-013**: Related current-state documentation and project governance files MUST be corrected when they materially misidentify the current component architecture.
- **FR-014**: Detailed operations, calibration, troubleshooting, and commissioning content MUST remain accessible through focused documentation rather than overwhelming the portfolio narrative.
- **FR-015**: A publication-readiness review MUST check the current tree and reasonable history indicators for credentials, personal data, exact device identifiers, network details, private endpoints, and unsafe image content.
- **FR-016**: Any discovered live secret MUST be treated as compromised and flagged for revocation or rotation; deleting the current-file occurrence alone MUST NOT be presented as sufficient remediation.
- **FR-017**: Repository visibility MUST NOT be changed without explicit owner authorization after publication-readiness findings are presented.
- **FR-018**: Public repository metadata MUST be prepared with an accurate description, relevant discoverability topics, a valid optional homepage, and a correctly detected license.
- **FR-019**: The final result MUST include concise resume-ready project bullets that use only substantiated outcomes and technologies.
- **FR-020**: Existing unrelated working-tree changes MUST be preserved and MUST NOT be overwritten during this feature.

### Key Entities

- **Portfolio Narrative**: The recruiter-facing story covering problem, ownership, architecture, challenges, evidence, and outcomes.
- **Technical Claim**: A public assertion about behavior, technology, deployment, safety, or validation with a traceable evidence source.
- **Visual Asset**: A screenshot, photograph, diagram, or badge prepared for public display with accessibility text and privacy review.
- **Publication Finding**: A privacy, credential, licensing, metadata, or operational risk that must be resolved or accepted before publication.
- **Repository Metadata**: The public description, topics, homepage, visibility, license classification, and automation status shown by the hosting platform.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In a two-minute review, an unfamiliar reader can correctly identify the project's problem, current system components, sole-developer ownership, deployment status, and three substantive engineering challenges.
- **SC-002**: One hundred percent of portfolio-facing component names, paths, commands, links, badges, and architecture labels match the reviewed current repository state.
- **SC-003**: Zero unsupported references to the retired web dashboard remain in current portfolio, automation, governance, or source-of-truth material.
- **SC-004**: One hundred percent of prominent technical and deployment claims have repository evidence or are explicitly qualified as personal field observations.
- **SC-005**: Every published visual is legible, purposefully captioned, accessible, and contains no visible credential, account identifier, precise household location, network name, or private device identifier.
- **SC-006**: All software validation that can run in the available environment passes, while every hardware- or credential-dependent validation gap is explicitly reported.
- **SC-007**: The publication audit reports zero known active secrets before any recommendation to make the repository public.
- **SC-008**: The final project page can be reduced to three accurate resume bullets without introducing new claims or technologies.

---

## Assumptions

- The current supported user interface is the native Android application; the former web dashboard is retired and should not remain part of current-state presentation or automation.
- The installation is operational at the owner's residence and may be described at the region or province level only when useful; exact address and household-identifying details are excluded.
- The project is accurately described as a field-deployed prototype, not as certified industrial equipment or a product with independently verified commercial reliability.
- No uptime, pump-cycle, energy-saving, water-saving, or failure-prevention metric will be invented when measurement evidence is unavailable.
- AI assistance does not change sole project ownership. Routine AI-tool disclosure is not featured in the portfolio narrative unless a specific employer, application, license, or material attribution obligation requires it.
- Repository visibility is currently private and remains so throughout implementation unless the owner later gives explicit authorization to change it.
- GitHub metadata may be proposed and prepared, but external metadata changes are performed only after their exact values and publication safety are verified.
- Existing Android screenshots and enclosure photography will be used if suitable files can be located or supplied; otherwise documentation will reserve a clear asset location without fabricating visuals.
- This feature changes documentation, automation, assets, and repository presentation only; it does not alter pump-control behavior, electrical wiring, database contracts, or field firmware.
