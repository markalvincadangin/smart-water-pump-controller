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

### 4.1 Visual Hierarchy & Layout

- **Primary state card**: Introduce a single, prominent **system status card** at the top (e.g. “System healthy”, “Check tank soon”, “Immediate attention required”), using consistent iconography and color (green / amber / red) so users instantly understand overall health.
- **Consistent sectioning**: Organize the page into clear sections with headings like **Live status**, **Tank & flow**, **Controls**, **History & insights**, **Alerts & safety**; use consistent heading sizes and spacing to improve scan-ability.
- **Spacing & density**: Slightly increase vertical spacing between rows on desktop and ensure a mostly **single-column flow on mobile** (Status → Tank → Stats → Controls → History → Alerts).

### 4.2 Controls & Advanced Features

- **Basic vs advanced settings**: In DeviceConfigSettings and NotificationSettings, add a **“Basic / Advanced” toggle**. Basic shows the most important fields (start/stop levels, low-water alerts). Advanced reveals calibration, sleep schedule, idle intervals, sensor thresholds, etc.
- **Safer FORCE_ON**:
  - Add an optional **“Run for…” timeout** (e.g. 10 / 20 / 30 minutes) when FORCE_ON is selected, with automatic return to AUTO afterwards.
  - When an error is active, show a **confirmation dialog** before allowing FORCE_ON: clearly state the risk of running the pump against errors.
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

13. Tank label "660L Tank" — Derive from config or add display name in DeviceConfig
14. Locale — Use `navigator.language` or config for date/time formatting
15. Chart tick interval — Responsive based on container width
16. Role-based views — Introduce admin vs standard user roles and gate advanced settings accordingly
17. Concurrent users — Document and (optionally) visualize presence or last-change info for multi-user scenarios

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
