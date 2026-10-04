# Anti-Overengineering & Minimal Implementation Standards

Adopt the "laziest senior engineer" mindset: **The best code is the code that never had to be written.**
AI agents frequently over-engineer solutions, invent superfluous abstractions, and introduce external dependencies for tasks that standard platform features already solve. Follow this ladder before writing or modifying any code.

---

## 1. The Decision Ladder

Before writing any new implementation, stop at the first rung that holds:

```
1. Does this need to exist?       → No: Skip it (YAGNI - You Aren't Gonna Need It).
2. Already in this codebase?      → Reuse the existing driver, service, component, or utility.
3. Language / Stdlib feature?     → Use C++ std / Kotlin stdlib directly.
4. Native platform capability?    → Use ESP-IDF/Arduino or Android Jetpack Compose built-ins.
5. Installed dependency does it?  → Use existing libraries (e.g., ArduinoJson, Firebase RTDB).
6. Can it be written cleanly?     → Prefer simple, direct, readable logic over indirection.
7. Only then:                     → Write the absolute minimum code that safely solves the problem.
```

> **Golden Rule**: Be lazy about the solution, never about reading. Always read existing files, contracts, and call paths thoroughly before writing code.

---

## 2. Invariants: What is NEVER on the Chopping Block

Brevity must never compromise system reliability or safety. Never remove or bypass:
- **Firmware Safety Gates**: E-Stop, Dry-Run detection, Maximum Runtime timeouts, and interlock latches. Safety overrides application intent.
- **Hardware Error Handling**: Boundary checks, pin state validation, communication timeouts (RS-485, Telnet, BLE, Wi-Fi).
- **Data Integrity & Contracts**: Firebase RTDB schema alignment (`docs/specifications/smartflow-system-contract.md`), state idempotency, and command acknowledgments.
- **Android UI Accessibility & Design System**: Material 3 semantic color tokens (`MaterialTheme.colorScheme`), haptic feedback on critical controls, and responsive layouts.

---

## 3. Firmware Guidelines (ESP32 / FreeRTOS / C++)

1. **Avoid Dynamic Memory Churn**:
   - Do not use dynamic heap allocations (`malloc`, `new`, unbounded `std::string` / `String` concatenations) inside loops or FreeRTOS task loops.
   - Use static buffers, `std::string_view`, fixed char arrays, or pre-allocated structures.
2. **Respect Architectural Layering**:
   - Follow downward-only dependency flow:
     `Application / Core` $\rightarrow$ `Safety` $\rightarrow$ `Services` $\rightarrow$ `Cloud / Network` $\rightarrow$ `Drivers` $\rightarrow$ `HAL`.
   - Never create artificial abstraction wrappers around a single hardware pin or a direct peripheral call if the existing HAL/driver already covers it.
3. **No Unneeded Design Patterns**:
   - Do not introduce complex generic factory patterns, visitor patterns, or deep inheritance hierarchies when a simple `enum class`, `switch` statement, or single function is cleaner and faster.
4. **PlatformIO Dependencies**:
   - Do not add new entries to `platformio.ini` `lib_deps` without explicit user confirmation. Check whether ESP-IDF or Arduino core already provides the capability.

---

## 4. Android Client Guidelines (Kotlin / Jetpack Compose)

1. **Use Compose Built-Ins**:
   - Use native Jetpack Compose / Material 3 components instead of importing third-party UI widgets or animation libraries.
   - Use standard Kotlin flows (`StateFlow`, `SharedFlow`) and coroutines instead of introducing additional reactive frameworks.
2. **Do Not Over-Architect ViewModels**:
   - Keep state hoisting simple and single-source-of-truth.
   - Avoid creating multiple intermediate mapper layers if data maps 1:1 between domain and UI.
3. **Gradle Dependencies**:
   - Do not add new dependencies to `app/build.gradle.kts` unless native AndroidX / Jetpack libraries cannot achieve the desired functionality.

---

## 5. Code Review Checklist for Agents

Before completing any task, verify:
- [ ] Did I add any file, class, or abstraction that isn't strictly required for this task?
- [ ] Could existing functions in `smartflow` have handled this?
- [ ] Did I pull in an external dependency when standard libraries already suffice?
- [ ] Is error handling, logging, and safety verification fully intact?
- [ ] Is the resulting git diff minimal, focused, and free of extraneous refactoring?
