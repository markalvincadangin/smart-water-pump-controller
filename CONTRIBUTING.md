# Contributing to SmartFlow

First, thank you for your interest in SmartFlow! We welcome contributions that improve reliability, safety, and usability of this water pump automation system.

---

## Before You Start

Please understand that **SmartFlow is safety-critical hardware-controlling software**. A bug could result in:
- Pump malfunction or damage
- Tank overflow or water waste
- Property damage or electrical hazard

Because of this, we have rigorous testing requirements.

---

## Getting Started

### 1. Read the Safety Rules

Review [docs/specs/firmware_operational_rules.md](docs/specs/firmware_operational_rules.md) and related specs under `docs/specs/`.

Key principles:
- ✅ **Fail toward pump OFF** — Any ambiguity = stop the pump
- ✅ **Never weaken dry-run lockout** — Flow starvation protection is inviolable
- ✅ **Hardware failsafe is always primary** — TOR relay is independent safety layer

### 2. Understand the Architecture

- **Firmware:** PlatformIO ESP32 (master) + ESP8266 (sensor node)
- **Dashboard:** Next.js 15 TypeScript + Firebase RTDB
- **Protocol:** RS-485 half-duplex text frames

See [README.md](README.md) for system overview.

### 3. Set Up Local Development

```bash
# Clone the repo (copy the HTTPS or SSH URL from the GitHub "Code" button)
git clone <repo-url>
cd smart-water-pump-controller

# Firmware setup
cd firmware/master_node
pip install platformio
cp src/config/secrets.h.example src/config/secrets.h
# Edit secrets.h with your WiFi/Firebase credentials

# Dashboard setup
cd ../../dashboard
npm install
cp .env.local.example .env.local
# Edit .env.local with Firebase project ID
npm run dev  # Runs on http://localhost:3000
```

---

## Types of Contributions

### 1. Bug Reports

**Before opening an issue:**
- Check [docs/operations/troubleshooting.md](docs/operations/troubleshooting.md)
- Search existing issues
- Try the latest version

**When reporting:**
```markdown
## Describe the bug
[Clear description]

## Steps to reproduce
1. Set mode to AUTO
2. Tank level below 30%
3. ...

## Expected behavior
Pump should start

## Actual behavior
Pump did not start

## Environment
- Firmware version: v1.0.0
- Tank size: 660L
- Sensor: JSN-SR04T-2.0
- Controller: ESP32 DevKit V1
- Dashboard version: latest

## Logs
[Paste relevant serial output or Firebase logs]
```

### 2. Feature Requests

**Keep in mind:**
- Must not compromise safety (dry-run, overflow, emergency-stop)
- Must be backward-compatible with existing deployments
- Explain real-world use case

```markdown
## Feature request
[Title]

## Problem
What need does this solve?

## Proposed solution
Concrete description

## Alternatives considered
Any other approaches?

## Safety impact
- Does this touch pump control logic? YES / NO
- Could this delay a safety shutdown? YES / NO
```

### 3. Code Changes

**Small fixes (typos, docs, minor bugs):**
- Fork the repo
- Create a branch: `git checkout -b fix/issue-name`
- Commit with clear message
- Open PR

**Larger changes (new features, refactor):**
1. Open an issue first to discuss
2. Wait for maintainer feedback
3. Fork and implement
4. Follow testing requirements (see below)

---

## Testing Requirements

### For Any Firmware Changes

1. **Compile without errors**
   ```bash
   cd firmware/master_node
   pio run
   # Should succeed with ✓
   ```

2. **Run unit tests** (if affected code has tests)
   ```bash
   pio test -v
   # All tests pass
   ```

3. **Safety checklist** (if pump control logic touched)
   - [ ] Dry-run protection still enforced
   - [ ] Overflow protection still enforced
   - [ ] Emergency stop still latches
   - [ ] Level freshness gate still blocks starts
   - [ ] RS-485 failure safely de-energizes pump

4. **Hardware test** (if possible)
   - Deploy to ESP32
   - Verify pump control works as expected
   - Test error conditions

### For Dashboard Changes

1. **Type check**
   ```bash
   cd dashboard
   npm run build
   # No TypeScript errors
   ```

2. **Lint check**
   ```bash
   npm run lint
   # No errors
   ```

