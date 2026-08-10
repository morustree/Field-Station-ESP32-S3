# Automatic Setup Script for ESP32-S3 Field Station

function Write-Header {
    Write-Host "`n===============================================" -ForegroundColor Cyan
    Write-Host "   ESP32-S3 Field Station - Auto Setup" -ForegroundColor Cyan
    Write-Host "===============================================`n" -ForegroundColor Cyan
}

Write-Header

if (Test-Path "setup_config.ini") {
    Write-Host "Configuration file 'setup_config.ini' found." -ForegroundColor Cyan
    Write-Host "1. Edit current file"
    Write-Host "2. Reset to defaults (Deletes current file!)"
    Write-Host "3. Continue with current settings"
    $choice = Read-Host "`nSelect an option (1-3)"

    if ($choice -eq '1') {
        notepad "setup_config.ini"
        Write-Host "`nAfter saving and closing the file, run .\setup.ps1 again to finish.`n" -ForegroundColor White
        exit
    } elseif ($choice -eq '2') {
        Remove-Item "setup_config.ini"
        Write-Host "File removed. Restarting setup..." -ForegroundColor Yellow
        # Re-run logic
        Copy-Item "setup_config.ini.example" "setup_config.ini"
        notepad "setup_config.ini"
        exit
    }
} else {
    Write-Host "Configuration file not found. Creating 'setup_config.ini' for you..." -ForegroundColor Yellow
    Copy-Item "setup_config.ini.example" "setup_config.ini"
    Write-Host "Opening file for editing. Fill in your details and save it." -ForegroundColor Cyan
    notepad "setup_config.ini"
    Write-Host "`nAfter saving and closing the file, run .\setup.ps1 again to finish.`n" -ForegroundColor White
    exit
}

Write-Host "Reading settings from setup_config.ini..." -ForegroundColor Yellow

$content = Get-Content "setup_config.ini" -Raw

# Robust extraction using Regex
$ssid = if ($content -match "ssid\s*=\s*`"([^`"]+)`"") { $Matches[1] } else { $null }
$pass = if ($content -match "password\s*=\s*`"([^`"]+)`"") { $Matches[1] } else { "" }
$ip   = if ($content -match "broker_ip\s*=\s*`"([^`"]+)`"") { $Matches[1] } else { $null }
$devId = if ($content -match "device_id\s*=\s*`"([^`"]+)`"") { $Matches[1] } else { "esp32s3_default" }
$sleepMin = if ($content -match "deep_sleep_minutes\s*=\s*([0-9.]+)") { $Matches[1] } else { "5" }

# Show detected values for confirmation
Write-Host "Detected Settings:" -ForegroundColor Gray
Write-Host "  SSID:      $ssid"
Write-Host "  Broker IP: $ip"
Write-Host "  Device ID: $devId"
Write-Host "  Sleep Min: $sleepMin"

if ([string]::IsNullOrWhiteSpace($ssid) -or [string]::IsNullOrWhiteSpace($ip)) {
    Write-Host "`nERROR: SSID or Broker IP could not be parsed from setup_config.ini!" -ForegroundColor Red
    Write-Host "Ensure they are filled correctly between double quotes." -ForegroundColor Yellow
    exit 1
}

# 1. Generate main/secrets.h
Write-Host "`nGenerating main/secrets.h..." -ForegroundColor Green
# Using ${var} to avoid ambiguity with ":" in the URI string
$secretsContent = @"
#ifndef SECRETS_H
#define SECRETS_H

// Automatically generated via setup.ps1
#define WIFI_STA_SSID      "$ssid"
#define WIFI_STA_PASSWORD  "$pass"
#define MQTT_BROKER_URI    "mqtt://${ip}:1883"
#define MQTT_USERNAME      NULL
#define MQTT_PASSWORD      NULL

#define DEVICE_ID          "$devId"
#define MQTT_TOPIC         "sensores/${devId}/dados"

#define DEEP_SLEEP_MINUTES $sleepMin

#endif
"@
$secretsContent | Out-File -FilePath "main/secrets.h" -Encoding UTF8

# 2. Generate frontend/config.js
Write-Host "Generating frontend/config.js..." -ForegroundColor Green
$jsConfig = @"
// Automatically generated via setup.ps1
const AUTO_CONFIG = {
    host: "$ip",
    port: 9001,
    topic: "sensores/+/dados"
};
"@
$jsConfig | Out-File -FilePath "frontend/config.js" -Encoding UTF8

# 3. Final steps
Write-Host "`nConfiguration files generated." -ForegroundColor Yellow

Write-Host "`nSetup completed successfully!" -ForegroundColor Cyan
Write-Host "Next steps:" -ForegroundColor White
Write-Host "1. Re-compile and Flash: idf.py build flash" -ForegroundColor White
Write-Host "2. Launch Dashboard:     .\frontend\index.html`n" -ForegroundColor White
