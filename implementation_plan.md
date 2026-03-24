# Minimalist Dashboard Polish — Research-Backed Plan

## Research Sources

| Source | Key Principle Applied |
|--------|----------------------|
| **Vercel Geist** | Crisp 1px borders, layered subtle shadows, 3-state border system (default → hover → active), type hierarchy via Geist Sans/Mono |
| **Nike Podium** | Whitespace as "breathing room", elimination of superfluous decoration, 8pt spacing grid, minimal marketing noise |
| **NN/g Dashboard UX** | F-pattern scanning, progressive disclosure, visibility of system status, cognitive load minimization |
| **Fitts's Law** | Touch targets ≥44px (already compliant); interactive elements near natural focus |
| **Hick's Law** | Fewer simultaneous choices = faster decisions; avoid "analysis paralysis" |
| **Miller's Law** | 7±2 chunks; group related data with clear visual separators |
| **SmartFlow Spec** | Remains the authority — all changes stay within its token/layout boundaries |

![Current dashboard at desktop width](file:///C:/Users/markc/.gemini/antigravity/brain/ec79fa7c-1178-4d3c-a607-1c096a82d179/.system_generated/click_feedback/click_feedback_1774310127943.png)

---

## Proposed Changes

### 1. Card Glows → Subtle Borders

#### [MODIFY] [globals.css](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/app/globals.css)

**Why:** Vercel Geist uses crisp 1px ring borders only — no colored spread shadows. Nike eliminates decorative noise. The current neon glows are purely decorative, violating the spec's "no color is purely decorative" rule.

- `card-glow-cyan`: Remove spread shadow + inset shine → just `0 0 0 1px brand/0.15`
- `card-glow-green`: Same treatment → `0 0 0 1px ok/0.18`
- `card-glow-red`: Same treatment → `0 0 0 1px error/0.25`
- Tank glass panel: Reduce shadow from `0.45/0.35` → `0.25/0.15` (Vercel: "subtle shadows that signal importance without cluttering")
- Remove unused `scanline` keyframes (dead code after TankVisual cleanup)

---

### 2. Header Buttons — Vercel 3-State Borders

#### [MODIFY] [DashboardHeader.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/DashboardHeader.tsx)

**Why:** Vercel's Geist defines default/hover/active border colors. Currently all buttons have visible `border-surface-3` at rest — Vercel makes them `transparent` at rest and reveals on hover. Nike: "minimize noise to focus on content".

- Desktop icon buttons: `border-surface-3` → `border-transparent hover:border-[rgb(var(--c-border-subtle))]`
- Tighten button gap: `gap-2 sm:gap-3` → `gap-1.5 sm:gap-2` (Nike 8pt grid alignment)

---

### 3. Stat Cards — Remove Decorative Glow

#### [MODIFY] [StatCard.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/StatCard.tsx)

**Why:** NN/g: "each panel displays only information required for its purpose." Card glows signal status semantics but are decorative on informational tiles. Vercel cards: flat surface + 1px border.

- Remove `c.glow` from card wrapper → plain `.card`
- Tighten icon well: `rounded-lg p-2` → `rounded-md p-1.5` (Nike: tighter, crisper geometric forms)

---

### 4. Toast — Consistent Left Accent

#### [MODIFY] [ToastHost.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/ToastHost.tsx)

**Why:** Miller's Law chunking — toasts and alert cards should share the same visual language for consistent system status communication. NN/g: "consistency reduces cognitive effort."

- `rounded-xl` → `rounded-lg` (8px — spec Radius Medium for interactive elements)
- Add `border-l-[3px]` semantic left accent to match alert card treatment

---

### 5. History Section — De-decorate

#### [MODIFY] [DashboardHistorySection.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/DashboardHistorySection.tsx)

**Why:** History is informational, not status-critical. Vercel: colored decoration reserved for semantics only. Nike: "no superfluous elements."

- Remove `card-glow-cyan` → plain `.card`

---

### 6. Collapsible Section — Spec-Aligned Tracking

#### [MODIFY] [CollapsibleSection.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/components/CollapsibleSection.tsx)

**Why:** SmartFlow spec §2.3 defines section heading as `0.10em` tracking. Currently using `tracking-widest` (0.1em) which is correct but Tailwind's `tracking-widest` is actually `0.1em` — this is already correct. However, the title uses `uppercase tracking-widest` which looks like 0.1em. Let me verify — Tailwind `tracking-widest` = `0.1em`. This is actually already correct per spec.

- **No change needed** — already spec-compliant.

---

### 7. Login Page — Flat, Confident

#### [MODIFY] [login/page.tsx](file:///c:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/dashboard/app/login/page.tsx)

**Why:** Nike: "white space for breathing room + focus on primary action." Vercel: flat canvas + crisp card. The current `card-glow-cyan` + `bg-surface` creates a soft, decorative feel.

- Remove `card-glow-cyan` → plain `.card`  
- `bg-surface` → `bg-canvas` (true flat background per spec §1.2)
- Tighten padding: `p-6 sm:p-10` → `p-8` (Nike: controlled, confident spacing)

---

## Verification Plan

### Automated
```bash
npm run build  # No compile errors
npm test       # 18/18 existing tests pass
```

### Visual (browser check)
1. Cards: clean 1px ring borders, no neon glow
2. Header buttons: borderless at rest, border appears on hover
3. Stat cards: flat, no colored halo
4. Toasts: left accent stripe, 8px radius consistent with alerts
5. History: plain card, no glow
6. Login: flat dark canvas, crisp card, focused layout
