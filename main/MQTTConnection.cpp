#include "support.h"

#include "MQTTConnection.h"

#include <charconv>

#include "esp_app_format.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"

LOG_TAG(MQTTConnection);

#define TOPIC_PREFIX "somfy_remote"
#define DEVICE_MANUFACTURER "Pieter"
#define DEVICE_MODEL "Somfy Remote"
#define DEVICE_MODEL_ID "Somfy Remote v1"

#define LAST_WILL_MESSAGE "{\"online\": false}"

#define QOS_MAX_ONE 0      // Send at most one.
#define QOS_MIN_ONE 1      // Send at least one.
#define QOS_EXACTLY_ONE 2  // Send exactly one.

#define MAXIMUM_PACKET_SIZE 4096

MQTTConnection::MQTTConnection(Queue* queue) : _queue(queue), _device_id(get_device_id()) {}

void MQTTConnection::begin() {
    ESP_ERROR_ASSERT(_configuration);

    esp_log_level_set("mqtt5_client", ESP_LOG_WARN);

    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = MAXIMUM_PACKET_SIZE,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 0,
        .message_expiry_interval = 10,
        .payload_format_indicator = true,
    };

    _topic_prefix = TOPIC_PREFIX "/" + _device_id + "/";

    const auto state_topic = _topic_prefix + "state";

    esp_mqtt_client_config_t config = {
        .broker =
            {
                .address =
                    {
                        .uri = _configuration->get_mqtt_endpoint().c_str(),
                    },
            },
        .session =
            {
                .last_will =
                    {
                        .topic = state_topic.c_str(),
                        .msg = LAST_WILL_MESSAGE,
                        .qos = QOS_MIN_ONE,
                        .retain = true,
                    },
                .protocol_ver = MQTT_PROTOCOL_V_5,
            },
        .network =
            {
                .disable_auto_reconnect = false,
            },
        .buffer =
            {
                .size = MAXIMUM_PACKET_SIZE,
            },
    };

    if (_configuration->get_mqtt_username().length()) {
        config.credentials.username = _configuration->get_mqtt_username().c_str();
        config.credentials.authentication.password = _configuration->get_mqtt_password().c_str();
    }

    _client = esp_mqtt_client_init(&config);

    esp_mqtt5_client_set_connect_property(_client, &connect_property);

    esp_mqtt_client_register_event(
        _client, MQTT_EVENT_ANY,
        [](auto eventHandlerArg, auto eventBase, auto eventId, auto eventData) {
            ((MQTTConnection*)eventHandlerArg)->event_handler(eventBase, eventId, eventData);
        },
        this);

    esp_mqtt_client_start(_client);
}

string MQTTConnection::get_device_id() {
    uint8_t mac[6];

    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));

    return strformat("0x%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void MQTTConnection::event_handler(esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, eventBase, eventId);
    auto event = (esp_mqtt_event_handle_t)eventData;

    ESP_LOGD(TAG, "Free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(),
             esp_get_minimum_free_heap_size());

    switch ((esp_mqtt_event_id_t)eventId) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            handle_connected();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");

            _connected_changed.queue(_queue, {false});
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT subscribed error %d", (int)event->error_handle->error_type);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT unsubscribed");
            break;

        case MQTT_EVENT_PUBLISHED:
            break;

        case MQTT_EVENT_DATA:
            handle_data(event);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT return code is %d", event->error_handle->connect_return_code);
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                if (event->error_handle->esp_tls_last_esp_err) {
                    ESP_LOGI(TAG, "reported from esp-tls");
                }
                if (event->error_handle->esp_tls_stack_err) {
                    ESP_LOGI(TAG, "reported from tls stack");
                }
                if (event->error_handle->esp_transport_sock_errno) {
                    ESP_LOGI(TAG, "captured as transport's socket errno");
                }
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;

        default:
            ESP_LOGD(TAG, "Other event id: %d", event->event_id);
            break;
    }
}

void MQTTConnection::handle_connected() {
    subscribe(_topic_prefix + "set/+");

    publish_configuration();
    publish_discovery();

    _connected_changed.queue(_queue, {true});
}

void MQTTConnection::handle_data(esp_mqtt_event_handle_t event) {
    // We don't support message chunking.
    ESP_ERROR_ASSERT(!event->current_data_offset);

    if (!event->topic_len) {
        ESP_LOGW(TAG, "Handling data without topic");
        return;
    }

    auto topic = string(event->topic, event->topic_len);

    if (!topic.starts_with(_topic_prefix)) {
        ESP_LOGE(TAG, "Unexpected topic %s topic len %d data len %d", topic.c_str(), event->topic_len, event->data_len);
        return;
    }

    auto sub_topic = topic.c_str() + _topic_prefix.length();
    auto data = string(event->data, event->data_len);

    if (strcmp(sub_topic, "set/identify") == 0) {
        ESP_LOGI(TAG, "Requested identification");

        _identify_requested.queue(_queue);
    } else if (strcmp(sub_topic, "set/restart") == 0) {
        ESP_LOGI(TAG, "Requested restart");

        _restart_requested.queue(_queue);
    } else {
        ESP_LOGE(TAG, "Unknown topic %s", topic.c_str());
    }
}

