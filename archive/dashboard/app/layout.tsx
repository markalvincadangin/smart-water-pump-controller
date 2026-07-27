// app/layout.tsx
import type { Metadata, Viewport } from "next";
import { GeistSans } from "geist/font/sans";
import { GeistMono } from "geist/font/mono";
import "./globals.css";
import ToastHost from "@/components/ToastHost";
import { ThemeProvider } from "@/components/ThemeProvider";

export const metadata: Metadata = {
  title: "SmartFlow — Water Management Dashboard",
  description: "Real-time water system monitoring and pump control with safety lockouts, alerts, and remote diagnostics.",
  appleWebApp: {
    capable: true,
    statusBarStyle: "black-translucent",
    title: "SmartFlow",
  },
  icons: {
    icon: [
      { url: "/favicon.ico", type: "image/x-icon" },
      { url: "/icons/icon-192.png", sizes: "192x192", type: "image/png" },
      { url: "/icons/icon-512.png", sizes: "512x512", type: "image/png" },
    ],
    apple: [
      { url: "/icons/apple-touch-icon.png", sizes: "180x180", type: "image/png" },
    ],
  },
};

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  maximumScale: 5,
  viewportFit: "cover",
  themeColor: "#185FA5", /* SmartFlow brand color (Phase 6) */
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className={`${GeistSans.variable} ${GeistMono.variable}`} suppressHydrationWarning
      style={{ 
        // Force the variable names expected by tailwind.config.ts
        "--font-geist": GeistSans.style.fontFamily,
        "--font-geist-mono": GeistMono.style.fontFamily 
      } as React.CSSProperties}>
      <body className="bg-canvas text-text-primary font-sans antialiased overflow-x-hidden" suppressHydrationWarning>
        <ThemeProvider attribute="data-theme">
          <a
            href="#main"
            className="sr-only focus:not-sr-only focus:fixed focus:top-3 focus:left-3 focus:z-[9999]
                     focus:px-3 focus:py-2 focus:rounded-lg focus:bg-surface-2 focus:border focus:border-surface-4
                     focus:text-text-primary focus:font-mono focus:text-xs"
          >
            Skip to main content
          </a>
          <ToastHost />
          {children}
        </ThemeProvider>
      </body>
    </html>
  );
}
