#pragma once

#include <vector>

#include "cJSON.h"

class RemoteDeviceConfiguration {
    string _id;
    string _short_id;
    string _name;

public:
    RemoteDeviceConfiguration(const string& id, const string& short_id, const string& name)
        : _id(id), _short_id(short_id), _name(name) {}

    const string& get_id() const { return _id; }
    const string& get_short_id() const { return _short_id; }
    const string& get_name() const { return _name; }
};

class DeviceConfiguration {
    vector<RemoteDeviceConfiguration> _devices;

public:
    DeviceConfiguration() = default;
    DeviceConfiguration(const DeviceConfiguration&) = delete;
    DeviceConfiguration& operator=(const DeviceConfiguration&) = delete;
    DeviceConfiguration(DeviceConfiguration&&) = delete;
    DeviceConfiguration& operator=(DeviceConfiguration&&) = delete;

    esp_err_t load(cJSON* data);

    const vector<RemoteDeviceConfiguration>& get_devices() const { return _devices; }
};
