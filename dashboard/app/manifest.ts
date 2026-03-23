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
    background_color: "#0a0e14",
    theme_color: "#0A0E14",
    orientation: "any",
    icons: [
      { src: "/icons/icon-72.png", sizes: "72x72", type: "image/png", purpose: "any" },
      { src: "/icons/icon-192.png", sizes: "192x192", type: "image/png", purpose: "any" },
      { src: "/icons/icon-512.png", sizes: "512x512", type: "image/png", purpose: "any" },
      { src: "/icons/icon-512.png", sizes: "512x512", type: "image/png", purpose: "maskable" },
    ],
  };
}
