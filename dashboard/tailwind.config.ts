import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      fontFamily: {
        display: ["var(--font-ui)", "system-ui", "sans-serif"],
        mono: ["var(--font-data)", "monospace"],
        body: ["var(--font-ui)", "system-ui", "sans-serif"],
      },
      colors: {
        surface: {
          DEFAULT: "rgb(var(--c-bg-base) / <alpha-value>)",
          1: "rgb(var(--c-bg-surface) / <alpha-value>)",
          2: "rgb(var(--c-bg-elevated) / <alpha-value>)",
          3: "rgb(var(--c-border-subtle) / <alpha-value>)",
          4: "rgb(var(--c-border-default) / <alpha-value>)",
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
        },
      },
      animation: {
        "pulse-slow": "pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite",
        "spin-slow": "spin 8s linear infinite",
        "fade-in": "fadeIn 0.6s ease forwards",
        "slide-up": "slideUp 0.5s ease forwards",
        "glow-pulse": "glowPulse 2s ease-in-out infinite",
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
      },
    },
  },
  plugins: [],
};

export default config;
