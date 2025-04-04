
#include "SUB_MQTT.h"

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_mqttsub, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_mqttsub, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "v1/devices/me/telemetry", 0);
        ESP_LOGI(TAG_mqttsub, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_mqttsub, "MQTT_EVENT_DISCONNECTED");
        break;
    default:
        ESP_LOGI(TAG_mqttsub, "Other event id:%d", event->event_id);
        break;
    }

}


void mqtt_app_start(void)
{
    
    /*
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://demo.thingsboard.io",
        .broker.address.port = 1883,
        .credentials.username = "esp32",
        .credentials.authentication.password = "67750205Tat@",
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };
    */
    esp_mqtt_client_config_t mqtt_cfg = {
    .broker = {
        .address = {
            .hostname = "mqtt.thingsboard.cloud",    // Set hostname only
            .port = 1883,                         // Default MQTT port
            .transport = MQTT_TRANSPORT_OVER_TCP,      // Set transport type, or remove this line
        },
        .verification = {
            .use_global_ca_store = false,
        },
    },
    .credentials = {
        .username = "esp32",
        .client_id = "3fd463f0-bd4d-11ef-ae0e-b51a7d74c077",
        .set_null_client_id = false,
        .authentication = {
            .password = "67750205Tat@",
        },
    },
    .session = {
        .keepalive = 120,
        .disable_clean_session = false,
    },
    .network = {
        .reconnect_timeout_ms = 10000,
        .timeout_ms = 10000,
        .disable_auto_reconnect = false,
    },
    .task = {
        .priority = 5,
        .stack_size = 4096,
    }, 
    .buffer = {
        .size = 1024,
    },
    .outbox = {
        .limit = 2048,
    },
};
    
    ESP_LOGI(TAG_mqttsub, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG_mqttsub, "Failed to initialize MQTT client");
        return;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    //
}



