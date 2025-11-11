#pragma once

#include <vector>

#include "DeviceConfiguration.h"
#include "RemoteDevice.h"
#include "freertos/queue.h"

class RemoteDeviceManager {
    vector<RemoteDevice> _devices;
    QueueHandle_t _queue;

public:
    RemoteDeviceManager();

    esp_err_t begin();
    void set_configuration(DeviceConfiguration* configuration);
    bool queue_command(int device_id, RemoteCommandId command_id, bool long_press);

private:
    void task();
    void send_command(int device_id, RemoteCommandId command_id, bool long_press);
};
