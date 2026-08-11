#!/bin/bash
# ESP32-S3 Field Station - Interactive Master Setup (Linux/Mac)

# Colors
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
GREEN='\033[0;32m'
GRAY='\033[0;90m'
NC='\033[0m'

write_header() {
    clear
    echo -e "${CYAN}==============================================="
    echo "   ESP32-S3 FIELD STATION - MASTER SETUP"
    echo -e "===============================================${NC}"
    echo -e "${GRAY} Press Ctrl+C to cancel at any time.${NC}"
    echo -e "${GRAY} Windows & Linux: press Ctrl+Shift+C to copy and Ctrl+Shift+V to paste.${NC}"
    echo -e "${GRAY} macOS: press Cmd+C to copy and Cmd+V to paste.${NC}"
}

check_prerequisites() {
    echo -e "\n${YELLOW}[1/5] Checking Prerequisites...${NC}"

    # Check ESP-IDF
    if ! command -v idf.py &> /dev/null; then
        echo -e "${RED}ERROR: 'idf.py' not found!${NC}"
        echo -e "${YELLOW}Please ensure ESP-IDF is installed and its environment variables are loaded.${NC}"
        exit 1
    fi
    echo -e "  ${GREEN}[OK] ESP-IDF detected.${NC}"

    # Check Mosquitto
    if ! command -v mosquitto &> /dev/null; then
        echo -e "${RED}ERROR: Mosquitto MQTT Broker not found!${NC}"
        echo -e "${YELLOW}Please install Mosquitto (e.g., 'brew install mosquitto' or 'apt install mosquitto').${NC}"
        exit 1
    fi
    echo -e "  ${GREEN}[OK] Mosquitto detected.${NC}"
}

get_config_input() {
    echo -e "\n${YELLOW}[2/5] Configuration (Interactive CLI)${NC}"
    echo -e "${GRAY}Tip: Press ENTER to keep the value shown in [brackets].${NC}"

    # Defaults
    DEF_SSID="Your_WiFi_SSID"
    DEF_PASS=""
    DEF_IP="127.0.0.1"
    DEF_DEV="esp32s3_field_01"
    DEF_SLEEP="5"

    if [ -f "setup_config.ini" ]; then
        DEF_SSID=$(grep "ssid" setup_config.ini | sed -E 's/.*=\s*"(.*)".*/\1/')
        DEF_PASS=$(grep "password" setup_config.ini | sed -E 's/.*=\s*"(.*)".*/\1/')
        DEF_IP=$(grep "broker_ip" setup_config.ini | sed -E 's/.*=\s*"(.*)".*/\1/')
        DEF_DEV=$(grep "device_id" setup_config.ini | sed -E 's/.*=\s*"(.*)".*/\1/')
        DEF_SLEEP=$(grep "deep_sleep_minutes" setup_config.ini | sed -E 's/.*=\s*([0-9.]+).*/\1/')
    fi

    while true; do
        read -p "Enter Wi-Fi SSID [$DEF_SSID]: " SSID
        SSID=${SSID:-$DEF_SSID}

        read -p "Enter Wi-Fi Password [$DEF_PASS]: " PASS
        PASS=${PASS:-$DEF_PASS}

        read -p "Enter Broker IP [$DEF_IP]: " IP
        IP=${IP:-$DEF_IP}

        read -p "Enter Device ID [$DEF_DEV]: " DEV
        DEV=${DEV:-$DEF_DEV}

        read -p "Deep Sleep Interval (minutes) [$DEF_SLEEP]: " SLEEP
        SLEEP=${SLEEP:-$DEF_SLEEP}

        echo -e "\nValidating connection to Broker IP: $IP..."
        if ping -c 1 -W 2 "$IP" > /dev/null 2>&1; then
            echo -e "  ${GREEN}[OK] Broker IP is reachable.${NC}"

            # Optional port check if nc is available
            if command -v nc >/dev/null 2>&1; then
                if nc -z -w 2 "$IP" 1883 >/dev/null 2>&1; then
                    echo -e "  ${GREEN}[OK] MQTT Broker is active on port 1883.${NC}"
                else
                    echo -e "  ${YELLOW}[WARNING] No MQTT Broker detected on $IP:1883.${NC}"
                fi
            fi
            break
        else
            echo -e "  ${YELLOW}[WARNING] Could not ping $IP. Check if the IP is correct.${NC}"
            read -p "Continue anyway? (y/n): " CHOICE
            if [ "$CHOICE" == "y" ]; then break; fi
        fi
    done
}

