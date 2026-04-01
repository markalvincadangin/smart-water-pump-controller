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
      colors: {
        sf: {
          blue:         '#185FA5',
          'blue-mid':   '#378ADD',
          'blue-light': '#E6F1FB',
          'blue-dark':  '#0C447C',
          teal:         '#0F6E56',
          'teal-light': '#E1F5EE',
          'teal-dark':  '#085041',
          amber:        '#BA7517',
          'amber-light':'#FAEEDA',
          'amber-dark': '#854F0B',
          red:          '#A32D2D',
          'red-light':  '#FCEBEB',
          'red-dark':   '#791F1F',
          green:        '#3B6D11',
          'green-light':'#EAF3DE',
          gray: {
            50:  '#F1EFE8',
            100: '#D3D1C7',
            200: '#B4B2A9',
            400: '#888780',
            600: '#5F5E5A',
            900: '#2C2C2A',
          },
        },
        /* Legacy compat — mapped to sf tokens where possible */
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
      fontFamily: {
        sans: ['var(--font-geist)', 'system-ui', 'sans-serif'],
        mono: ['var(--font-geist-mono)', 'ui-monospace', 'monospace'],
      },
      borderRadius: {
        card: "12px",
        chip: "6px",
      },
      boxShadow: {
        card: "0 1px 3px 0 rgb(0 0 0 / 0.04), 0 1px 2px -1px rgb(0 0 0 / 0.04)",
        "card-hover": "0 4px 6px -1px rgb(0 0 0 / 0.07)",
      },
      animation: {
        "pulse-slow": "pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite",
        "spin-slow": "spin 8s linear infinite",
        "fade-in": "fadeIn 0.6s ease forwards",
        "slide-up": "slideUp 0.5s ease forwards",
        "glow-pulse": "glowPulse 2s ease-in-out infinite",
        "tank-wave": "tankWave 3s ease-in-out infinite",
        shimmer: "shimmerSweep 1.5s linear infinite",
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
        shimmerSweep: {
          "0%": { transform: "translateX(-100%)" },
          "100%": { transform: "translateX(100%)" },
        },
      },
    },
  },
  plugins: [],
};

export default config;
