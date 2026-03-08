import type { MetadataRoute } from "next";

/**
 * PWA manifest — enables "Add to Home Screen" / installable app.
 * Users can install the dashboard like YouTube, Facebook, etc.
 */
export default function manifest(): MetadataRoute.Manifest {
  return {
    name: "Smart Water Pump System",
    short_name: "Water Pump",
    description: "Real-time monitoring and control for the Smart Water Pump System. Deep well pump with dry-run protection.",
    start_url: "/",
    display: "standalone",
    background_color: "#0a0e14",
    theme_color: "#00d4aa",
    orientation: "any",
    icons: [
      {
        src: "/icons/icon-72.png",
        sizes: "72x72",
        type: "image/png",
        purpose: "any",
      },
      {
        src: "/icons/icon-192.png",
        sizes: "192x192",
        type: "image/png",
        purpose: "any",
      },
      {
        src: "/icons/icon-512.png",
        sizes: "512x512",
        type: "image/png",
        purpose: "maskable",
      },
    ],
  };
}
