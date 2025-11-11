#pragma once

#include <optional>
#include <set>

#include "Callback.h"
#include "DeviceConfiguration.h"
#include "DeviceState.h"
#include "Queue.h"
#include "RemoteDevice.h"
#include "Span.h"
#include "mqtt_client.h"

struct MQTTConnectionState {
    bool connected;
};

struct MQTTRemoteCommand {
    int device_id;
    RemoteCommandId command_id;
};

class MQTTConnection {
    static constexpr double DEFAULT_SETPOINT = 19;

    static string get_device_id();

    Queue* _queue;
    string _device_id;
    DeviceConfiguration* _configuration;
    string _topic_prefix;
    esp_mqtt_client_handle_t _client{};
    Callback<MQTTConnectionState> _connected_changed;
    Callback<void> _identify_requested;
    Callback<void> _restart_requested;
    Callback<MQTTRemoteCommand> _remote_command_requested;

public:
    MQTTConnection(Queue* queue);

    void set_configuration(DeviceConfiguration* configuration) { _configuration = configuration; }
    void begin();
    bool is_connected() { return !!_client; }
    void send_state(DeviceState& state);
    void on_connected_changed(function<void(MQTTConnectionState)> func) { _connected_changed.add(func); }
    void on_identify_requested(function<void()> func) { _identify_requested.add(func); }
    void on_restart_requested(function<void()> func) { _restart_requested.add(func); }
    void on_remote_command_requested(function<void(MQTTRemoteCommand)> func) { _remote_command_requested.add(func); }

private:
    void event_handler(esp_event_base_t eventBase, int32_t eventId, void* eventData);
    void handle_connected();
    void handle_data(esp_mqtt_event_handle_t event);
    void subscribe(const string& topic);
    void unsubscribe(const string& topic);
    void publish_configuration();
    void publish_json(cJSON* root, const string& topic, bool retain);
    void publish_discovery();
    void publish_button_discovery(const char* name, const char* command_topic, const char* icon,
                                  const char* entity_category, const char* device_class);
    void publish_subdevice_button_discovery(const char* name, const char* command_topic, const char* subdevice_name,
                                            const char* subdevice_id, const char* icon, const char* entity_category,
                                            const char* device_class);
    cJSON_Data create_discovery(const char* component, const char* name, const char* object_id,
                                const char* subdevice_name, const char* subdevice_id, const char* icon,
                                const char* entity_category, const char* device_class, bool enabled_by_default);
    string get_firmware_version();
    optional<RemoteCommandId> command_id_from_name(const char* name);
    int find_remote_id(const string& remote_name);
};
