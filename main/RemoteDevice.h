#pragma once

enum class RemoteCommandId {
    My,
    Up,
    MyUp,
    Down,
    MyDown,
    UpDown,
    Prog,
    SunFlag,
    Flag,
};

class RemoteDevice {
    string _device_id;
    void* _somfy_remote;

public:
    RemoteDevice(const string& device_id);

    void send_command(RemoteCommandId command_id);
};
