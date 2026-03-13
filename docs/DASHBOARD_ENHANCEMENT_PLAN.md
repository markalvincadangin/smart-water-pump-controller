## 9. Recent UX Review (March 2026)

This section captures a focused 2026 UX review of the current dashboard, with emphasis on first-load experience, perceived performance, and accessibility for real-time monitoring/control use cases.

### 9.1 Authentication & First-Load Experience

- **Finding**: Initial load currently shows two sequential blocking loaders (one in `DashboardPage` waiting on `authChecked`, and another in `AuthGuard` checking authorization), each potentially attaching its own auth listener.
- **Impact**: Users experience a “blank, spinner-only” screen for longer than necessary on first load, which feels slow and brittle for a critical control dashboard.
- **Recommendation**:
  - Consolidate auth/authorization into a single guard (ideally `AuthGuard`) and remove redundant auth effects from `page.tsx`.
  - Render the **shell** (`StatusBar`, `DashboardHeader`, main layout) immediately, with skeletons in place of live data, rather than a fully blank blocking loader.
  - (Optional) Cache the last-known safe snapshot locally (e.g. IndexedDB/localStorage) to show “Last known state as of HH:MM” while live data connects.

### 9.2 Progressive Loading & Perceived Performance

- **Finding**: Main content appears largely “all at once” after data is ready; `HistoryChart`, `ActivityPanel`, and some cards lack skeleton or staged states.
- **Impact**: On slower networks or cold starts, the UI can feel like it “pops in” suddenly, which is suboptimal for an always-on operational console.
- **Recommendation**:
  - Add lightweight skeletons for `DashboardMainGrid` (tank card, stat cards, mode controls), `HistoryChart`, and `ActivityPanel`.
  - Use `usePumpData` to progressively enable controls as soon as minimal status/control data is present, instead of gating entire sections behind a single loading flag.
  - Lean on existing `usePendingControl` to provide clear inline “in flight” and “confirmed” micro-feedback when changing modes (e.g. subtle color pulse on confirmation).

### 9.3 Modal UX & Accessibility

- **Finding**: `NotificationSettings` and `DeviceConfigSettings` modals are visually strong but lack explicit dialog semantics and robust focus handling.
- **Impact**: Keyboard and screen reader users may have difficulty understanding that a modal dialog has opened, and focus can escape behind the overlay.
- **Recommendation**:
  - Add `role="dialog"`, `aria-modal="true"`, and `aria-labelledby` pointing to the modal title.
  - Implement focus trapping within each modal and restore focus to the triggering control on close; support Esc to close.
  - Keep the existing Basic/Advanced split but reinforce task-based groupings (e.g. “Safe operating envelope”, “Alert timing”) and add small “What does this do?” tooltips on advanced fields.

### 9.4 Information Architecture & “Attention First” Design

- **Finding**: The current layout already follows a strong “bento” pattern (status → header → live cards → history → system info → activity), but urgent issues are not summarized in one place.
- **Impact**: Operators may scan multiple sections to understand what needs attention right now, rather than seeing a single, clear “attention” strip.
- **Recommendation**:
  - Introduce a compact **“Attention” summary band** near the top of `main` that aggregates pump errors, connectivity issues, and critical alerts into one, color-coded card.
  - Make time ranges and data windows explicit in `HistoryChart` (e.g. “Last 3 minutes”, or add simple 15m/1h/24h presets).
  - On large screens, consider placing Activity/System Info alongside History to keep “what just happened” in the same scan path as telemetry.

### 9.5 Status & Error Communication

- **Finding**: Connection issues are surfaced via a banner, but with minimal guidance; several secondary features (presence, audit log) fail silently by design.
- **Impact**: Users may not know whether to retry, wait, or check hardware; silent degradation can undermine trust in the dashboard.
- **Recommendation**:
  - Make the connection error banner actionable (Retry button, brief “If this persists >2 minutes…” helper text).
  - When presence or audit queries fail, show a small inline notice (“Activity temporarily unavailable. Commands still work; logs will resume when connection recovers.”).
  - Add a mild escalation when `updatedAt` is stale but the app is “connected” (e.g. amber state in `StatusBar`: “No telemetry received for 5+ minutes”).

