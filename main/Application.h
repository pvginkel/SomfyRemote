#pragma once

#include "ApplicationBase.h"
#include "DeviceConfiguration.h"
#include "MQTTConnection.h"
#include "RemoteDeviceManager.h"

class Application : public ApplicationBase {
    DeviceConfiguration _configuration;
    RemoteDeviceManager _devices;

protected:
    void do_begin() override;
    void do_ready() override;
    void do_configuration_loaded(cJSON* data) override;

private:
    void state_changed();
    void publish_mqtt_discovery();
};
