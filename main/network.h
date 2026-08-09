#ifndef NETWORK_H
#define NETWORK_H

#include "esp_err.h"
#include <stdbool.h>
#include "secrets.h"

#define MQTT_CLIENT_ID     DEVICE_ID
#define MQTT_KEEPALIVE     60
#define MQTT_LWT_TOPIC     "sensores/esp32s3_01/status"
#define MQTT_LWT_MSG       "offline"
#define MQTT_LWT_QOS       1
#define MQTT_LWT_RETAIN    false

esp_err_t network_init(const char *ssid, const char *password, const char *mqtt_uri);
esp_err_t network_sync_time(void);
bool network_is_connected(void);
esp_err_t network_publish(const char *topic, const char *payload, int qos);
void network_disconnect(void);

#endif // NETWORK_H
