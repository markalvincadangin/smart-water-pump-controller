param([string]$TaskName)

$WorkspaceRoot = Split-Path -Parent $PSScriptRoot
$EnvPath = Join-Path $WorkspaceRoot ".env"

if (Test-Path $EnvPath) {
    Get-Content $EnvPath | ForEach-Object {
        if ($_ -match '^\s*([^#=]+)\s*=\s*(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1].Trim(), $matches[2].Trim(), "Process")
        }
    }
} else {
    Write-Warning ".env file not found at $EnvPath"
}

if ($TaskName -eq "app_build") {
    Set-Location $WorkspaceRoot
    .\gradlew.bat assembleDebug
}
elseif ($TaskName -eq "app_run") {
    & $env:ADB_PATH connect $env:POCO_IP
    Set-Location $WorkspaceRoot
    .\gradlew.bat app:installDebug
    & $env:ADB_PATH -s $env:POCO_IP shell am start -n com.smartflow/.MainActivity
}
elseif ($TaskName -eq "app_pair") {
    Write-Host "--- ADB Wireless Pairing ---"
    $pairString = Read-Host "Enter the PAIRING IP and Port (from 'Pair device with pairing code' e.g., 192.168.1.10:12345)"
    $code = Read-Host "Enter the 6-digit Wi-Fi pairing code"
    if ([string]::IsNullOrWhiteSpace($pairString) -or [string]::IsNullOrWhiteSpace($code)) {
        Write-Error "Pairing string and code cannot be empty."
        exit 1
    }
    & $env:ADB_PATH pair $pairString $code

    Write-Host "`n--- ADB Wireless Connection ---"
    $connectString = Read-Host "Look at the main Wireless Debugging screen. Enter the CONNECTION IP and Port (e.g., 192.168.1.10:54321)"
    if (-not [string]::IsNullOrWhiteSpace($connectString)) {
        $envPath = Join-Path $WorkspaceRoot ".env"
        $envContent = Get-Content $envPath
        $envContent = $envContent -replace "^POCO_IP=.*", "POCO_IP=$connectString"
        Set-Content -Path $envPath -Value $envContent
        Write-Host "Successfully updated POCO_IP in .env to $connectString!"
    }
}
elseif ($TaskName -eq "app_logcat") {
    $pidof = & $env:ADB_PATH -s $env:POCO_IP shell pidof -s com.smartflow
    if ($pidof) {
        & $env:ADB_PATH -s $env:POCO_IP logcat --pid=$pidof
    } else {
        Write-Output "App is not running."
    }
}
elseif ($TaskName -eq "firmware_build") {
    Set-Location (Join-Path $WorkspaceRoot "firmware\master_node")
    python -m platformio run -e esp32dev_usb_ota
}
elseif ($TaskName -eq "firmware_flash_ota") {
    Set-Location (Join-Path $WorkspaceRoot "firmware\master_node")
    python -m platformio run -e esp32dev_ota -t upload --upload-port $env:ESP32_IP
}
elseif ($TaskName -eq "firmware_flash_usb") {
    Set-Location (Join-Path $WorkspaceRoot "firmware\master_node")
    python -m platformio run -e esp32dev_usb_ota -t upload
}
elseif ($TaskName -eq "firmware_monitor") {
    python -u read_telnet.py $env:ESP32_IP
}
