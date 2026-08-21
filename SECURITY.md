# Security Policy

SmartFlow is a personal, field-deployed prototype maintained by one developer. This policy explains how to report a vulnerability safely; it does not represent a commercial support agreement or guaranteed response schedule.

## Supported version

Only the latest commit on the default branch is currently considered for security fixes. SmartFlow has no supported public release or long-term-support version.

| Version | Supported |
|---------|-----------|
| Latest default-branch revision | Yes, on a best-effort basis |
| Private development history and retired experiments | No |

## Reporting a vulnerability

Do not open a public issue containing exploit details, credentials, personal data, or instructions that could operate physical equipment.

Report privately by email:

- **Address:** `markc.dev.iot@gmail.com`
- **Subject:** `[SmartFlow Security]` followed by a short title

Include, where available:

- the affected component and revision;
- a concise description of the impact;
- safe reproduction steps using your own test environment;
- logs or screenshots with secrets and personal data removed;
- whether you believe physical safety could be affected.

After the repository becomes public, GitHub private vulnerability reporting should be used when it is enabled. The email address above remains the fallback.

## Responsible research boundaries

- Test only systems, accounts, devices, and data you own or are explicitly authorized to test.
- Do not send commands to the field installation or attempt to discover its network, cloud project, accounts, or device identifiers.
- Do not energize pumps, contactors, mains wiring, or exposed prototype hardware for security testing.
- Do not access, retain, or disclose data beyond what is necessary to explain the issue.
- Stop testing if it could affect people, property, water supply, electrical equipment, or third-party services.
- Do not use denial-of-service, social-engineering, persistence, destructive, or credential-stuffing techniques.

## In scope

- Unauthorized pump-control intent or safety-control bypass
- Authentication, device bootstrap, pairing, ownership, or account-isolation failures
- Firebase Security Rules or Cloud Functions authorization errors
- Exposure of bootstrap secrets, service-account credentials, signing material, passwords, or deployment tokens
- Firmware memory-safety problems reachable through supported interfaces
- Malformed RS-485 or cloud input that causes unsafe controller behavior
- Dependency vulnerabilities that materially affect SmartFlow

Documentation errors, ordinary UI defects, theoretical issues without a meaningful SmartFlow impact, and physical attacks requiring possession of the prototype may be handled as normal issues rather than security vulnerabilities.

## Response expectations

Reports are reviewed on a best-effort basis. I may not be able to acknowledge or fix an issue within a specific period. If a report is valid, I will aim to reproduce it safely, assess physical and data risk, prepare a fix or mitigation, and coordinate disclosure when practical.

There is no bug-bounty program and no promise of payment. Credit may be offered with the reporter's permission.

## Known security limitations

- The prototype has not completed an independently audited secure-boot and signed-firmware deployment chain.
- The local RS-485 sensor link provides framing and CRC-based error detection, not encryption or cryptographic peer authentication.
- Physical access to the controller, sensor node, wiring, or debug interfaces is outside the software trust boundary.
- Remote communication depends on Firebase's authenticated TLS services and correctly deployed Security Rules and backend authorization.
- The system is installed at one residential site and has not undergone a commercial penetration test or product certification.

These limitations must not be interpreted as permission to test the installed system.

## Credential exposure

If a real credential is exposed, revoke or rotate it first. Removing it from the latest commit is not sufficient because it may remain in Git history, caches, forks, or local clones. Relevant examples include service-account keys, device bootstrap secrets, passwords, OTA credentials, and deployment tokens.

Firebase client configuration identifiers should still be restricted and monitored appropriately, but they are not a replacement for backend authorization or Security Rules.

## Physical safety

A software license, security review, or successful build does not certify the electrical installation. SmartFlow must fail toward pump OFF, and the independent contactor and thermal overload protections must not be replaced or bypassed by software.
