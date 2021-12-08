#include "esp_websocket_client.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *WS_TAG = "websocket";

#define WEBSOCKET_URI CONFIG_FD_WEBSOCKET_ADDR

esp_websocket_client_handle_t client;

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(WS_TAG, "Received opcode=%d", data->op_code);
        ESP_LOGW(WS_TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        ESP_LOGW(WS_TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(WS_TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}


int ws_init()
{
    esp_websocket_client_config_t websocket_cfg = {};

    websocket_cfg.uri = WEBSOCKET_URI;

    ESP_LOGI(WS_TAG, "Connecting to %s...", websocket_cfg.uri);

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(client);

    int i = 0 ;
    while (i++ < 10) {
        if (esp_websocket_client_is_connected(client)) {
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    return 0;
}

int ws_send(const char*data , int len)
{
    if (esp_websocket_client_is_connected(client))
    {
        return esp_websocket_client_send_text(client, data, len, (TickType_t) 500);
    }
    return 0;
}

int ws_is_connected()
{
    return esp_websocket_client_is_connected(client) ? 1 : 0;
}