### 9.6 Visual & Interaction Polish

- **Finding**: The dark/glassmorphism aesthetic, neon accents, and mono typography are well-aligned with a telemetry/control-room feel.
- **Impact**: The design is strong; most remaining gains are in subtle consistency and motion.
- **Recommendation**:
  - Standardize paddings and gaps across all cards/sections to reduce visual noise.
  - Use consistent accent color semantics (cyan for normal/live, amber for warnings, red for errors).
  - Add small, consistent hover/press animations (150–200ms) on high-impact controls (mode buttons, acknowledge, save) and short confirmation microcopy where actions directly affect hardware.

---

# Dashboard Frontend Enhancement Plan

**Date:** March 2026  
**Scope:** Dashboard and all frontend components (Next.js 14 App Router, React 18, Tailwind, Firebase)  
**Focus:** Issues, gaps, mistakes, inconsistencies, responsiveness, and mobile app (PWA) optimization

---

## Executive Summary

The Smart Water Pump dashboard is well-structured with a consistent dark theme, PWA support, and real-time Firebase integration. However, there are several inconsistencies in configuration handling, hardcoded values, redundant logic, and opportunities for better mobile/responsive UX. This plan documents all findings and provides actionable recommendations.

---

## 1. Issues, Gaps & Mistakes

### 1.1 Hardcoded Fallbacks (DRY Violations)

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `app/page.tsx` | `config?.pump_start_level ?? 30`, `config?.pump_stop_level ?? 100`, `config?.dry_run_threshold_lpm ?? 0.5` use inline magic numbers instead of `DEFAULT_DEVICE_CONFIG` | UI diverges from single source of truth; future default changes won't propagate |
| 2 | `app/page.tsx` | `dryRunTimeoutSec={config?.dry_run_timeout_sec ?? 30}` — same pattern | ModeControls shows correct value but duplicates default logic |
| 3 | `components/ModeControls.tsx` | Default param `dryRunTimeoutSec = 30` in function signature | Redundant with `DEFAULT_DEVICE_CONFIG.dry_run_timeout_sec` |

**Recommendation:** Import `DEFAULT_DEVICE_CONFIG` in `page.tsx` and use it for all fallbacks, e.g. `config?.pump_start_level ?? DEFAULT_DEVICE_CONFIG.pump_start_level`.
**Status:** Implemented ✅

---

### 1.2 Inaccurate Static Text

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `app/page.tsx` L275 | Chart subtext: `Last {history.length} readings · updated every 3s` | ESP32 uses `idle_firebase_interval_ms` (default 30s) when tank is full and pump off for 5+ min. Text is misleading during idle/sleep states |
| 2 | `lib/types.ts` L4 | Comment says "pushed every 3s" | Only accurate when pump is running; idle state pushes less frequently |

**Recommendation:** Make the chart label dynamic:
- When `snapshot?.status.is_sleeping` or pump idle for 5+ min: `updated every {config.idle_firebase_interval_ms / 1000}s when idle`
- When running: `updated every 3s`
- Or use generic: `Last {history.length} readings · real-time sync`
**Status:** Implemented ✅

---

### 1.3 Non-Idiomatic Next.js Routing

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `app/page.tsx` L147 | Sign-out uses `window.location.assign("/login")` | Full page reload; loses SPA transition, increases perceived latency |

**Recommendation:** Use `signOut().then(() => router.replace("/login"))` for a smoother client-side navigation.
**Status:** Implemented ✅

---

### 1.4 Redundant CSS & Logic

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `components/ModeControls.tsx` L76-77 | `touch-manipulation active:scale-[0.98]` duplicated in `clsx` string | Minor code bloat |
| 2 | `app/page.tsx` vs `AuthGuard.tsx` | Both implement auth redirect: `page.tsx` has `useEffect([authChecked, authUser])` → `router.replace("/login")`; AuthGuard has `onAuthStateChanged` → same redirect | Redundant; AuthGuard already prevents rendering children when unauthorized |

**Recommendation:** Remove the redundant auth redirect `useEffect` from `page.tsx` — AuthGuard is the single source of truth. Remove duplicate `touch-manipulation active:scale-[0.98]` from ModeControls.
**Status:** Implemented ✅

---