3. **Functional test**
   ```bash
   npm run dev
   # Test on http://localhost:3000
   # Verify UI renders correctly
   # Test form inputs work as expected
   ```

4. **Test suite** (if applicable)
   ```bash
   npm test
   # All tests pass
   ```

---

## Pull Request Process

1. **Create a feature branch**
   ```bash
   git checkout -b feature/descriptive-name
   ```

2. **Make changes**
   - Keep commits focused and logical
   - Write clear commit messages
   - Update docs/comments if needed

3. **Test locally** (follow testing section above)

4. **Push and open PR**
   ```bash
   git push origin feature/descriptive-name
   ```

5. **PR Description** (use template below)

### Pull Request Template

```markdown
## Description
[What does this PR do?]

## Type
- [ ] Bug fix (solves issue #___)
- [ ] Feature (adds new capability)
- [ ] Refactor (improves code, no behavior change)
- [ ] Documentation update

## Testing
[Describe how you tested this]

## Safety Impact
- [ ] Touches pump control logic
- [ ] Touches safety interlocks (dry-run, overflow, e-stop)
- [ ] Changes communication protocol

If checked, explain safety implications:
[Explain]

## Checklist
- [ ] Code compiles/runs without errors
- [ ] All tests pass (or explained why not)
- [ ] Documentation updated (if needed)
- [ ] No breaking changes (or explained migration)

## Related Issues
Fixes #___ or Related to #___
```

---

## Code Style

### Firmware (C++)

```cpp
// Use descriptive names
bool isLevelSensorError = false;  // ✓
bool err = false;                 // ✗

// Use const where appropriate
const unsigned long TIMEOUT_MS = 5000;

// Document safety-critical logic
// HARD SAFETY: Pump must stop if level is stale
if (now - levelLastUpdateMs > LEVEL_STALE_TIMEOUT_MS) {
  setPump(false);  // Failsafe: stop pump
}

// Use snprintf safely
char buf[64];
snprintf(buf, sizeof(buf), "Level: %d%%", level);  // ✓
sprintf(buf, "Level: %d%%", level);                // ✗ (buffer overflow risk)
```

### Dashboard (TypeScript/React)

```typescript
// Use strict types
interface PumpProps {
  level: number;
  isRunning: boolean;
}

// Document complex logic
/**
 * Derives control mode from hardware state + user intent.
 * Safety: Always prefers OFF if emergency stop is latched.
 */
function getControlMode(): ControlMode { ... }

// Use meaningful variable names
const isPumpStale = now - lastUpdate > 5000;  // ✓
const stale = now - last > 5000;              // ✗
```

---

## Branches & Version Control

### Branching Strategy

- **`master`** — Stable production branch. Only receives PRs from `dev` when a release or integration milestone is verified.
- **`dev`** — Active integration branch. All feature branches and bug fixes branch off and PR into `dev`.

### Branch Naming

```
feature/short-description          # New feature
fix/bug-description                # Bug fix
docs/update-something              # Documentation
refactor/component-name            # Code refactoring
```

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
type(scope): short description

feat(firmware): add countdown mode support
fix(dashboard): resolve level display rounding error
docs(safety): clarify dry-run protection in README
test(rs485): add frame validation test
```

---

## Documentation

Please update docs when appropriate:

- **Code comments** — Explain WHY, not WHAT
- **README.md** — High-level overview
- **docs/specs/** — Architecture and protocol specs
- **docs/operations/** — Operational guides and troubleshooting

For safety-critical changes, add an entry to [DEPLOYMENT_SAFETY.md](DEPLOYMENT_SAFETY.md).

---

## Asking Questions

Unsure about something?

- **GitHub Discussions** — General questions about SmartFlow
- **GitHub Issues** — Bug reports with details
- **Email** — Private security concerns to markc.dev.iot@gmail.com

---

## Code of Conduct

We follow a simple principle: **Be respectful.**

- Critique code, not people
- Assume good intent
- Be patient with new contributors
- Help others learn

Harassment, discrimination, or abuse will not be tolerated.

---

## Recognition

Contributors are recognized in:
- [CONTRIBUTORS.md](CONTRIBUTORS.md) — (when we create it)
- GitHub PR comments and tags
- Release notes

---

## License

By contributing, you agree that your code will be licensed under the **Apache License 2.0**. See [LICENSE](LICENSE) for details.

---

**Thank you for helping make SmartFlow safer and better! 🙏**
