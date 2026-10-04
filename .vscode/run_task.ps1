param(
    [string]$TaskName,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraArgs
)

$WorkspaceRoot = Split-Path -Parent $PSScriptRoot
$EnvPath = Join-Path $WorkspaceRoot ".env"

# Load developer environment variables from .env
if (Test-Path $EnvPath) {
    Get-Content $EnvPath | ForEach-Object {
        if ($_ -match '^\s*([^#=]+)\s*=\s*(.*)$') {
            $val = $matches[2].Trim()
            # Strip surrounding quotes if present
            $val = $val -replace '^["'']|["'']$', ''
            [Environment]::SetEnvironmentVariable($matches[1].Trim(), $val, "Process")
        }
    }
} else {
    Write-Warning ".env file not found at $EnvPath"
}

# --- Helper Functions ---

function Test-TcpPort {
    param(
        [string]$RemoteHost,
        [int]$Port,
        [int]$TimeoutMs = 2000
    )
    if ([string]::IsNullOrWhiteSpace($RemoteHost)) { return $false }
    try {
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $iar = $tcpClient.BeginConnect($RemoteHost, $Port, $null, $null)
        $wait = $iar.AsyncWaitHandle.WaitOne($TimeoutMs, $false)
        if (-not $wait) {
            $tcpClient.Close()
            return $false
        }
        $tcpClient.EndConnect($iar)
        $tcpClient.Close()
        return $true
    } catch {
        return $false
    }
}

function Test-AdbConnection {
    param([string]$AdbPath, [string]$DeviceIp)
    if (-not (Test-Path $AdbPath)) {
        Write-Host "[-] ADB not found at: $AdbPath" -ForegroundColor Red
        return $false
    }
    if ([string]::IsNullOrWhiteSpace($DeviceIp)) {
        Write-Host "[-] POCO_IP not set in .env" -ForegroundColor Red
        return $false
    }

    $devices = & $AdbPath devices
    $matched = $devices | Where-Object { $_ -match [regex]::Escape($DeviceIp) -and $_ -match "device\b" }
    return ($null -ne $matched)
}

# --- Task Implementations ---

