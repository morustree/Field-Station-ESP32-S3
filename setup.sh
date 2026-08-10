#!/bin/bash

# Automatic Setup Script for ESP32-S3 Field Station (Linux/Mac)

set -e

write_header() {
    echo -e "\033[0;36m"
    echo "==============================================="
    echo "   ESP32-S3 Field Station - Auto Setup"
    echo "==============================================="
    echo -e "\033[0m"
}

write_header

if [ -f "setup_config.ini" ]; then
    echo -e "Configuration file 'setup_config.ini' found."
    echo "1. Edit current file"
    echo "2. Reset to defaults (Deletes current file!)"
    echo "3. Continue with current settings"
    read -p "Select an option (1-3): " choice

    if [ "$choice" == "1" ]; then
        if command -v nano >/dev/null 2>&1; then nano "setup_config.ini"; elif command -v vi >/dev/null 2>&1; then vi "setup_config.ini"; else echo "Edit manually."; fi
        exit 0
    elif [ "$choice" == "2" ]; then
        rm "setup_config.ini"
        cp "setup_config.ini.example" "setup_config.ini"
        if command -v nano >/dev/null 2>&1; then nano "setup_config.ini"; elif command -v vi >/dev/null 2>&1; then vi "setup_config.ini"; else echo "Edit manually."; fi
        exit 0
    fi
else
    echo -e "\033[0;33mConfiguration file not found. Creating 'setup_config.ini' for you...\033[0m"
    cp "setup_config.ini.example" "setup_config.ini"
    echo -e "\033[0;36mOpening file for editing. Fill in your details and save it.\033[0m"

    if command -v nano >/dev/null 2>&1; then
        nano "setup_config.ini"
    elif command -v vi >/dev/null 2>&1; then
        vi "setup_config.ini"
    else
        echo "Please edit 'setup_config.ini' manually."
    fi

    echo -e "\nAfter saving and closing the file, run ./setup.sh again to finish.\n"
    exit 0
fi

echo -e "\033[0;33mReading settings from setup_config.ini...\033[0m"

# Extraction using sed
extract_val() {
    grep "$1" setup_config.ini | sed -E 's/.*=\s*"(.*)".*/\1/' || echo ""
}

extract_num() {
    grep "$1" setup_config.ini | sed -E 's/.*=\s*([0-9.]+).*/\1/' || echo "5"
}

SSID=$(extract_val "ssid")
PASS=$(extract_val "password")
IP=$(extract_val "broker_ip")
DEVID=$(extract_val "device_id")
SLEEPMIN=$(extract_num "deep_sleep_minutes")

# Fallback for Device ID
if [ -z "$DEVID" ]; then DEVID="esp32s3_default"; fi
if [ -z "$SLEEPMIN" ]; then SLEEPMIN="5"; fi

echo -e "\033[0;90mDetected Settings:"
echo "  SSID:      $SSID"
echo "  Broker IP: $IP"
echo "  Device ID: $DEVID"
echo -e "  Sleep Min: $SLEEPMIN\033[0m"

if [ -z "$SSID" ] || [ -z "$IP" ]; then
    echo -e "\n\033[0;31mERROR: SSID or Broker IP could not be parsed from setup_config.ini!\033[0m"
    echo -e "\033[0;33mEnsure they are filled correctly between double quotes.\033[0m"
    exit 1
fi

# 1. Generate main/secrets.h
echo -e "\n\033[0;32mGenerating main/secrets.h...\033[0m"
cat <<EOF > main/secrets.h
#ifndef SECRETS_H
#define SECRETS_H

// Automatically generated via setup.sh
#define WIFI_STA_SSID      "$SSID"
#define WIFI_STA_PASSWORD  "$PASS"
#define MQTT_BROKER_URI    "mqtt://${IP}:1883"
#define MQTT_USERNAME      NULL
#define MQTT_PASSWORD      NULL

#define DEVICE_ID          "$DEVID"
#define MQTT_TOPIC         "sensores/${DEVID}/dados"

#define DEEP_SLEEP_MINUTES $SLEEPMIN

#endif
EOF

# 2. Generate frontend/config.js
echo -e "\033[0;32mGenerating frontend/config.js...\033[0m"
cat <<EOF > frontend/config.js
// Automatically generated via setup.sh
const AUTO_CONFIG = {
    host: "$IP",
    port: 9001,
    topic: "sensores/+/dados"
};
EOF

# 3. Final steps
echo -e "\n\033[0;33mConfiguration files generated.\033[0m"

echo -e "\n\033[0;36mSetup completed successfully!\033[0m"
echo -e "\033[0;37mNext steps:"
echo "1. Re-compile and Flash: idf.py build flash"
echo -e "2. Launch Dashboard:     open frontend/index.html (Mac) or xdg-open frontend/index.html (Linux)\033[0m\n"
