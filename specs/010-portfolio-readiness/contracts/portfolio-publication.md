# Portfolio Publication Contract

## README

The root page must lead with the problem, current solution, field-prototype status, and sole-developer scope; provide safe visual evidence; show the current sensor-to-Android architecture; highlight evidence-backed challenges; explain layered safety without implying certification; list current stack and structure; provide runnable software checks; and route operations detail to canonical docs.

It must not describe the archived web app as supported, use “industrial-grade,” “battle-tested,” or unqualified “production,” invent impact metrics, expose household/device data, or show a status badge unsupported by current CI.

## Visuals

- Public copies live under `docs/assets/portfolio/` with descriptive filenames.
- Every image has alt text and a caption.
- Sensitive content is cropped/redacted or the image is excluded.
- Electrical photos make no certification or code-compliance claim without evidence.

## CI

- Pull requests and default-branch pushes validate Android and Cloud Functions.
- Android uses the Gradle wrapper and required Java version.
- Functions install locked dependencies, compile, and test on declared Node.js.
- No workflow references the archived dashboard or Vercel.
- Deployment remains out of scope without a verified destination and protected credential model.

## Publication Gate

Before visibility changes: no known active secrets remain; likely historical exposure is assessed and active secrets rotated; visuals pass privacy review; links, license, metadata, and CI are accurate; unresolved limitations are shown to the owner; and the owner explicitly authorizes publication.
