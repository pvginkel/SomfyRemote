#pragma once

#include "DeviceState.h"
#include "MQTTConnection.h"

class Device {
    MQTTConnection& _mqtt_connection;
    DeviceState _state;

public:
    Device(MQTTConnection& mqtt_connection);

    void begin();

private:
    void state_changed();
    void load_state();
    void save_state();
};
