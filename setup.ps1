# Script de Setup Automático para ESP32-S3 Field Station

function Write-Header {
    Write-Host "`n===============================================" -ForegroundColor Cyan
    Write-Host "   ESP32-S3 Field Station - Auto Setup" -ForegroundColor Cyan
    Write-Host "===============================================`n" -ForegroundColor Cyan
}

if (!(Test-Path "setup_config.ini")) {
    Write-Host "ERRO: setup_config.ini não encontrado!" -ForegroundColor Red
    Write-Host "Por favor, copie 'setup_config.ini.example' para 'setup_config.ini' e preencha seus dados." -ForegroundColor Yellow
    exit
}

Write-Header
Write-Host "Lendo configurações de setup_config.ini..." -ForegroundColor Yellow

$config = Get-Content "setup_config.ini"
$ssid = ($config | Select-String "ssid\s*=\s*`"(.+)`"").Matches.Groups[1].Value
$pass = ($config | Select-String "password\s*=\s*`"(.+)`"").Matches.Groups[1].Value
$ip = ($config | Select-String "broker_ip\s*=\s*`"(.+)`"").Matches.Groups[1].Value
$devId = ($config | Select-String "device_id\s*=\s*`"(.+)`"").Matches.Groups[1].Value

if ([string]::IsNullOrWhiteSpace($ssid) -or [string]::IsNullOrWhiteSpace($ip)) {
    Write-Error "SSID ou IP do Broker não configurados no setup_config.ini!"
    exit
}

# 1. Gerar main/secrets.h
Write-Host "Gerando main/secrets.h..." -ForegroundColor Green
$secretsContent = @"
#ifndef SECRETS_H
#define SECRETS_H

// Gerado automaticamente via setup.ps1
#define WIFI_STA_SSID      "$ssid"
#define WIFI_STA_PASSWORD  "$pass"
#define MQTT_BROKER_URI    "mqtt://$ip:1883"
#define MQTT_USERNAME      NULL
#define MQTT_PASSWORD      NULL

#define DEVICE_ID          "$devId"
#define MQTT_TOPIC         "sensores/$devId/dados"

#endif
"@
$secretsContent | Out-File -FilePath "main/secrets.h" -Encoding UTF8

# 2. Gerar frontend/config.js
Write-Host "Gerando frontend/config.js..." -ForegroundColor Green
$jsConfig = @"
// Gerado automaticamente via setup.ps1
const AUTO_CONFIG = {
    host: "$ip",
    port: 9001,
    topic: "sensores/+/dados"
};
"@
$jsConfig | Out-File -FilePath "frontend/config.js" -Encoding UTF8

# 3. Subir Infraestrutura Docker
Write-Host "Iniciando Docker Compose (Mosquitto)..." -ForegroundColor Yellow
docker-compose up -d

Write-Host "`nSetup concluído com sucesso!" -ForegroundColor Cyan
Write-Host "Próximos passos:" -ForegroundColor White
Write-Host "1. Compile e grave o firmware: idf.py build flash" -ForegroundColor White
Write-Host "2. Abra o dashboard: frontend/index.html`n" -ForegroundColor White
