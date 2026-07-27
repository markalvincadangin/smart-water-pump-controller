import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { Resvg } from "@resvg/resvg-js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const ROOT = path.resolve(__dirname, "..");
const INPUT_SVG = path.join(ROOT, "public", "logos", "brandmark.svg");
const OUT_DIR = path.join(ROOT, "public", "icons");

function forceBrandFill(svgText) {
  // Ensure the mark renders in SmartFlow blue and is visible on transparent backgrounds.
  // We only touch <path ...> in this SVG (single-path asset).
  return svgText.replace(/<path\s+/g, '<path fill="#185FA5" ');
}

function extractMark(svgText) {
  // Resvg expects a single valid SVG document; we embed only the path(s) from the source SVG.
  const matches = svgText.match(/<path\b[\s\S]*?\/>/g);
  if (!matches || matches.length === 0) {
    throw new Error("No <path /> elements found in source SVG.");
  }
  return matches.join("\n");
}

function renderPng(svgText, sizePx, { paddingPct = 0 } = {}) {
  // Render into a square canvas with optional padding (for maskable icons).
  const pad = Math.max(0, Math.min(40, paddingPct));
  const inner = 100 - pad * 2;
  const wrapped = `
<svg xmlns="http://www.w3.org/2000/svg" width="${sizePx}" height="${sizePx}" viewBox="0 0 100 100">
  <rect width="100" height="100" fill="#F1EFE8"/>
  <g transform="translate(${pad} ${pad}) scale(${inner / 100})">
    ${svgText}
  </g>
</svg>`;

  const resvg = new Resvg(wrapped, {
    fitTo: { mode: "width", value: sizePx },
    background: "rgba(0,0,0,0)",
  });
  return resvg.render().asPng();
}

async function main() {
  const raw = await fs.readFile(INPUT_SVG, "utf8");
  const branded = forceBrandFill(raw);
  const mark = extractMark(branded);

  await fs.mkdir(OUT_DIR, { recursive: true });

  // Standard icons
  await fs.writeFile(path.join(OUT_DIR, "icon-192.png"), renderPng(mark, 192, { paddingPct: 10 }));
  await fs.writeFile(path.join(OUT_DIR, "icon-512.png"), renderPng(mark, 512, { paddingPct: 10 }));

  // Maskable icons: more padding for safe maskable area
  await fs.writeFile(path.join(OUT_DIR, "icon-192-maskable.png"), renderPng(mark, 192, { paddingPct: 18 }));
  await fs.writeFile(path.join(OUT_DIR, "icon-512-maskable.png"), renderPng(mark, 512, { paddingPct: 18 }));

  // Apple touch icon (180×180)
  await fs.writeFile(path.join(OUT_DIR, "apple-touch-icon.png"), renderPng(mark, 180, { paddingPct: 12 }));

  // eslint-disable-next-line no-console
  console.log("PWA icons generated in public/icons/");
}

await main();

