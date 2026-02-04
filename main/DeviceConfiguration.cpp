#include "support.h"

#include "DeviceConfiguration.h"

#include "esp_mac.h"

LOG_TAG(DeviceConfiguration);

esp_err_t DeviceConfiguration::load(cJSON* data) {
    auto devices = cJSON_GetObjectItemCaseSensitive(data, "devices");
    if (!cJSON_IsArray(devices)) {
        ESP_LOGE(TAG, "Cannot get devices property");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* device = nullptr;
    cJSON_ArrayForEach(device, devices) {
        if (!cJSON_IsObject(device)) {
            ESP_LOGE(TAG, "Device must be an object");
            return ESP_ERR_INVALID_ARG;
        }

        auto device_id = cJSON_GetObjectItemCaseSensitive(device, "id");
        if (!cJSON_IsString(device_id) || !device_id->valuestring) {
            ESP_LOGE(TAG, "Device ID must be a string");
            return ESP_ERR_INVALID_ARG;
        }

        auto device_short_id = cJSON_GetObjectItemCaseSensitive(device, "short_id");
        if (!cJSON_IsString(device_short_id) || !device_short_id->valuestring) {
            ESP_LOGE(TAG, "Device short ID must be a string");
            return ESP_ERR_INVALID_ARG;
        }
        if (strlen(device_short_id->valuestring) > 10) {
            ESP_LOGE(TAG, "Device short ID may not be longer than 10 characters");
            return ESP_ERR_INVALID_ARG;
        }

        auto device_name = cJSON_GetObjectItemCaseSensitive(device, "name");
        if (!cJSON_IsString(device_name) || !device_name->valuestring) {
            ESP_LOGE(TAG, "Device name must be a string");
            return ESP_ERR_INVALID_ARG;
        }

        _devices.push_back(
            RemoteDeviceConfiguration(device_id->valuestring, device_short_id->valuestring, device_name->valuestring));

        ESP_LOGI(TAG, "Device ID %s, name %s", device_id->valuestring, device_name->valuestring);
    }

    return ERR_OK;
}
