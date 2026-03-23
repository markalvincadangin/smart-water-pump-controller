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
    ],
    apple: [
      { url: "/apple-icon.png", sizes: "180x180", type: "image/png" },
    ],
  },
};

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  maximumScale: 5,
  viewportFit: "cover",
  themeColor: "#0A0E14",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className={`${GeistSans.variable} ${GeistMono.variable}`}>
      <body className="bg-surface text-text-primary font-body antialiased overflow-x-hidden">
        <ThemeProvider attribute="data-theme" defaultTheme="dark" enableSystem={false}>
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