### 1.5 Notification Config — Missing Optimistic Update

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `lib/useNotificationConfig.ts` | `saveConfig` does not call `setConfig(merged)` after write | Unlike `useDeviceConfig`, UI does not reflect changes until RTDB listener fires; slight UX inconsistency |

**Recommendation:** Add `setConfig(merged)` after successful `await set(configRef, merged)` to match `useDeviceConfig` behavior.
**Status:** Implemented ✅

---

### 1.6 TypeScript & Typing Gaps

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `components/HistoryChart.tsx` L20 | `CustomTooltip` uses `any` for `payload`, `label` | Weak type safety |
| 2 | `components/HistoryChart.tsx` L26 | `payload.map((p: any) => ...)` | Same |

**Recommendation:** Use Recharts' `TooltipProps<T, V>` or a typed interface, e.g. `{ active?: boolean; payload?: Array<{ name: string; value: number; color?: string }>; label?: string }`.
**Status:** Implemented ✅

---

### 1.7 Breakpoint Mismatch (InfoTooltip)

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | `components/InfoTooltip.tsx` L61, L129, L147-148 | Uses `window.innerWidth < 640` and `>= 640` | Tailwind `sm` breakpoint is 640px — matches, but SSR/hydration: `window` is undefined on server; `typeof window !== "undefined"` guards exist, but initial render may flash |

**Recommendation:** Consider `useMediaQuery` hook with `matchMedia("(min-width: 640px)")` for consistent breakpoint handling and SSR safety. Or document that mobile overlay is intentionally client-only.
**Status:** Implemented ✅ (`useMediaQuery` + refactor in `InfoTooltip`)

---

### 1.8 Hardcoded UI Strings & Thresholds

| # | Location | Value | Notes |
|---|----------|-------|-------|
| 1 | `app/page.tsx` L114 | "Deep Well Pump · 660L Tank" | Tank capacity; could come from device config (tank_empty_cm, tank_full_cm) or a display name |
| 2 | `lib/usePumpData.ts` | `en-PH` | Time formatting locale — should be configurable or derived from user/browser |
| 3 | `lib/usePumpData.ts` | `MAX_HISTORY = 60` | History buffer length — consider making configurable |
| 4 | `components/TankVisual.tsx` L24-25 | `level <= 20` → amber, `level >= 90` → cyan | Hardcoded; could use `config.pump_start_level` for low and `config.pump_stop_level` for full |
| 5 | `components/NotificationSettings.tsx` L255 | `[10, 15, 20, 25, 30]` for low-level options | Could extend to `[5, 10, 15, 20, 25, 30, 40, 50]` for flexibility |
| 6 | `components/NotificationSettings.tsx` L285 | "15 minutes" alert throttling | Documented only; not configurable in UI |

---

### 1.9 Error Handling Gaps

| # | Location | Issue | Impact |
|---|----------|-------|--------|
| 1 | Project-wide | No React Error Boundary | Unhandled component errors can white-screen the app |
| 2 | `app/page.tsx` | Loading state uses generic spinner; no retry on auth/connection failure | Users cannot easily recover from transient failures |
| 3 | `lib/firebase.ts` | Placeholder env fallbacks (e.g. "YOUR_API_KEY") | Can mask misconfiguration in development |

---

## 2. Accessibility Gaps

| # | Area | Issue | Recommendation |
|---|------|-------|----------------|
| 1 | Charts | Recharts has limited keyboard support and screen reader semantics | Add `aria-label` describing chart data; consider `sr-only` summary of latest values |
| 2 | Navigation | No skip link | Add `<a href="#main" class="sr-only focus:not-sr-only">Skip to main content</a>` |
| 3 | Status changes | No live region for dynamic status (RUNNING → IDLE) | Add `aria-live="polite"` to pump status indicator for screen readers |
| 4 | Focus | Modal focus trapping | DeviceConfigSettings and NotificationSettings: ensure focus is trapped and Esc closes (partially done via InfoTooltip) |
| 5 | Language | `lang="en"` only | Consider `lang` attribute for i18n if localization is planned |

---

## 3. Responsiveness & Mobile Review

### 3.1 Strengths

