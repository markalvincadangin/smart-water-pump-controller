#!/usr/bin/env sh
# Post-deployment health check: pings GET /api/health and exits non-zero if 503.
# Usage: ./scripts/post-deploy-health-check.sh [BASE_URL]
# Example: ./scripts/post-deploy-health-check.sh https://your-app.vercel.app

BASE_URL="${1:-http://localhost:3000}"
URL="${BASE_URL%/}/api/health"

echo "Pinging $URL ..."
HTTP_CODE=$(curl -s -o /tmp/health-response.json -w "%{http_code}" "$URL" || echo "000")

if [ "$HTTP_CODE" = "503" ]; then
  echo "FAIL: Health check returned 503 — Firebase not initialized or env missing."
  echo "Response: $(cat /tmp/health-response.json 2>/dev/null || echo 'none')"
  echo "Remediation: Ensure NEXT_PUBLIC_FIREBASE_* are set in the deployment environment (e.g. Vercel env vars)."
  exit 1
fi

if [ "$HTTP_CODE" != "200" ]; then
  echo "FAIL: Health check returned HTTP $HTTP_CODE (expected 200)."
  cat /tmp/health-response.json 2>/dev/null || true
  exit 1
fi

echo "OK: Health check HTTP $HTTP_CODE"
cat /tmp/health-response.json 2>/dev/null | head -5
exit 0
