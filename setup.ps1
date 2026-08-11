# ESP32-S3 Field Station - Interactive Master Setup (Windows)
$ErrorActionPreference = "Stop"

function Write-Header {
    Clear-Host
    Write-Host "===============================================" -ForegroundColor Cyan
    Write-Host "   ESP32-S3 FIELD STATION - MASTER SETUP" -ForegroundColor Cyan
    Write-Host "===============================================" -ForegroundColor Cyan
    Write-Host " Press Ctrl+C to cancel at any time." -ForegroundColor Gray
    Write-Host " Windows & Linux: press Ctrl+Shift+C to copy and Ctrl+Shift+V to paste." -ForegroundColor Gray
    Write-Host " macOS: press Cmd+C to copy and Cmd+V to paste." -ForegroundColor Gray
}

function Check-Prerequisites {
    Write-Host "`n[1/5] Checking Prerequisites..." -ForegroundColor Yellow

    # Check ESP-IDF
    $idfPath = Get-Command idf.py -ErrorAction SilentlyContinue
    if (-not $idfPath) {
        Write-Host "ERROR: 'idf.py' not found!" -ForegroundColor Red
        Write-Host "Please run this script from the 'ESP-IDF Terminal' in VS Code." -ForegroundColor Yellow
        exit 1
    }
    Write-Host "  [OK] ESP-IDF detected." -ForegroundColor Green

    # Check Mosquitto
    $mosqCmd = Get-Command mosquitto -ErrorAction SilentlyContinue
    $mosquittoPaths = @()
    if ($mosqCmd) { $mosquittoPaths += $mosqCmd.Source }
    $mosquittoPaths += "C:\Program Files\mosquitto\mosquitto.exe"
    $mosquittoPaths += "C:\Program Files (x86)\mosquitto\mosquitto.exe"

    $global:MOSQUITTO_CMD = $null
    foreach ($path in $mosquittoPaths) {
        if ($path -and (Test-Path $path)) { $global:MOSQUITTO_CMD = $path; break }
    }

    if (-not $global:MOSQUITTO_CMD) {
        Write-Host "ERROR: Mosquitto MQTT Broker not found!" -ForegroundColor Red
        Write-Host "Please install Mosquitto and add it to your PATH." -ForegroundColor Yellow
        exit 1
    }
    Write-Host "  [OK] Mosquitto detected at: $global:MOSQUITTO_CMD" -ForegroundColor Green
}

