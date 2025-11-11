#pragma once

enum class RemoteCommandId : int {
    My = 0x1,
    Up = 0x2,
    MyUp = 0x3,
    Down = 0x4,
    MyDown = 0x5,
    UpDown = 0x6,
    Prog = 0x8,
    SunFlag = 0x9,
    Flag = 0xA,
    Long = 0x80
};

class RemoteDevice {
    string _device_id;
    void* _somfy_remote;

public:
    RemoteDevice(const string& device_id);

    void send_command(RemoteCommandId command_id, bool long_press);

private:
    uint32_t get_remote_id();
};
