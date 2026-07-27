const withPWA = require("@ducanh2912/next-pwa").default({
  dest: "public",
  // Don't cache firebase-messaging-sw — it's served by our API
  exclude: ["firebase-messaging-sw.js"],
  disable: process.env.NODE_ENV === "development",
});

/** @type {import('next').NextConfig} */
const nextConfig = {
  async rewrites() {
    return [
      {
        source: "/firebase-messaging-sw.js",
        destination: "/api/firebase-messaging-sw",
      },
    ];
  },
};

module.exports = withPWA(nextConfig);