if ($TaskName -eq "app_build") {
    Set-Location $WorkspaceRoot
    Write-Host "[*] Building Android App (assembleDebug)..." -ForegroundColor Cyan
    .\gradlew.bat assembleDebug
}
elseif ($TaskName -eq "app_run") {
    Set-Location $WorkspaceRoot
    if (-not (Test-Path $env:ADB_PATH)) {
        Write-Error "ADB not found at '$env:ADB_PATH'. Please check ADB_PATH in .env."
        exit 1
    }

    if ([string]::IsNullOrWhiteSpace($env:POCO_IP)) {
        Write-Error "POCO_IP is not set in .env."
        exit 1
    }

    Write-Host "[*] Ensuring ADB wireless connection to $env:POCO_IP..." -ForegroundColor Cyan
    $connected = Test-AdbConnection -AdbPath $env:ADB_PATH -DeviceIp $env:POCO_IP
    if (-not $connected) {
        & $env:ADB_PATH connect $env:POCO_IP
    }

    Write-Host "[*] Installing debug APK onto device..." -ForegroundColor Cyan
    .\gradlew.bat app:installDebug
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[*] Launching com.smartflow/.MainActivity..." -ForegroundColor Green
        & $env:ADB_PATH -s $env:POCO_IP shell am start -n com.smartflow/.MainActivity
    }
}
elseif ($TaskName -eq "app_pair") {
    if (-not (Test-Path $env:ADB_PATH)) {
        Write-Error "ADB not found at '$env:ADB_PATH'. Please check ADB_PATH in .env."
        exit 1
    }

    Write-Host "=== ADB Wireless Pairing ===" -ForegroundColor Cyan
    $pairString = Read-Host "Enter PAIRING IP and Port (from 'Pair device with pairing code' e.g., 192.168.1.10:12345)"
    $code = Read-Host "Enter the 6-digit Wi-Fi pairing code"
    if ([string]::IsNullOrWhiteSpace($pairString) -or [string]::IsNullOrWhiteSpace($code)) {
        Write-Error "Pairing string and code cannot be empty."
        exit 1
    }

    Write-Host "[*] Pairing device..." -ForegroundColor Cyan
    & $env:ADB_PATH pair $pairString $code

    Write-Host "`n=== ADB Wireless Connection ===" -ForegroundColor Cyan
    $connectString = Read-Host "Enter CONNECTION IP and Port from main Wireless Debugging screen (e.g., 192.168.1.10:54321)"
    if (-not [string]::IsNullOrWhiteSpace($connectString)) {
        $envContent = Get-Content $EnvPath
        if ($envContent -match "^POCO_IP=") {
            $envContent = $envContent -replace "^POCO_IP=.*", "POCO_IP=$connectString"
        } else {
            $envContent += "`nPOCO_IP=$connectString"
        }
        Set-Content -Path $EnvPath -Value $envContent
        Write-Host "[+] Successfully updated POCO_IP in .env to $connectString" -ForegroundColor Green

        Write-Host "[*] Connecting to $connectString now..." -ForegroundColor Cyan
        & $env:ADB_PATH connect $connectString
        & $env:ADB_PATH devices
    }
}
elseif ($TaskName -eq "app_check") {
    Write-Host "=== Android ADB Connection Check ===" -ForegroundColor Cyan
    if (-not (Test-Path $env:ADB_PATH)) {
        Write-Host "[-] ADB Path: NOT FOUND ($env:ADB_PATH)" -ForegroundColor Red
        exit 1
    }
    Write-Host "[+] ADB Path: $env:ADB_PATH" -ForegroundColor Green
    Write-Host "[*] Target Device IP: $env:POCO_IP" -ForegroundColor Cyan

    & $env:ADB_PATH connect $env:POCO_IP
    $devices = & $env:ADB_PATH devices
    Write-Host "`nAttached Devices:" -ForegroundColor Yellow
    $devices | ForEach-Object { Write-Host "  $_" }

    $isReady = Test-AdbConnection -AdbPath $env:ADB_PATH -DeviceIp $env:POCO_IP
    if ($isReady) {
        Write-Host "`n[+] Device is CONNECTED and READY for deployment." -ForegroundColor Green
        $model = & $env:ADB_PATH -s $env:POCO_IP shell getprop ro.product.model
        $androidVer = & $env:ADB_PATH -s $env:POCO_IP shell getprop ro.build.version.release
        Write-Host "    Model: $model" -ForegroundColor White
        Write-Host "    Android Version: $androidVer" -ForegroundColor White
    } else {
        Write-Host "`n[-] Device is NOT connected or unauthorized." -ForegroundColor Red
        Write-Host "    Tip: Run task 'App: Pair Device' if Wi-Fi debugging port changed." -ForegroundColor Yellow
    }
}
elseif ($TaskName -eq "app_logcat") {
    if (-not (Test-Path $env:ADB_PATH)) {
        Write-Error "ADB not found at '$env:ADB_PATH'."
        exit 1
    }
    $pidof = & $env:ADB_PATH -s $env:POCO_IP shell pidof -s com.smartflow
    if ($pidof) {
        Write-Host "[*] Streaming Logcat for PID: $pidof (com.smartflow)..." -ForegroundColor Cyan
        & $env:ADB_PATH -s $env:POCO_IP logcat --pid=$pidof
    } else {
        Write-Host "[!] App 'com.smartflow' is not running on device $env:POCO_IP." -ForegroundColor Yellow
        Write-Host "[*] Streaming all SmartFlow tags instead..." -ForegroundColor Cyan
        & $env:ADB_PATH -s $env:POCO_IP logcat -s SmartFlow:V AndroidRuntime:E
    }
}
elseif ($TaskName -eq "firmware_build") {
    Set-Location (Join-Path $WorkspaceRoot "firmware\master_node")
    Write-Host "[*] Compiling ESP32 Master Node Firmware..." -ForegroundColor Cyan
    python -m platformio run -e esp32dev_usb_ota
}
elseif ($TaskName -eq "firmware_flash_ota") {
    Set-Location (Join-Path $WorkspaceRoot "firmware\master_node")
    if ([string]::IsNullOrWhiteSpace($env:ESP32_IP)) {
        Write-Error "ESP32_IP is not set in .env."
        exit 1
    }
    Write-Host "[*] Checking reachability of ESP32 at $env:ESP32_IP..." -ForegroundColor Cyan
    $reachable = Test-Connection -ComputerName $env:ESP32_IP -Count 1 -Quiet
    if (-not $reachable) {
        Write-Warning "ESP32 at $env:ESP32_IP did not respond to ping. Attempting OTA anyway..."
    }
    python -m platformio run -e esp32dev_ota -t upload --upload-port $env:ESP32_IP
}
elseif ($TaskName -eq "firmware_flash_usb") {
    Set-Location (Join-Path $WorkspaceRoot "firmware\master_node")
    Write-Host "[*] Flashing ESP32 via USB..." -ForegroundColor Cyan
    python -m platformio run -e esp32dev_usb_ota -t upload
}
elseif ($TaskName -eq "firmware_monitor") {
    Set-Location $WorkspaceRoot
    $targetIp = if ($ExtraArgs -and $ExtraArgs.Count -ge 1) { $ExtraArgs[0] } else { $env:ESP32_IP }
    python -u read_telnet.py $targetIp
}
elseif ($TaskName -eq "firmware_check") {
    Write-Host "=== ESP32 Firmware Connection Check ===" -ForegroundColor Cyan
    $targetIp = if ($ExtraArgs -and $ExtraArgs.Count -ge 1) { $ExtraArgs[0] } else { $env:ESP32_IP }

    if ([string]::IsNullOrWhiteSpace($targetIp)) {
        Write-Host "[-] ESP32_IP is not set in arguments or .env" -ForegroundColor Red
        exit 1
    }

    Write-Host "[*] Target ESP32 IP: $targetIp" -ForegroundColor White
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $pingOk = Test-Connection -ComputerName $targetIp -Count 1 -Quiet
    $sw.Stop()

    if ($pingOk) {
        Write-Host "[+] ICMP Ping: ALIVE (${($sw.ElapsedMilliseconds)}ms)" -ForegroundColor Green
    } else {
        Write-Host "[-] ICMP Ping: NO RESPONSE" -ForegroundColor Yellow
        Write-Host "    (Note: Some Wi-Fi routers block ICMP between wireless clients)" -ForegroundColor DarkGray
    }

    $telnetOpen = Test-TcpPort -RemoteHost $targetIp -Port 2323 -TimeoutMs 3000
    if ($telnetOpen) {
        Write-Host "[+] Telnet Port 2323: OPEN & ACCEPTING CONNECTIONS" -ForegroundColor Green
        Write-Host "[+] ESP32 logging sink is ready. You can start 'Firmware: Monitor (Telnet)'." -ForegroundColor Green
    } else {
        Write-Host "[-] Telnet Port 2323: CLOSED / UNREACHABLE" -ForegroundColor Red
        Write-Host "    Check if ESP32 has finished booting and TelnetSink is initialized." -ForegroundColor Yellow
    }
}
elseif ($TaskName -eq "env_check") {
    Write-Host "=== SmartFlow Environment Verification ===" -ForegroundColor Cyan
    Write-Host "Workspace: $WorkspaceRoot" -ForegroundColor DarkGray

    # 1. .env file
    if (Test-Path $EnvPath) {
        Write-Host "[+] .env file: FOUND" -ForegroundColor Green
    } else {
        Write-Host "[-] .env file: MISSING at $EnvPath" -ForegroundColor Red
    }

    # 2. Python
    try {
        $pyVer = python --version 2>&1
        Write-Host "[+] Python: $pyVer" -ForegroundColor Green
    } catch {
        Write-Host "[-] Python: NOT INSTALLED or NOT IN PATH" -ForegroundColor Red
    }

    # 3. PlatformIO
    try {
        $pioVer = python -m platformio --version 2>&1
        Write-Host "[+] PlatformIO: $pioVer" -ForegroundColor Green
    } catch {
        Write-Host "[-] PlatformIO: NOT AVAILABLE" -ForegroundColor Red
    }

    # 4. ADB
    if (Test-Path $env:ADB_PATH) {
        Write-Host "[+] Android ADB: FOUND ($env:ADB_PATH)" -ForegroundColor Green
    } else {
        Write-Host "[-] Android ADB: NOT FOUND at $env:ADB_PATH" -ForegroundColor Red
    }

    # 5. Device Reachability
    Write-Host "`n--- Device Connectivity ---" -ForegroundColor Yellow
    if (-not [string]::IsNullOrWhiteSpace($env:ESP32_IP)) {
        $telnetOk = Test-TcpPort -RemoteHost $env:ESP32_IP -Port 2323 -TimeoutMs 2000
        if ($telnetOk) {
            Write-Host "[+] ESP32 ($env:ESP32_IP): ONLINE (Telnet port 2323 reachable)" -ForegroundColor Green
        } else {
            Write-Host "[-] ESP32 ($env:ESP32_IP): UNREACHABLE on port 2323" -ForegroundColor Yellow
        }
    } else {
        Write-Host "[?] ESP32_IP: Not configured in .env" -ForegroundColor Yellow
    }

    if (-not [string]::IsNullOrWhiteSpace($env:POCO_IP) -and (Test-Path $env:ADB_PATH)) {
        $adbOk = Test-AdbConnection -AdbPath $env:ADB_PATH -DeviceIp $env:POCO_IP
        if ($adbOk) {
            Write-Host "[+] Android Phone ($env:POCO_IP): CONNECTED via ADB" -ForegroundColor Green
        } else {
            Write-Host "[-] Android Phone ($env:POCO_IP): NOT CONNECTED via ADB" -ForegroundColor Yellow
        }
    } else {
        Write-Host "[?] POCO_IP: Not configured in .env" -ForegroundColor Yellow
    }
}
else {
    Write-Host "Unknown task: $TaskName" -ForegroundColor Red
    Write-Host "Available tasks: app_build, app_pair, app_run, app_check, app_logcat, firmware_build, firmware_flash_ota, firmware_flash_usb, firmware_monitor, firmware_check, env_check" -ForegroundColor Yellow
}