- Safe area insets (`env(safe-area-inset-*)`) used in StatusBar, modals, InstallPrompt, login
- Touch targets: `min-h-[44px]` on critical buttons (WCAG 2.5.5)
- `touch-manipulation` on interactive elements to reduce 300ms tap delay
- `overscroll-contain` on modals to prevent scroll chaining
- Responsive grid: `grid-cols-1 md:grid-cols-2 lg:grid-cols-3`
- Modals: bottom sheet on mobile (`flex items-end`), centered on desktop
- Abbreviations: "ERR"/"RUN"/"IDLE" on mobile vs full text on larger screens
- `100dvh` for viewport height on mobile (addresses browser chrome)

### 3.2 Gaps & Recommendations

| # | Area | Issue | Recommendation |
|---|------|-------|----------------|
| 1 | Chart on small screens | Fixed heights `h-[180px] sm:h-[200px]` may feel cramped on very small phones | Consider `min-h-[140px] h-[ clamp(140px, 35vh, 220px)]` for better flexibility |
| 2 | HistoryChart X-axis | `tickInterval = data.length / 6` can produce dense labels on narrow viewports | Use `ResponsiveContainer` width to dynamically reduce tick count on small screens |
| 3 | StatusBar on very narrow | Badges (MODE, SENSOR, OVERFLOW, Sleep) can overflow | Add `flex-wrap` or `overflow-x-auto` with `-webkit-overflow-scrolling: touch` |
| 4 | InstallPrompt | Only shows on `beforeinstallprompt` (Chrome/Edge); iOS Safari never fires it | Add manual "Add to Home Screen" instructions for iOS users when in standalone-capable context |
| 5 | Header on mobile | Email hidden (`hidden md:block`); Settings/Bell/Sign out + status can crowd | Consider hamburger or compact icon-only layout; ensure all actions remain reachable |

---

## 4. Visual Design, Interactivity & UX Enhancements

### 4.1 Visual Hierarchy, Layout & Typography

- **Primary state card**: Introduce a single, prominent **system status card** at the top (e.g. “System healthy”, “Check tank soon”, “Immediate attention required”), using consistent iconography and color (green / amber / red) so users instantly understand overall health.
- **Consistent sectioning**: Organize the page into clear sections with headings like **Live status**, **Tank & flow**, **Controls**, **History & insights**, **Alerts & safety**; use consistent heading sizes and spacing to improve scan-ability.
- **Spacing & density**: Slightly increase vertical spacing between rows on desktop and ensure a mostly **single-column flow on mobile** (Status → Tank → Stats → Controls → History → Alerts).
- **Typography consistency**: Define a small, explicit **type scale** (e.g. `text-xs/sm/base/lg/xl`) and apply it consistently across the dashboard (headers, cards, modals, tooltips). Avoid mixing many font sizes/weights for similar elements so the hierarchy is clearer at a glance.

### 4.2 Controls & Advanced Features

- **Basic vs advanced settings**: In DeviceConfigSettings and NotificationSettings, add a **“Basic / Advanced” toggle**. Basic shows the most important fields (start/stop levels, low-water alerts). Advanced reveals calibration, sleep schedule, idle intervals, sensor thresholds, etc.
- **Safer manual & timed runs (aligned with Phase 7 firmware plan)**:
  - Introduce a dedicated **“Run pump”** control group, separate from the `AUTO / FORCE_ON / FORCE_OFF` mode selector.
  - Provide two primary actions:
    - **Quick start (Manual)** — sends a `manual_start` control flag; the firmware treats this as “run until I press Stop”, with all safety checks still active.
    - **Timed run…** — opens a small dialog to pick a duration (e.g. 5 / 10 / 15 / 30 / 60 minutes), then writes a `timed_start_sec` value. The ESP32 auto-stops at the end or earlier on safety fault.
  - Show a **Stop** button whenever `run_mode` is `"MANUAL"` or `"TIMED"`, wired to a `manual_stop` control flag.
  - Display `run_mode` and `run_remaining_sec` from status next to the controls (e.g. “Run mode: Timed · Auto-stop in 09:32”).
  - When an error is active, show a short explanation and (for admins) an extra confirmation step before allowing a new manual/timed run.