function Get-ConfigInput {
    Write-Host "`n[2/5] Configuration (Interactive CLI)" -ForegroundColor Yellow
    Write-Host "Tip: Press ENTER to keep the current value shown in [brackets]." -ForegroundColor Gray

    # Load defaults if exists
    $defSsid = "Your_WiFi_SSID"
    $defPass = ""
    $defIp = "192.168.1.100"
    $defDev = "esp32s3_field_01"
    $defSleep = "5"

    if (Test-Path "setup_config.ini") {
        $content = Get-Content "setup_config.ini" -Raw
        if ($content -match "ssid\s*=\s*`"([^`"]+)`"") { $defSsid = $Matches[1] }
        if ($content -match "password\s*=\s*`"([^`"]*)`"") { $defPass = $Matches[1] }
        if ($content -match "broker_ip\s*=\s*`"([^`"]+)`"") { $defIp = $Matches[1] }
        if ($content -match "device_id\s*=\s*`"([^`"]+)`"") { $defDev = $Matches[1] }
        if ($content -match "deep_sleep_minutes\s*=\s*([0-9.]+)") { $defSleep = $Matches[1] }
    }

    while ($true) {
        $ssid = Read-Host "Enter Wi-Fi SSID [$defSsid]"
        if ([string]::IsNullOrWhiteSpace($ssid)) { $ssid = $defSsid }

        $pass = Read-Host "Enter Wi-Fi Password (leave empty for none) [$defPass]"
        if ([string]::IsNullOrWhiteSpace($pass) -and $defPass -ne "") { $pass = $defPass }

        $ip = Read-Host "Enter Broker IP (your computer's IP) [$defIp]"
        if ([string]::IsNullOrWhiteSpace($ip)) { $ip = $defIp }

        $devId = Read-Host "Enter Device ID [$defDev]"
        if ([string]::IsNullOrWhiteSpace($devId)) { $devId = $defDev }

        $sleep = Read-Host "Deep Sleep Interval (minutes) [$defSleep]"
        if ([string]::IsNullOrWhiteSpace($sleep)) { $sleep = $defSleep }

        Write-Host "`nValidating connection to Broker IP: $ip..." -ForegroundColor Cyan
        $ping = Test-Connection -ComputerName $ip -Count 1 -Quiet
        if ($ping) {
            Write-Host "  [OK] Broker IP is reachable." -ForegroundColor Green

            # Check if MQTT port is open
            $tcpTest = Test-NetConnection -ComputerName $ip -Port 1883 -InformationLevel Quiet
            if ($tcpTest) {
                Write-Host "  [OK] MQTT Broker is active on port 1883." -ForegroundColor Green
            } else {
                Write-Host "  [WARNING] No MQTT Broker detected on $ip`:1883. Ensure it is running before the station starts." -ForegroundColor Yellow
            }

            $global:CONFIG = @{ ssid=$ssid; pass=$pass; ip=$ip; dev=$devId; sleep=$sleep }
            break
        } else {
            Write-Host "  [WARNING] Could not ping $ip. Check if the IP is correct." -ForegroundColor Yellow
            $choice = Read-Host "Continue anyway? (y/n)"
            if ($choice -eq 'y') {
                $global:CONFIG = @{ ssid=$ssid; pass=$pass; ip=$ip; dev=$devId; sleep=$sleep }
                break
            }
        }
    }
}

function Save-Configs {
    Write-Host "`n[3/5] Generating configuration files..." -ForegroundColor Yellow

    $c = $global:CONFIG

    # 1. setup_config.ini
    $iniContent = @"
; Field Station Configuration
ssid = "$($c.ssid)"
password = "$($c.pass)"
broker_ip = "$($c.ip)"
device_id = "$($c.dev)"
deep_sleep_minutes = $($c.sleep)
"@
    $iniContent | Out-File -FilePath "setup_config.ini" -Encoding UTF8

    # 2. main/secrets.h
    $secretsContent = @"
#ifndef SECRETS_H
#define SECRETS_H

// Automatically generated via setup.ps1
#define WIFI_STA_SSID      "$($c.ssid)"
#define WIFI_STA_PASSWORD  "$($c.pass)"
#define MQTT_BROKER_URI    "mqtt://$($c.ip):1883"
#define MQTT_USERNAME      NULL
#define MQTT_PASSWORD      NULL

#define DEVICE_ID          "$($c.dev)"
#define MQTT_TOPIC         "sensores/$($c.dev)/dados"

#define DEEP_SLEEP_MINUTES $($c.sleep)

#endif
"@
    $secretsContent | Out-File -FilePath "main/secrets.h" -Encoding UTF8

    # 3. frontend/config.js
    $jsConfig = @"
// Automatically generated via setup.ps1
const AUTO_CONFIG = {
    host: "$($c.ip)",
    port: 9001,
    topic: "sensores/+/dados"
};
"@
    $jsConfig | Out-File -FilePath "frontend/config.js" -Encoding UTF8

    Write-Host "  [OK] All configuration files updated." -ForegroundColor Green
}

function Invoke-BuildFlash {
    Write-Host "`n[4/5] Build & Flash Process" -ForegroundColor Yellow
    $ans = Read-Host "Do you want to compile and flash to ESP32-S3 now? (y/n)"
    if ($ans -ne 'y') { return }

    Write-Host "`nIMPORTANT: Ensure your ESP32-S3 is connected via USB." -ForegroundColor Cyan
    Write-Host "You might need to put it into Download Mode:" -ForegroundColor White
    Write-Host "  1. Hold BOOT button`n  2. Press RESET`n  3. Release BOOT" -ForegroundColor White

    try {
        Write-Host "`nExecuting: idf.py build flash..." -ForegroundColor Gray
        & idf.py build flash
        Write-Host "`n[OK] Firmware successfully flashed!" -ForegroundColor Green
    } catch {
        Write-Host "`n[ERROR] Build or Flash failed. Check the logs above." -ForegroundColor Red
    }
}

function Launch-Services {
    Write-Host "`n[5/5] Launch Services" -ForegroundColor Yellow

    # Check if MQTT is already running
    $portActive = Get-NetTCPConnection -LocalPort 1883 -ErrorAction SilentlyContinue

    if ($portActive) {
        Write-Host "  [INFO] A service is already listening on port 1883 (Mosquitto Service?)." -ForegroundColor Green
    } else {
        Write-Host "Starting Mosquitto MQTT Broker..." -ForegroundColor Cyan
        $mosqArgs = "-c mosquitto/mosquitto.conf -v"
        Start-Process -FilePath $global:MOSQUITTO_CMD -ArgumentList $mosqArgs -WindowStyle Normal
    }

    $ans = Read-Host "Open Web Dashboard? (y/n)"
    if ($ans -eq 'y') {
        Write-Host "Opening Dashboard in your browser..." -ForegroundColor Cyan
        Start-Process "frontend/index.html"
    }
}

# Execution
Write-Header
Check-Prerequisites
Get-ConfigInput
Save-Configs
Invoke-BuildFlash
Launch-Services

Write-Host "`n===============================================" -ForegroundColor Cyan
Write-Host "          SETUP PROCESS COMPLETED!" -ForegroundColor Cyan
Write-Host "===============================================`n" -ForegroundColor Cyan
