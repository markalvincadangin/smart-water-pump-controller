Param(
    [string]$OutputFile = "docs/audit/qa/2026-03-31/day-1/baseline_snapshot.md"
)

# Capture baseline metadata for QA traceability before execution.
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$timestamp = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
$mainHash = git -C "$repoRoot/firmware/platformio_smart_water_pump_controller" rev-parse --short HEAD 2>$null
$sensorHash = git -C "$repoRoot/firmware/platformio_sensor_node" rev-parse --short HEAD 2>$null
$dashHash = git -C "$repoRoot/dashboard" rev-parse --short HEAD 2>$null
$rulesHash = git -C "$repoRoot" hash-object database.rules.json 2>$null

if (-not $mainHash) { $mainHash = "UNKNOWN" }
if (-not $sensorHash) { $sensorHash = "UNKNOWN" }
if (-not $dashHash) { $dashHash = "UNKNOWN" }
if (-not $rulesHash) { $rulesHash = "UNKNOWN" }

$content = @"
# QA Baseline Snapshot

Captured UTC: $timestamp

- ESP32 master firmware commit: $mainHash
- NodeMCU firmware commit: $sensorHash
- Dashboard commit: $dashHash
- Firebase rules file hash: $rulesHash

"@

$targetPath = Join-Path $repoRoot $OutputFile
$targetDir = Split-Path -Parent $targetPath
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir | Out-Null
}

Set-Content -Path $targetPath -Value $content -Encoding UTF8
Write-Output "Baseline snapshot written to $OutputFile"
