import type { MetadataRoute } from "next";

/**
 * PWA manifest — enables "Add to Home Screen" / installable app.
 * Users can install the dashboard like YouTube, Facebook, etc.
 */
export default function manifest(): MetadataRoute.Manifest {
  return {
    name: "SmartFlow",
    short_name: "SmartFlow",
    description: "Water, managed. Real-time monitoring and pump control with safety-first automation.",
    start_url: "/",
    display: "standalone",
    background_color: "#F1EFE8",
    theme_color: "#185FA5", /* SmartFlow brand color (Phase 6) */
    orientation: "any",
    icons: [
      { src: "/icons/icon-192.png", sizes: "192x192", type: "image/png", purpose: "any" },
      { src: "/icons/icon-512.png", sizes: "512x512", type: "image/png", purpose: "any" },
      { src: "/icons/icon-192-maskable.png", sizes: "192x192", type: "image/png", purpose: "maskable" },
      { src: "/icons/icon-512-maskable.png", sizes: "512x512", type: "image/png", purpose: "maskable" },
    ],
  };
}
