# Security Policy

## Reporting Vulnerabilities

We take security seriously, especially for code that controls physical hardware and electrical systems.

**If you discover a security vulnerability, please report it responsibly:**

### DO NOT:
- ❌ Open a public GitHub issue
- ❌ Post on social media
- ❌ Disclose the vulnerability before we have a fix

### DO:
- ✅ Email the details to: **markc.dev.iot@gmail.com**
- ✅ Include a clear description of the vulnerability
- ✅ Provide steps to reproduce (if possible)
- ✅ Specify which version(s) are affected

---

## Scope: What We Consider Security Issues

### In-Scope (Serious)
- **Command injection** — Ability to execute arbitrary code on ESP32 or Node.js
- **Authentication bypass** — Circumventing Firebase auth or dashboard login
- **Unsafe cryptography** — Weak keys, unencrypted secrets
- **Buffer overflows** — Memory corruption in firmware
- **Logic flaws in safety** — Pump control bypasses (dry-run, overflow, emergency stop)
- **Sensor spoofing** — Ability to forge RS-485 frames and trigger false levels
- **Denial of Service (DoS)** — Crashing the controller via malformed input

### Out-of-Scope (Research/Non-Critical)
- Missing rate-limiting on non-critical API endpoints
- UI/UX issues that don't affect safety
- Documentation typos
- Performance issues
- Vulnerabilities in third-party libraries (report to the library maintainer)

---

## Response Timeline

We aim to:

| Severity | Time to Acknowledge | Time to Fix | Disclosure |
|----------|-------------------|------------|-----------|
| **Critical** (Exploitable safety bypass) | 24 hours | 1 week | 30 days after patch |
| **High** (Auth bypass, RCE) | 48 hours | 2 weeks | 60 days after patch |
| **Medium** (Minor logic flaw) | 1 week | 1 month | 90 days after patch |

---

## Security Best Practices (For Users)

### Credential Rotation (Required After Any Exposure)
- [ ] Rotate WiFi credentials used by controllers
- [ ] Rotate Firebase Email/Password service user credentials
- [ ] Rotate Firebase API keys and any server-side keys/tokens
- [ ] Rotate OTA credentials in `secrets_ota.h`
- [ ] Rotate any local service account keys used by scripts/automation
- [ ] Reflash devices with new credentials
- [ ] Revoke old sessions/tokens where applicable
- [ ] Verify old secrets are removed from git history

### Deployment
- [ ] Use strong WiFi passwords (WPA3 if available)
- [ ] Change Firebase email/password from defaults
- [ ] Never commit `secrets.h` to version control
- [ ] Keep firmware updated to latest version
- [ ] Monitor telemetry for unusual activity

### Monitoring
- [ ] Check dashboard logs weekly for errors
- [ ] Verify pump cycle counts are reasonable
- [ ] Alert if level readings become erratic (possible sensor tampering)
- [ ] Audit Firebase rules periodically

### Access Control
- [ ] Limit dashboard user accounts to trusted operators
- [ ] Disable public read-write on Firebase RTDB
- [ ] Use Firebase Security Rules: Operator role + Admin role (separate)
- [ ] Revoke access for users no longer active

---

## Known Limitations

We acknowledge these security constraints:

1. **WiFi Security**
   - SmartFlow transmits control commands over WiFi
   - An attacker on the same network could intercept or replay commands
   - **Mitigation:** Use WPA3 encryption; consider VPN for remote access

2. **Firebase RTDB Limits**
   - Real-time database uses standard Firebase auth
   - No end-to-end encryption between client and RTDB
   - **Mitigation:** Use HTTPS; assume network is untrusted

3. **Firmware Integrity**
   - No secure boot or attestation (ESP32 doesn't have it by default)
   - Firmware could be replaced with malicious version if attacker has USB access
   - **Mitigation:** Physical security of the ESP32/enclosure

4. **RS-485 Network**
   - RS-485 is not encrypted
   - Sensor node could be impersonated if cable is accessible
   - **Mitigation:** Use isolated network; monitor for unexpected messages

---

## Responsible Disclosure

If we receive a valid security report, we will:

1. Acknowledge receipt within 24 hours (critical issues)
2. Investigate and reproduce the issue
3. Develop a fix
4. Test the fix thoroughly
5. Release a patched version
6. Credit the finder UNLESS they request anonymity

We ask that you:
- Give us reasonable time to fix before disclosure
- Not access data beyond what's needed to demonstrate the vulnerability
- Not disrupt service or damage systems
- Work with us cooperatively

---

## Security Contact

**Email:** markc.dev.iot@gmail.com  
**Subject:** `[SmartFlow Security]` + brief title

Example:
```
Subject: [SmartFlow Security] Unauthenticated Pump Control via Firebase Rules
```

---

## Supported Versions

Only the latest release receives security updates.

| Version | Released | Security Support |
|---------|----------|------------------|
| v1.0.0 | 2026-04 | ✅ Active |
| v0.9.0 | 2026-03 | ⚠️ Limited (critical only) |

If you're running an older version, please upgrade to v1.0.0 or later.

---

## Security Incidents History

*None yet — this is a new project (2026). Security history will be logged here.*

---

**Last Updated:** 2026-04-04  
**Maintained By:** Mark Alvin Cadangin
