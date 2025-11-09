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
}

void Device::state_changed() {
    if (_mqtt_connection.is_connected()) {
        _mqtt_connection.send_state(_state);
    }
}

void Device::load_state() {
    nvs_handle_t handle;
    auto err = nvs_open("storage", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return;
    }
    ESP_ERROR_CHECK(err);

    nvs_close(handle);

    ESP_LOGI(TAG, "Loaded audio configuration:");
}

void Device::save_state() {
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));

    nvs_close(handle);
}
