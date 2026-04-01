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
      { src: "/logos/brandmark.svg", sizes: "72x72", type: "image/svg+xml", purpose: "any" },
      { src: "/logos/combinationmark.svg", sizes: "192x192", type: "image/svg+xml", purpose: "any" },
      { src: "/logos/wordmark.svg", sizes: "512x512", type: "image/svg+xml", purpose: "any" },
      { src: "/logos/wordmark.svg", sizes: "512x512", type: "image/svg+xml", purpose: "maskable" },
    ],
  };
}