- **Mode explanations**: Under ModeControls (or via InfoTooltip), show short, plain-English explanations for AUTO, FORCE_ON and FORCE_OFF, optimized for non-technical users.
- **Troubleshooting helpers**: For error banners and dry-run lockouts, add a small **“Troubleshoot”** link that opens a short checklist (check power, valve, sensor wiring) and links to deeper docs where relevant.

### 4.3 Interactivity & Feedback

- **Toasts/snackbars**: After actions like saving device/notification config or changing mode, show a small toast (“Settings saved and sent to pump”, “Mode changed to AUTO”) that auto-dismisses.
- **Command in-flight states**: When ModeControls sends a new mode, briefly show a “Sending…” state until the next confirmed snapshot arrives (with a timeout and error message if confirmation never comes).
- **Chart interaction**:
  - Enable tapping/hovering chart points to show a small popover with time, level, flow and mode at that point.
  - Optionally add simple range presets above the chart (Last 1h / 6h / 24h / 7d) to filter history arrays.

### 4.4 Notifications UX

- **First-time setup flow**: For new users, introduce a short, optional **onboarding** sequence:
  1. Confirm or adjust basic tank/pump defaults.
  2. Choose alert channels (email, push).
  3. Offer to install the app on mobile.
- **Alert summary**: At the top of NotificationSettings, show a human-readable summary, e.g. “You’ll be alerted by **email** when: pump runs dry, water is low, pump runs too long.”
- **Test alerts**: Add a **“Send test alert”** action so users can easily verify that email and/or push notifications are working.

### 4.5 Branding, Logo & Icon Consistency

- **Single source of truth**: Align the `Logo` component, `favicon.svg`, PWA icons, and any future splash screens so they share the same drop shape, proportions, and color palette.
- **Usage guidelines**:
  - Use `Logo` in the header, login page, and modal headers instead of ad-hoc icons to reinforce brand identity.
  - Document where to use the **rounded-square icon** (app icon, favicon, home-screen) vs. the bare drop (inline branding).
- **Accessibility**: Ensure that when the logo is the primary brand text, it has an accessible name (e.g. `aria-label="Smart Water Pump System"` or adjacent text), and is only `aria-hidden` when purely decorative.

### 4.6 Ongoing UI/UX Research & Evaluation

- **Heuristic review**: Periodically review the dashboard against standard usability heuristics (visibility of system status, match with real-world language, error prevention, recognition vs recall) and log findings back into this plan.
- **Role-based walkthroughs**: Do separate end-to-end walkthroughs as **admin** and as **standard user** to confirm the layout, wording, and visible controls match each role’s mental model (no confusing “admin-only” noise for non-admins).
- **User feedback loop**: When possible, collect quick feedback from real users (or yourself on different devices/browsers) about clarity of labels, readability, and ease of finding common actions, and iteratively refine typography, layout, and copy.

### 4.7 Navigation, Discoverability & Information Architecture

- **Entry points for settings**: Ensure that key actions (mode control, device config, notifications) are clearly labeled and visually grouped, so first-time users know where to go for “change how the pump behaves” vs “change how I get alerts”.
- **Scannable dashboard**: Aim for a top-to-bottom storyline: **“Is everything OK?” → “What’s the tank doing now?” → “What happened recently?” → “Do I need to change anything?”**. Re-order or visually emphasize sections to support this mental flow.
- **Progressive disclosure**: Hide rarely used or expert-only controls behind secondary affordances (Advanced, “More details”, collapsible sections) to keep the main view calm and focused.

### 4.8 States, Errors & Empty States

- **Loading/skeleton states**: Where possible, replace generic spinners with lightweight skeletons or placeholders that match the final layout (for StatusBar, cards, chart) so the page feels more stable while data is loading.
- **Empty history & first-run**: When there is little or no history yet, show an explanation (e.g. “History will appear here after the pump has run for a while”) instead of an almost-empty chart.
- **Inline validation & helper text**: For configuration fields (levels, thresholds, timeouts), show inline guidance and validation (e.g. “Stop level must be higher than start level”) to prevent invalid combinations before users hit Save.
- **Clear recovery paths**: For error banners and connection issues, always pair the status text with a concrete next step (e.g. “Retry”, “Check Wi-Fi on the controller”, “Open Troubleshoot guide”).

---

## 5. PWA & Mobile App Enhancements