void MQTTConnection::subscribe(const string& topic) {
    ESP_LOGI(TAG, "Subscribing to topic %s", topic.c_str());

    ESP_ERROR_ASSERT(esp_mqtt_client_subscribe(_client, topic.c_str(), 0) >= 0);
}

void MQTTConnection::unsubscribe(const string& topic) {
    ESP_LOGI(TAG, "Unsubscribing from topic %s", topic.c_str());

    ESP_ERROR_ASSERT(esp_mqtt_client_unsubscribe(_client, topic.c_str()) >= 0);
}

void MQTTConnection::publish_configuration() {
    ESP_LOGI(TAG, "Publishing configuration information");

    auto uniqueIdentifier = strformat("%s_%s", TOPIC_PREFIX, _device_id.c_str());

    cJSON_Data root = {cJSON_CreateObject()};

    cJSON_AddStringToObject(*root, "unique_id", uniqueIdentifier.c_str());

    auto device = cJSON_AddObjectToObject(*root, "device");
    cJSON_AddStringToObject(device, "manufacturer", DEVICE_MANUFACTURER);
    cJSON_AddStringToObject(device, "model", DEVICE_MODEL);
    cJSON_AddStringToObject(device, "name", _configuration->get_device_name().c_str());
    cJSON_AddStringToObject(device, "firmware_version", get_firmware_version().c_str());

    publish_json(*root, _topic_prefix + "configuration", true);
}

void MQTTConnection::publish_json(cJSON* root, const string& topic, bool retain) {
    auto json = cJSON_PrintUnformatted(root);

    ESP_ERROR_ASSERT(esp_mqtt_client_publish(_client, topic.c_str(), json, 0, QOS_MIN_ONE, retain) >= 0);

    cJSON_free(json);
}

void MQTTConnection::publish_discovery() {
    publish_number_discovery("Volume", "volume", "volume", "mdi:knob", "config", "sound_pressure", "Db", 0, 1, 0.1);
    publish_binary_sensor_discovery("Playing", "playing", "mdi:play", "diagnostic", nullptr, false);
    publish_binary_sensor_discovery("Recording", "recording", "mdi:record", "diagnostic", nullptr, false);
    publish_switch_discovery("Enabled", "enabled", "enabled", "mdi:toggle-switch", "config", nullptr);
    publish_button_discovery("Identify", "identify", nullptr, "config", "identify");
    publish_button_discovery("Restart", "restart", nullptr, "config", "restart");
}

void MQTTConnection::publish_number_discovery(const char* name, const char* command_topic, const char* state_property,
                                              const char* icon, const char* entity_category, const char* device_class,
                                              const char* unit_of_measurement, double min, double max, double step) {
    auto root = create_discovery("number", name, state_property, icon, entity_category, device_class, true);

    if (unit_of_measurement) {
        cJSON_AddStringToObject(*root, "unit_of_measurement", unit_of_measurement);
    }
    cJSON_AddNumberToObject(*root, "min", min);
    cJSON_AddNumberToObject(*root, "max", max);
    cJSON_AddNumberToObject(*root, "step", step);
    cJSON_AddStringToObject(*root, "command_topic",
                            strformat("somfy_remote/%s/set/%s", _device_id.c_str(), command_topic).c_str());
    cJSON_AddStringToObject(*root, "state_topic", strformat("somfy_remote/%s/state", _device_id.c_str()).c_str());
    cJSON_AddStringToObject(*root, "value_template", strformat("{{ value_json.%s }}", state_property).c_str());

    publish_json(*root, strformat("homeassistant/number/%s/%s/config", _device_id.c_str(), state_property), true);
}

void MQTTConnection::publish_binary_sensor_discovery(const char* name, const char* state_property, const char* icon,
                                                     const char* entity_category, const char* device_class,
                                                     bool enabled_by_default) {
    auto root = create_discovery("binary_sensor", name, state_property, icon, entity_category, device_class,
                                 enabled_by_default);

    cJSON_AddBoolToObject(*root, "payload_on", true);
    cJSON_AddBoolToObject(*root, "payload_off", false);
    cJSON_AddStringToObject(*root, "state_topic", strformat("somfy_remote/%s/state", _device_id.c_str()).c_str());
    cJSON_AddStringToObject(*root, "value_template", strformat("{{ value_json.%s }}", state_property).c_str());

    publish_json(*root, strformat("homeassistant/binary_sensor/%s/%s/config", _device_id.c_str(), state_property),
                 true);
}

