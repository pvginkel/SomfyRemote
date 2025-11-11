#pragma once

#include "DeviceState.h"
#include "MQTTConnection.h"
#include "RemoteDeviceManager.h"

class Device {
    MQTTConnection& _mqtt_connection;
    DeviceState _state;
    RemoteDeviceManager _devices;

public:
    Device(MQTTConnection& mqtt_connection);

    void begin();
    void set_configuration(DeviceConfiguration* configuration);

private:
    void state_changed();
    void load_state();
    void save_state();
};
