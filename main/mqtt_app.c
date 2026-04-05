#include "mqtt_app.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "MQTT5";

#define LOG_IDLE_TIMEOUT_MS   (10000)

static QueueHandle_t s_publish_queue = NULL;
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static TaskHandle_t s_publish_task = NULL;

enum MQTT_TOPIC {
    LOGGER,
    RPC_REQUEST,
    RPC_RESPONSE,
    TOPIC_COUNT,
};

static const char * const TOPIC_NAME[TOPIC_COUNT] = {
    "/logger",
    "/rpc_interface/request",
    "/rpc_interface/response",
};

static inline const char* topic_name(enum MQTT_TOPIC topic) {
    return (topic >= LOGGER && topic < TOPIC_COUNT) ? TOPIC_NAME[topic] : NULL;
}

struct PublishInfo {
    enum MQTT_TOPIC topic;
    void* publishData;
};

static int mqtt_publish(struct PublishInfo info) {
    if (!s_publish_queue || !info.publishData) {
        if (info.publishData) {
            free(info.publishData);
        }
        return -1;
    }

    // 非阻塞发送到队列
    if (xQueueSend(s_publish_queue, &info, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Publish queue full, dropping message");
        free(info.publishData);
        return -1;
    }

    // 如果任务处于挂起状态，则唤醒它
    if (s_publish_task != NULL) {
        if (eTaskGetState(s_publish_task) == eSuspended) {
            // 注意：xTaskResume 不能在 ISR 中使用；此函数设计为从任务上下文调用
            // 若日志接口可能被中断调用，需改用 xTaskResumeFromISR 并判断是否需要切换
            vTaskResume(s_publish_task);
        }
    }
    return 0;
}

void mqtt_log(char* msg) {
    mqtt_publish((struct PublishInfo){LOGGER, msg});
}

static void mqtt_publish_task(void *arg)
{
    struct PublishInfo info;
    TickType_t idle_timeout_ticks = pdMS_TO_TICKS(LOG_IDLE_TIMEOUT_MS);

    while (1) {
        // 等待队列消息，带超时
        BaseType_t rc = xQueueReceive(s_publish_queue, &info, idle_timeout_ticks);
        if (rc == pdTRUE) {
            // 收到消息，发布到 MQTT
            if (s_mqtt_client) {
                if (topic_name(info.topic) != NULL) {
                    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic_name(info.topic), info.publishData, 0, 1, 0);
                    if (msg_id == -1) {
                        ESP_LOGW(TAG, "Failed to publish message");
                    }
                }
            }
            if (info.publishData != NULL) {
                free(info.publishData);
            }
            // 继续循环，等待下一条消息
        } else {
            // 超时无新消息，挂起自身
            ESP_LOGI(TAG, "Log task idle timeout, suspending");
            vTaskSuspend(NULL);  // 挂起自己
            // 当被恢复时，会从这里继续，重新进入循环等待消息
        }
    }
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static esp_mqtt5_subscribe_property_config_t subscribe_property = {
    .subscribe_id = 25555,
    .no_local_flag = false,
    .retain_as_published_flag = false,
    .retain_handle = 0,
    .is_share_subscribe = true,
    .share_name = "group1",
};

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_log(strdup("Clinet ESP32 online!"));

        esp_mqtt5_client_set_subscribe_property(client, &subscribe_property);
        msg_id = esp_mqtt_client_subscribe(client, topic_name(RPC_REQUEST), 2);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
        ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
        ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
        ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt5_app_start(void)
{
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = false,
        .request_problem_info = false,
        .will_delay_interval = 10,
        .message_expiry_interval = 10,
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,
        .session.keepalive = 10,
        .credentials.username = "",
        .credentials.authentication.password = "",
        .session.last_will.topic = topic_name(LOGGER),
        .session.last_will.msg = "Clinet ESP32-C3 offline.",
        .session.last_will.msg_len = 0,
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);
    s_mqtt_client = client;

    /* Set connection properties and user properties */
    esp_mqtt5_client_set_connect_property(client, &connect_property);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);

    // 创建队列和日志任务
    s_publish_queue = xQueueCreate(10, sizeof(struct PublishInfo));
    if (s_publish_queue) {
        xTaskCreate(mqtt_publish_task, "mqtt_publish", 4096, NULL, 5, &s_publish_task);
    } else {
        ESP_LOGE(TAG, "Failed to create mqtt publish queue");
    }
}