void MQTTConnection::publish_switch_discovery(const char* name, const char* command_topic, const char* state_property,
                                              const char* icon, const char* entity_category, const char* device_class) {
    auto root = create_discovery("switch", name, state_property, icon, entity_category, device_class, true);

    cJSON_AddStringToObject(*root, "command_topic",
                            strformat("somfy_remote/%s/set/%s", _device_id.c_str(), command_topic).c_str());
    cJSON_AddBoolToObject(*root, "payload_on", true);
    cJSON_AddBoolToObject(*root, "payload_off", false);
    cJSON_AddStringToObject(*root, "state_topic", strformat("somfy_remote/%s/state", _device_id.c_str()).c_str());
    cJSON_AddStringToObject(*root, "value_template", strformat("{{ value_json.%s }}", state_property).c_str());

    publish_json(*root, strformat("homeassistant/switch/%s/%s/config", _device_id.c_str(), state_property), true);
}

void MQTTConnection::publish_button_discovery(const char* name, const char* command_topic, const char* icon,
                                              const char* entity_category, const char* device_class) {
    auto root = create_discovery("button", name, command_topic, icon, entity_category, device_class, true);

    cJSON_AddStringToObject(*root, "command_topic",
                            strformat("somfy_remote/%s/set/%s", _device_id.c_str(), command_topic).c_str());
    cJSON_AddStringToObject(*root, "payload_press", "true");

    publish_json(*root, strformat("homeassistant/button/%s/%s/config", _device_id.c_str(), command_topic), true);
}

cJSON_Data MQTTConnection::create_discovery(const char* component, const char* name, const char* object_id,
                                            const char* icon, const char* entity_category, const char* device_class,
                                            bool enabled_by_default) {
    // Device classes can be found here: https://www.home-assistant.io/integrations/sensor/#device-class.
    // Entity category is either config or diagnostic.
    // MDI icons can be found here: https://pictogrammers.com/library/mdi/.

    const auto root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "icon", icon);
    if (entity_category) {
        cJSON_AddStringToObject(root, "entity_category", entity_category);
    }
    if (device_class) {
        cJSON_AddStringToObject(root, "device_class", device_class);
    }

    const auto availability = cJSON_AddArrayToObject(root, "availability");

    const auto availability_item = cJSON_CreateObject();
    cJSON_AddItemToArray(availability, availability_item);

    cJSON_AddStringToObject(availability_item, "topic", strformat("somfy_remote/%s/state", _device_id.c_str()).c_str());
    cJSON_AddStringToObject(availability_item, "value_template", "{{ value_json.online }}");
    cJSON_AddBoolToObject(availability_item, "payload_available", true);

    cJSON_AddStringToObject(root, "availability_mode", "all");

    const auto device = cJSON_AddObjectToObject(root, "device");

    const auto identifiers = cJSON_AddArrayToObject(device, "identifiers");
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(strformat("%s_%s", TOPIC_PREFIX, _device_id.c_str()).c_str()));

    cJSON_AddStringToObject(device, "manufacturer", DEVICE_MANUFACTURER);
    cJSON_AddStringToObject(device, "model", DEVICE_MODEL);
    cJSON_AddStringToObject(device, "model_id", DEVICE_MODEL_ID);
    cJSON_AddStringToObject(device, "name", _configuration->get_device_name().c_str());
    cJSON_AddStringToObject(device, "sw_version", get_firmware_version().c_str());

    cJSON_AddStringToObject(root, "unique_id", strformat("%s_%s_%s", _device_id.c_str(), component, object_id).c_str());
    cJSON_AddStringToObject(root, "object_id",
                            strformat("%s_%s", _configuration->get_device_entity_id().c_str(), object_id).c_str());

    if (!enabled_by_default) {
        cJSON_AddBoolToObject(root, "enabled_by_default", false);
    }

    return root;
}

string MQTTConnection::get_firmware_version() {
    const auto running_partition = esp_ota_get_running_partition();

    esp_app_desc_t running_app_info;
    ESP_ERROR_CHECK(esp_ota_get_partition_description(running_partition, &running_app_info));

    return running_app_info.version;
}

void MQTTConnection::send_state(DeviceState& state) {
    ESP_LOGI(TAG, "Publishing new state");

    ESP_ERROR_ASSERT(_client);

    cJSON_Data root = {cJSON_CreateObject()};

    cJSON_AddBoolToObject(*root, "online", true);

    auto json = cJSON_PrintUnformatted(*root);

    auto topic = _topic_prefix + "state";
    auto result = esp_mqtt_client_publish(_client, topic.c_str(), json, 0, QOS_MIN_ONE, true);
    if (result < 0) {
        ESP_LOGE(TAG, "Sending status update message failed with error %d", result);
    }

    cJSON_free(json);
}
