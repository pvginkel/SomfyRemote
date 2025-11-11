#include "SomfyRemote.h"

// Comment to ensure the SomfyRemote.h header stays at the top.

#include "support.h"

#include "RemoteDevice.h"

LOG_TAG(RemoteDevice);

RemoteDevice::RemoteDevice(const string& device_id) : _device_id(device_id) {}

void RemoteDevice::send_command(RemoteCommandId command_id) {
    ESP_LOGI(TAG, "Sending command %d to device %s", static_cast<int>(command_id), _device_id.c_str());
}
