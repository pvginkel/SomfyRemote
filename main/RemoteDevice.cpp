#include "NVSRollingCodeStorage.h"
#include "SomfyRemote.h"

// Comment to ensure the SomfyRemote.h header stays at the top.

#include "support.h"

#include "RemoteDevice.h"

#define NVS_STORAGE "somfy_remotes"

LOG_TAG(RemoteDevice);

struct SomfyRemoteWrapper {
    NVSRollingCodeStorage code_storage;
    SomfyRemote remote;

    SomfyRemoteWrapper(uint8_t emitter_pin, uint32_t remote, const char* nvs_key)
        : code_storage(NVS_STORAGE, nvs_key), remote(emitter_pin, remote, &code_storage) {}
};

RemoteDevice::RemoteDevice(const string& device_id) : _device_id(device_id) {
    const auto remote_id = get_remote_id();

    ESP_LOGI(TAG, "Assigned remote ID %06" PRIX32 " to device %s", remote_id, _device_id.c_str());

    auto wrapper = new SomfyRemoteWrapper(CONFIG_DEVICE_GDO0_PIN, remote_id, _device_id.c_str());

    wrapper->remote.setup();

    _somfy_remote = wrapper;
}

uint32_t RemoteDevice::get_remote_id() {
    nvs_handle rcs_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_STORAGE, NVS_READWRITE, &rcs_handle));

    const auto key = strformat("%s_id", _device_id.c_str());

    uint32_t remote_id;
    auto err = nvs_get_u32(rcs_handle, key.c_str(), &remote_id);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        remote_id = esp_random() & 0xffffff;
        ESP_ERROR_CHECK(nvs_set_u32(rcs_handle, key.c_str(), remote_id));
    } else {
        ESP_ERROR_CHECK(err);
    }
    ESP_ERROR_CHECK(nvs_commit(rcs_handle));

    return remote_id;
}

void RemoteDevice::send_command(RemoteCommandId command_id, bool long_press) {
    int repeat;
    if (long_press) {
        // I'm really not sure what "long" is. For the Up/Down command 2 seconds seems fine. But
        // for the My command, to switch motor direction, 2 seconds is not enough. Queueing a
        // second long press works sometimes, but not consistently.
        repeat = command_id == RemoteCommandId::My ? SOMFY_MS_TO_ITERS(4000) : SOMFY_MS_TO_ITERS(2000);
    } else {
        repeat = 4;
    }

    ((SomfyRemoteWrapper*)_somfy_remote)->remote.sendCommand(static_cast<Command>(command_id), repeat);
}
