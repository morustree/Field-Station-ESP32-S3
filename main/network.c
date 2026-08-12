#include "network.h"
#include <string.h>
#include <time.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_sntp.h"
#include "freertos/semphr.h"

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_wifi_connected = false;
static bool s_mqtt_connected = false;
static SemaphoreHandle_t s_mqtt_pub_sem = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false;
        s_mqtt_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_wifi_connected = true;
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    (void)event_id;
    const esp_mqtt_event_t *event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            s_mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_mqtt_connected = false;
            break;
        case MQTT_EVENT_PUBLISHED:
            if (s_mqtt_pub_sem) {
                xSemaphoreGive(s_mqtt_pub_sem);
            }
            break;
        default:
            break;
    }
}

esp_err_t network_init(const char *ssid, const char *password, const char *mqtt_uri)
{
    static bool s_sys_init = false;
    if (!s_sys_init) {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            err = nvs_flash_init();
        }
        if (err != ESP_OK) return err;
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
        s_sys_init = true;
    }

    if (s_mqtt_pub_sem == NULL) {
        s_mqtt_pub_sem = xSemaphoreCreateBinary();
    }

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // Wait for Wi-Fi (max 10s)
    for (int retry = 0; retry < 100; retry++) {
        if (s_wifi_connected) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!s_wifi_connected) return ESP_ERR_TIMEOUT;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = mqtt_uri,
            },
        },
        .credentials = {
            .client_id = MQTT_CLIENT_ID,
        },
    };
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt_client);

    for (int retry = 0; retry < 50; retry++) {
        if (s_mqtt_connected) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return s_mqtt_connected ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t network_sync_time(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Se o ano for maior que 2020, o RTC já tem o tempo mantido do sleep anterior
    if (timeinfo.tm_year > (2020 - 1900)) {
        return ESP_OK;
    }

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    const int max_retries = 30;
    for (int retry = 0; retry < max_retries; retry++) {
        if (esp_sntp_get_sync_status() != SNTP_SYNC_STATUS_RESET) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

bool network_is_connected(void)
{
    return s_wifi_connected && s_mqtt_connected;
}

esp_err_t network_publish(const char *topic, const char *payload, int qos)
{
    if (!s_mqtt_connected) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_mqtt_pub_sem, 0); // Clear
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, payload, 0, qos, 0);
    if (msg_id < 0) return ESP_FAIL;

    if (qos > 0) {
        BaseType_t ret = xSemaphoreTake(s_mqtt_pub_sem, pdMS_TO_TICKS(5000));
        if (ret != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

void network_disconnect(void)
{
    if (s_mqtt_client) {
        esp_mqtt_client_stop(s_mqtt_client);
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
    }
    esp_wifi_stop();
    s_wifi_connected = false;
    s_mqtt_connected = false;
}