| # | Area | Issue | Recommendation |
|---|------|-------|----------------|
| 1 | `app/layout.tsx` | Missing `themeColor` in metadata | Add `metadata.themeColor = "#0A0E1A"` (Next.js 14.2+ supports `themeColor` in Metadata) |
| 2 | `app/layout.tsx` | Missing `appleWebApp` settings | Add `appleWebApp: { capable: true, statusBarStyle: "black-translucent" }` for iOS PWA feel |
| 3 | Manifest | Already has `theme_color`, `background_color` | Confirm `manifest.ts` is wired in layout (Next.js auto-injects when using `manifest()` in app dir) |
| 4 | Offline | PWA caches assets; Firebase RTDB requires network | Consider offline placeholder or cached last snapshot for resilience |
| 5 | InstallPrompt | Only Chrome/Edge support `beforeinstallprompt` | Keep InstallPrompt for supported browsers; add iOS-specific “Add to Home Screen” instructions when running in Safari |
| 6 | Notification deep links | Notifications land on generic dashboard | Add `click_action`/URL in notification payloads and handle `notificationclick` in the service worker to route directly to relevant sections (e.g. open controls+errors for dry-run alerts) |

---

## 6. Role-Based Views & Concurrent Users

### 6.1 Role-Based Access (Admin vs Standard User)

- **Roles**:
  - **Admin (you)**: Full access to all configuration, advanced features (sleep schedule, calibration, safety thresholds), and diagnostic tools.
  - **Standard user**: Focused view with only essential information and safe controls (e.g. viewing status, switching between AUTO / FORCE_OFF, basic alerts), no access to dangerous calibration parameters.
- **Implementation outline**:
  - Introduce a simple role concept in auth (e.g. via a small `/roles/{uid}` node or `customClaims`) and expose it to the dashboard through an auth hook.
  - Implement **conditional rendering** of advanced sections in DeviceConfigSettings and NotificationSettings based on role.
  - In the UI, avoid cluttering the standard user experience with advanced fields; expose them only when the user is an admin.
  - For non-admin users, **prefer hiding** Advanced settings entirely instead of showing disabled controls with “Advanced settings are admin-only” on every advanced field; where needed, use a single, subtle note in the settings entry point.

### 6.2 Concurrent Users & Multi-Session Behavior

- **Simultaneous access**:
  - Ensure the dashboard explicitly **supports concurrent sessions** (multiple users/devices viewing and controlling the same pump).
  - Clarify how last-write-wins is handled: when two users change config at the same time, Firebase RTDB’s latest write wins; the UI should always reflect the live database state via listeners.
- **Conflict minimization**:
  - For potentially conflicting actions (e.g. two users switching modes rapidly), consider lightweight **rate limiting** on control writes or brief “cooldown” indicators.
  - Optionally log who changed what and when (event history) so admins can diagnose unexpected behavior.
- **Visibility**:
  - (Optional) Show a small indicator like “2 users currently viewing” using a lightweight presence mechanism if needed in future phases.
**Status:** Implemented ✅ (admin-only FORCE_ON, Basic/Advanced gating, presence indicator + audit + activity feed)

---

## 7. Work Execution Plan

### Phase A — Quick Wins (1–2 hours)

1. **page.tsx**
   - Replace hardcoded defaults with `DEFAULT_DEVICE_CONFIG`
   - Change sign-out to `router.replace("/login")`
   - Remove redundant auth redirect `useEffect` (let AuthGuard handle it)
   - Quick sanity check: ensure `DEFAULT_DEVICE_CONFIG` matches ESP32 firmware offline defaults (e.g. `firmware/smart_pump_controller/smart_pump_controller.ino`) so dashboard “defaults” reflect real device behavior when booted offline
2. **ModeControls.tsx** — Remove duplicate `touch-manipulation active:scale-[0.98]`
3. **useNotificationConfig.ts** — Add optimistic `setConfig(merged)` in `saveConfig`
4. **layout.tsx** — Add `themeColor` and `appleWebApp` to metadata

### Phase B — Accuracy & UX (2–3 hours)

