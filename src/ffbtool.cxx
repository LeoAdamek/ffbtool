#include <thread>

#include "hid.hxx"
#include "ui/ui.hxx"

int main(const int argc, const char **argv) {
    const hid_device_info *devices = HID::GlobalDeviceManager.get_devices();

    if (UI::InitializeBackend()) {
        UI::Loop();
    }

    //std::terminate();

    return 0;
}