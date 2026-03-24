import type { Config } from "tailwindcss";

const config: Config = {
  darkMode: ["selector", '[data-theme="dark"]'],
  content: [
    "./pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      fontSize: {
        /** SmartFlow type scale (§2.2) */
        body: ["0.9375rem", { lineHeight: "1.6" }], /* 15px */
        code: ["0.8125rem", { lineHeight: "1.6" }], /* 13px — event log, diagnostics */
        title: ["1.125rem", { lineHeight: "1.3", fontWeight: "600" }], /* 18px card title */
        "sub-metric": ["1.5rem", { lineHeight: "1", fontWeight: "500" }], /* 24px — secondary readings */
        metric: ["2rem", { lineHeight: "1", fontWeight: "600" }], /* 32px */
        hero: ["3rem", { lineHeight: "1", fontWeight: "600" }], /* 48px */
      },
      fontFamily: {
        display: ["var(--font-ui)", "system-ui", "sans-serif"],
        mono: ["var(--font-data)", "ui-monospace", "monospace"],
        body: ["var(--font-ui)", "system-ui", "sans-serif"],
        data: ["var(--font-data)", "ui-monospace", "monospace"],
      },
      colors: {
        canvas: "rgb(var(--c-bg-canvas) / <alpha-value>)",
        shell: "rgb(var(--c-bg-base) / <alpha-value>)",
        elevated: "rgb(var(--c-bg-elevated) / <alpha-value>)",
        inset: "rgb(var(--c-bg-inset) / <alpha-value>)",
        surface: {
          DEFAULT: "rgb(var(--c-bg-canvas) / <alpha-value>)",
          1: "rgb(var(--c-bg-surface) / <alpha-value>)",
          2: "rgb(var(--c-bg-raised) / <alpha-value>)",
          3: "rgb(var(--c-border-subtle) / <alpha-value>)",
          4: "rgb(var(--c-border-default) / <alpha-value>)",
        },
        border: {
          faint: "rgb(var(--c-border-faint) / <alpha-value>)",
          subtle: "rgb(var(--c-border-subtle) / <alpha-value>)",
          DEFAULT: "rgb(var(--c-border-default) / <alpha-value>)",
          strong: "rgb(var(--c-border-strong) / <alpha-value>)",
          focus: "rgb(var(--c-border-focus) / <alpha-value>)",
        },
        accent: {
          cyan: "rgb(var(--c-brand-500) / <alpha-value>)",
          green: "rgb(var(--c-status-ok) / <alpha-value>)",
          amber: "rgb(var(--c-status-warn) / <alpha-value>)",
          red: "rgb(var(--c-status-error) / <alpha-value>)",
        },
        text: {
          primary: "rgb(var(--c-text-primary) / <alpha-value>)",
          secondary: "rgb(var(--c-text-secondary) / <alpha-value>)",
          muted: "rgb(var(--c-text-tertiary) / <alpha-value>)",
          data: "rgb(var(--c-text-data) / <alpha-value>)",
          unit: "rgb(var(--c-text-unit) / <alpha-value>)",
          link: "rgb(var(--c-text-link) / <alpha-value>)",
        },
      },
      animation: {
        "pulse-slow": "pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite",
        "spin-slow": "spin 8s linear infinite",
        "fade-in": "fadeIn 0.6s ease forwards",
        "slide-up": "slideUp 0.5s ease forwards",
        "glow-pulse": "glowPulse 2s ease-in-out infinite",
        "tank-wave": "tankWave 3s ease-in-out infinite",
        shimmer: "shimmerSweep 1.5s linear infinite", /* §8.3 */
      },
      keyframes: {
        fadeIn: {
          from: { opacity: "0" },
          to: { opacity: "1" },
        },
        slideUp: {
          from: { opacity: "0", transform: "translateY(20px)" },
          to: { opacity: "1", transform: "translateY(0)" },
        },
        glowPulse: {
          "0%, 100%": { boxShadow: "0 0 20px rgb(var(--c-brand-500) / 0.3)" },
          "50%": { boxShadow: "0 0 40px rgb(var(--c-brand-500) / 0.7)" },
        },
        tankWave: {
          "0%, 100%": { transform: "translateX(0)" },
          "50%": { transform: "translateX(3px)" },
        },
      },
    },
  },
  plugins: [],
};

export default config;
