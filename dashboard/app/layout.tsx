// app/layout.tsx
import type { Metadata, Viewport } from "next";
import { Syne, DM_Sans, JetBrains_Mono } from "next/font/google";
import "./globals.css";
import ToastHost from "@/components/ToastHost";

const syne = Syne({
  subsets: ["latin"],
  variable: "--font-syne",
  weight: ["400", "600", "700", "800"],
});

const dmSans = DM_Sans({
  subsets: ["latin"],
  variable: "--font-dm-sans",
  weight: ["300", "400", "500"],
});

const jetbrainsMono = JetBrains_Mono({
  subsets: ["latin"],
  variable: "--font-jetbrains",
  weight: ["300", "400", "500"],
});

export const metadata: Metadata = {
  title: "Smart Water Pump System — Control Dashboard",
  description: "Real-time monitoring and control for the Smart Water Pump System. Deep well pump with dry-run protection and cloud connectivity.",
  appleWebApp: {
    capable: true,
    statusBarStyle: "black-translucent",
    title: "Smart Water Pump",
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
  themeColor: "#0A0E1A",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className={`${syne.variable} ${dmSans.variable} ${jetbrainsMono.variable}`}>
      <body className="bg-surface text-text-primary font-body antialiased overflow-x-hidden">
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
      </body>
    </html>
  );
}
