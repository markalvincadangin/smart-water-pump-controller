"use client";

import * as React from "react";
import { Moon, Sun } from "lucide-react";
import { useTheme } from "next-themes";

/** Part 11.2 — tooltips; toggles resolved light/dark (works with system default). */
export function ThemeToggle() {
  const { resolvedTheme, setTheme } = useTheme();
  const [mounted, setMounted] = React.useState(false);

  React.useEffect(() => {
    setMounted(true);
  }, []);

  if (!mounted) {
    return <div className="h-9 w-9 rounded-lg border border-surface-3 bg-surface-2" />;
  }

  const isDark = resolvedTheme === "dark";
  const next = isDark ? "light" : "dark";
  const title = isDark ? "Switch to light mode" : "Switch to dark mode";

  return (
    <button
      type="button"
      title={title}
      onClick={() => setTheme(next)}
      className="relative inline-flex h-9 w-9 items-center justify-center rounded-lg border border-surface-3 bg-surface-2 text-text-secondary transition-colors duration-200 ease-out hover:border-accent-cyan/40 hover:text-accent-cyan focus-visible:outline-none focus-visible:ring-[3px] focus-visible:ring-[rgb(var(--c-border-focus)/0.45)]"
      aria-label={title}
    >
      <Sun className="h-[1.2rem] w-[1.2rem] rotate-0 scale-100 transition-transform duration-200 dark:-rotate-90 dark:scale-0" />
      <Moon className="absolute h-[1.2rem] w-[1.2rem] rotate-90 scale-0 transition-transform duration-200 dark:rotate-0 dark:scale-100" />
    </button>
  );
}
