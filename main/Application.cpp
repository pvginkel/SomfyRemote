#include "support.h"

#include "Application.h"

#include "driver/i2c.h"
#include "nvs_flash.h"

LOG_TAG(Application);

void Application::do_begin() {
    get_mqtt_connection().on_publish_discovery([this]() { publish_mqtt_discovery(); });

    get_mqtt_connection().on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });
}

void Application::do_configuration_loaded(cJSON* data) {
    ESP_ERROR_CHECK(_configuration.load(data));

    _devices.set_configuration(&_configuration);

    get_mqtt_connection().on_publish_discovery([this]() { publish_mqtt_discovery(); });

    get_mqtt_connection().on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });
}

void Application::state_changed() {
    if (!get_mqtt_connection().is_connected()) {
        return;
    }

    get_mqtt_connection().send_state();
}

#define REGISTER_DEVICE_BUTTON(device, device_id, name_, key_, icon_, command, long_press) \
    get_mqtt_connection().publish_button_discovery(                                        \
        {                                                                                  \
            .name = name_,                                                                 \
            .object_id = key_,                                                             \
            .icon = icon_,                                                                 \
            .subdevice_name = device.get_name().c_str(),                                   \
            .subdevice_id = device.get_id().c_str(),                                       \
        },                                                                                 \
        [this, device_id]() {                                                              \
            ESP_LOGI(TAG, "Requested button press " name_);                                \
                                                                                           \
            _devices.queue_command(device_id, RemoteCommandId::command, long_press);       \
        });

void Application::publish_mqtt_discovery() {
    get_mqtt_connection().publish_button_discovery(
        {
            .name = "Identify",
            .object_id = "identify",
            .entity_category = "config",
            .device_class = "identify",
        },
        []() { ESP_LOGI(TAG, "Requested identification"); });

    get_mqtt_connection().publish_button_discovery(
        {
            .name = "Restart",
            .object_id = "restart",
            .entity_category = "config",
            .device_class = "restart",
        },
        []() {
            ESP_LOGI(TAG, "Requested restart");

            esp_restart();
        });

    for (int i = 0; i < _configuration.get_devices().size(); i++) {
        const auto& device = _configuration.get_devices()[i];

        REGISTER_DEVICE_BUTTON(device, i, "My", "my", "mdi:star", My, false);
        REGISTER_DEVICE_BUTTON(device, i, "My (long)", "my_long", "mdi:star", My, true);
        REGISTER_DEVICE_BUTTON(device, i, "Up", "up", "mdi:arrow-up-bold", Up, false);
        REGISTER_DEVICE_BUTTON(device, i, "My Up", "my_up", "mdi:arrow-up-bold-circle", MyUp, false);
        REGISTER_DEVICE_BUTTON(device, i, "Down", "down", "mdi:arrow-down-bold", Down, false);
        REGISTER_DEVICE_BUTTON(device, i, "My Down", "my_down", "mdi:arrow-down-bold-circle", MyDown, false);
        REGISTER_DEVICE_BUTTON(device, i, "Up Down", "up_down", "mdi:arrow-up-down-bold", UpDown, false);
        REGISTER_DEVICE_BUTTON(device, i, "Up Down (long)", "up_down_long", "mdi:arrow-up-down-bold", UpDown, true);
        REGISTER_DEVICE_BUTTON(device, i, "Prog", "prog", "mdi:cog", Prog, false);
        REGISTER_DEVICE_BUTTON(device, i, "Prog (long)", "prog_long", "mdi:cog", Prog, true);
        REGISTER_DEVICE_BUTTON(device, i, "Sun Flag", "sun_flag", "mdi:weather-sunny", SunFlag, false);
        REGISTER_DEVICE_BUTTON(device, i, "Flag", "flag", "mdi:weather-sunny-off", Flag, false);
    }
}

void Application::do_ready() { ESP_ERROR_CHECK(_devices.begin()); }