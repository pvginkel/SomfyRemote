#pragma once

#include <vector>

#include "DeviceConfiguration.h"
#include "RemoteDevice.h"

class RemoteDeviceManager {
    vector<RemoteDevice> _devices;

public:
    RemoteDeviceManager() {}

    esp_err_t begin();
    void set_configuration(DeviceConfiguration* configuration);

    void send_command(int device_id, RemoteCommandId command_id);
};
