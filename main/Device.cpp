#include "support.h"

#include "Device.h"

#include "NVSProperty.h"

LOG_TAG(Device);

Device::Device(MQTTConnection& mqtt_connection) : _mqtt_connection(mqtt_connection) {}

void Device::begin() {
    load_state();

    _mqtt_connection.on_restart_requested([]() { esp_restart(); });

    _mqtt_connection.on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });

    _mqtt_connection.on_remote_command_requested([this](auto command) {
        ESP_LOGI(TAG, "Received remote command %d for device %d", static_cast<int>(command.command_id),
                 command.device_id);
    });
}

void Device::state_changed() {
    if (_mqtt_connection.is_connected()) {
        _mqtt_connection.send_state(_state);
    }
}

void Device::load_state() {
    // The device doesn't have any structured state. State for the remote
    // devices is managed by the SomfyRemote implementation itself.
}

void Device::save_state() {
    // The device doesn't have any structured state. State for the remote
    // devices is managed by the SomfyRemote implementation itself.
}
