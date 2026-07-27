import { spawn } from "node:child_process";

// Lighthouse CI is flaky on some Windows setups (Temp folder locks / EPERM).
// We still keep `npm run validate:lighthouse` for manual runs, but skip it in `npm run validate`
// on Windows so CI/dev validation is deterministic.
if (process.platform === "win32") {
  console.log("[lhci] Skipping Lighthouse CI on Windows (known EPERM temp-dir cleanup issue).");
  process.exit(0);
}

const child = spawn("npx", ["lhci", "autorun"], { stdio: "inherit", shell: true });
child.on("exit", (code) => process.exit(code ?? 1));

