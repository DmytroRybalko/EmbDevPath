#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>   


#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"

#define WIFI_SSID "TP-LINK_54D0"            // REPLACE DATA!!!
#define WIFI_PASS "fixDag-5vophi-cudfeg"    // REPLACE PAROLE
#define WIFI_GOT_IP_BIT BIT0

static esp_mqtt_client_handle_t s_client;
static bool s_mqtt_connected = false;   

static const char *TAG = "mqtts_example";
static EventGroupHandle_t s_wifi_events;


/*
 * @brief Handles Wi-Fi and IP events for the station interface.
 *
 * This function initiates a connection attempt when the stack starts,
 * retries after a disconnect, and signals when the station receives an IP.
 *
 * @param arg User data registered for the event handler.
 * @param base Event base for the received event.
 * @param id Event ID for the received event.
 * @param data Event payload associated with the Wi-Fi/IP event.
 */
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        xEventGroupSetBits(s_wifi_events, WIFI_GOT_IP_BIT);
    }
}

/*
 * @brief Initializes the ESP32 station interface and connects to Wi-Fi.
 *
 * This function creates the default Wi-Fi station netif, registers the event
 * handlers, configures the SSID/password, starts the Wi-Fi driver, and waits
 * until an IP is acquired before continuing.
 */
static void wifi_init_sta(void)
{
    s_wifi_events = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Wait until we have an IP before starting MQTT */
    xEventGroupWaitBits(s_wifi_events, WIFI_GOT_IP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

/*
 * @brief Event handler registered to receive MQTT events.
 *
 * This function is called by the MQTT client event loop and handles connection,
 * subscription, publish, data reception, and error events.
 *
 * @param handler_args User data registered to the event.
 * @param base Event base for the handler (always MQTT base in this example).
 * @param event_id The ID of the received event.
 * @param event_data The event payload, cast to esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            s_mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "dmytro/esp32/test", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            s_mqtt_connected = false;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
            msg_id = esp_mqtt_client_publish(client, "dmytro/esp32/test", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

/*
 * @brief Publishes a payload to the specified MQTT topic.
 *
 * The message is queued only if the MQTT client exists and is connected.
 *
 * @param topic Target MQTT topic to publish to.
 * @param data Message payload to publish.
 */
void mqtt_app_publish(const char *topic, const char *data)
{
    if (!s_client) {
        ESP_LOGW(TAG, "publish skipped: client is NULL");
        return;
    }
    if (!s_mqtt_connected) {
        ESP_LOGW(TAG, "publish skipped: MQTT not connected");
        return;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, data, 0, 1, 1); // qos=1, retain=1 for debug
    if (msg_id < 0) {
        ESP_LOGE(TAG, "publish failed: topic=%s", topic);
    } else {
        ESP_LOGI(TAG, "publish queued: topic=%s, msg_id=%d, payload=%s", topic, msg_id, data);
    }
}

/*
 * @brief Initializes the required Wi-Fi and MQTT components for the application.
 *
 * This function configures NVS, the network stack, the default event loop,
 * connects to Wi-Fi, creates the MQTT client, registers the event handler,
 * and starts the MQTT client.
 */
void mqtt_app_start(void)
{

    ESP_LOGI("MQTT", "[APP] Startup..");   

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_sta();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtts://broker.hivemq.com:8883", //CONFIG_EXAMPLE_MQTT_BROKER_URI,
            .verification.crt_bundle_attach = esp_crt_bundle_attach, /* Use built-in certificate bundle */
        },
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
}
