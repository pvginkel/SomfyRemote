#include "support.h"

#include "Device.h"

#include "NVSProperty.h"

LOG_TAG(Device);

Device::Device(MQTTConnection& mqtt_connection) : _mqtt_connection(mqtt_connection) {}

void Device::begin() {
    load_state();

    ESP_ERROR_CHECK(_devices.begin());

    _mqtt_connection.on_restart_requested([]() { esp_restart(); });

    _mqtt_connection.on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });

    _mqtt_connection.on_remote_command_requested(
        [this](auto command) { _devices.queue_command(command.device_id, command.command_id, command.long_press); });
}

void Device::set_configuration(DeviceConfiguration* configuration) { _devices.set_configuration(configuration); }

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