5. **page.tsx** — Dynamic chart label (idle vs running update interval)
6. **HistoryChart.tsx** — Replace `any` with proper Tooltip types
7. **TankVisual.tsx** — Optionally use `config.pump_start_level` / `config.pump_stop_level` for color thresholds (or keep hardcoded if UX prefers consistency)
8. **Firebase RTDB rules review** — Verify database rules enforce that users can only read/write their own notification preferences and `fcmTokens` (e.g. `/pump_system/config/notifications_by_user/{uid}` locked to `auth.uid == uid`) before relying on optimistic updates (**Implemented ✅**; updated `database.rules.json` for `presence` + `audit` + admin write checks)

### Phase C — Resilience & Polish (2–4 hours)

9. **Error Boundary** — Add `app/error.tsx` and `app/global-error.tsx` for graceful error handling (**Implemented ✅**)
10. **InfoTooltip** — Add `useMediaQuery` or document SSR behavior (**Implemented ✅**)
11. **Accessibility** — Skip link, aria-live, chart labels (**Implemented ✅** for skip link + aria-live; chart labels optional/pending if desired)
12. **InstallPrompt** — iOS fallback: show "Add to Home Screen" instructions when `!window.matchMedia('(display-mode: standalone)').matches` and user agent suggests iOS (**Implemented ✅**)

### Phase D — Optional Enhancements

13. Tank label "660L Tank" — Move label into configuration so it's not hardcoded in the component
    - **Implemented ✅** via `NEXT_PUBLIC_TANK_LABEL` env var used in `DashboardHeader`. Default remains `"Deep Well Pump · 660L Tank"` when the env var is not set. This keeps firmware config schema unchanged while allowing per-deployment customization.
14. Locale — Use `navigator.language` or config for date/time formatting
    - **Implemented ✅**: `usePumpData` now derives a `timeLocale` from `window.navigator.language` (with `"en-PH"` as a safe default) for chart timestamps.
15. Chart tick interval — Responsive based on container width
    - **Implemented ✅**: `HistoryChart` uses a `ResizeObserver` + container width to choose tick density for small vs large viewports.
16. Role-based views — Introduce admin vs standard user roles and gate advanced settings accordingly
    - **Implemented ✅** (see Section 6): admin vs standard users are differentiated via `/pump_system/config/admins/{uid}` and reflected in both Firebase rules and UI (Advanced tabs, FORCE ON, etc.).
17. Concurrent users — Document and (optionally) visualize presence or last-change info for multi-user scenarios
    - **Implemented ✅**: Online presence and recent actions are surfaced via `usePresence`, `useAuditEvents`, and `ActivityPanel`; rules and docs describe concurrent usage behavior.

### Phase E — Validation & QA (30–60 minutes)

18. **Responsive validation** — Use Chrome DevTools (Device Mode) to verify breakpoints and overflow behavior for StatusBar, header controls, modals, and HistoryChart labels/ticks across common widths (e.g. 360×800, 390×844, 768×1024).
19. **PWA validation** — Run a Lighthouse audit to confirm PWA metadata is detected and correct (themeColor, appleWebApp behavior, manifest icons). Verify install behavior on Chrome/Edge and iOS Safari fallback instructions.

**Status:** Pending manual verification (requires running the app + Lighthouse).

---

## 8. Summary Checklist

| Category | Items |
|----------|-------|
| **Hardcoded values** | Use DEFAULT_DEVICE_CONFIG; dynamic chart label; TankVisual thresholds |
| **Routing** | router.replace on sign-out |
| **DRY** | Remove duplicate auth logic; ModeControls CSS |
| **Optimistic updates** | useNotificationConfig.saveConfig |
| **TypeScript** | HistoryChart CustomTooltip typing |
| **PWA / Mobile app** | themeColor, appleWebApp in layout; iOS install guidance; notification deep links |
| **Accessibility** | Skip link, aria-live, chart labels |
| **Mobile UX** | Chart height flexibility; StatusBar overflow; iOS install instructions; single-column flow |
| **UX / Interactivity** | Toasts, command in-flight states, mode explanations, troubleshooting helpers |
| **Branding** | Consistent Logo, favicon and PWA icons |
| **Roles & users** | Admin vs standard role behavior; concurrent usage considerations |
| **Error handling** | Error boundaries; retry UX |

---

*Plan version 3 — March 2026*