save_configs() {
    echo -e "\n${YELLOW}[3/5] Generating configuration files...${NC}"

    # setup_config.ini
    cat <<EOF > setup_config.ini
; Field Station Configuration
ssid = "$SSID"
password = "$PASS"
broker_ip = "$IP"
device_id = "$DEV"
deep_sleep_minutes = $SLEEP
EOF

    # main/secrets.h
    cat <<EOF > main/secrets.h
#ifndef SECRETS_H
#define SECRETS_H
// Automatically generated via setup.sh
#define WIFI_STA_SSID      "$SSID"
#define WIFI_STA_PASSWORD  "$PASS"
#define MQTT_BROKER_URI    "mqtt://$IP:1883"
#define MQTT_USERNAME      NULL
#define MQTT_PASSWORD      NULL
#define DEVICE_ID          "$DEV"
#define MQTT_TOPIC         "sensores/$DEV/dados"
#define DEEP_SLEEP_MINUTES $SLEEP
#endif
EOF

    # frontend/config.js
    cat <<EOF > frontend/config.js
// Automatically generated via setup.sh
const AUTO_CONFIG = {
    host: "$IP",
    port: 9001,
    topic: "sensores/+/dados"
};
EOF

    echo -e "  ${GREEN}[OK] All configuration files updated.${NC}"
}

invoke_build_flash() {
    echo -e "\n${YELLOW}[4/5] Build & Flash Process${NC}"
    read -p "Do you want to compile and flash to ESP32-S3 now? (y/n): " ANS
    if [ "$ANS" != "y" ]; then return; fi

    echo -e "\n${CYAN}IMPORTANT: Ensure your ESP32-S3 is connected via USB.${NC}"
    echo "You might need to put it into Download Mode:"
    echo "  1. Hold BOOT button"
    echo "  2. Press RESET"
    echo "  3. Release BOOT"

    if idf.py build flash; then
        echo -e "\n${GREEN}[OK] Firmware successfully flashed!${NC}"
    else
        echo -e "\n${RED}[ERROR] Build or Flash failed. Check the logs above. Ensure the correct port is selected.${NC}"
    fi
}

launch_services() {
    echo -e "\n${YELLOW}[5/5] Launch Services${NC}"

    # Check if MQTT is already running
    MQTT_RUNNING=false
    if command -v lsof >/dev/null 2>&1; then
        if lsof -Pi :1883 -sTCP:LISTEN -t >/dev/null 2>&1; then MQTT_RUNNING=true; fi
    elif command -v netstat >/dev/null 2>&1; then
        if netstat -tuln | grep -q :1883; then MQTT_RUNNING=true; fi
    fi

    if [ "$MQTT_RUNNING" = true ]; then
        echo -e "  ${GREEN}[INFO] MQTT Broker already running on port 1883.${NC}"
    else
        echo -e "Starting Mosquitto in background...${CYAN}"
        mosquitto -c mosquitto/mosquitto.conf -v &
    fi

    read -p "Open Telemetry Monitor? (y/n): " ANS
    if [ "$ANS" == "y" ]; then
        echo -e "Opening Monitor in your browser...${NC}"
        if command -v open > /dev/null; then open frontend/index.html
        elif command -v xdg-open > /dev/null; then xdg-open frontend/index.html
        else echo "Please open frontend/index.html manually."; fi
    fi
}

# Main
write_header
check_prerequisites
get_config_input
save_configs
invoke_build_flash
launch_services

echo -e "\n${CYAN}==============================================="
echo "          SETUP PROCESS COMPLETED!"
echo -e "===============================================${NC}\n"
