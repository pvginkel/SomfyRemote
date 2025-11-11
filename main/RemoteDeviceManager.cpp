#include "ELECHOUSE_CC1101_SRC_DRV.h"

// Comment to ensure that the ELECHOUSE_CC1101_SRC_DRV.h file stays at the top.

#include "support.h"

#include "RemoteDeviceManager.h"

LOG_TAG(RemoteDeviceManager);

esp_err_t RemoteDeviceManager::begin() {
    ESP_LOGI(TAG, "Initializing the CC1101");

    ELECHOUSE_cc1101.setSpiPin(CONFIG_DEVICE_SCK_PIN, CONFIG_DEVICE_GDO1_PIN, CONFIG_DEVICE_MOSI_PIN,
                               CONFIG_DEVICE_CSN_PIN);
    ELECHOUSE_cc1101.setGDO(CONFIG_DEVICE_GDO0_PIN, CONFIG_DEVICE_GDO2_PIN);
    if (!ELECHOUSE_cc1101.Init()) {
        ESP_LOGE(TAG, "Failed to initialize the CC1101");
    }
    ELECHOUSE_cc1101.setMHZ(433.42);

    ESP_LOGI(TAG, "Successfully initialized the CC1101");

    return ESP_OK;
}

void RemoteDeviceManager::set_configuration(DeviceConfiguration* configuration) {
    for (const auto& device : configuration->get_devices()) {
        _devices.push_back(RemoteDevice(device.get_short_id()));
    }
}

void RemoteDeviceManager::send_command(int device_id, RemoteCommandId command_id) {
    if (device_id < 0 || device_id >= _devices.size()) {
        ESP_LOGE(TAG, "Invalid device ID %d", device_id);
    } else {
        ESP_LOGI(TAG, "Sending command %d to device ID %d", static_cast<int>(command_id), device_id);

        ELECHOUSE_cc1101.SetTx();

        _devices[device_id].send_command(command_id);

        ELECHOUSE_cc1101.setSidle();
    }
}
