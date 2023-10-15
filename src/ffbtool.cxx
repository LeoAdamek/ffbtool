#include <spdlog/spdlog.h>

#include "hid.hxx"
#include "ui/ui.hxx"

int main(const int argc, const char **argv) {
    HID::InitResult result = HID::Init();

    if (result != HID::InitResult::SUCCESS) {
        spdlog::critical("Unable to initialize HID. Exiting");
        return 1;
    }

    if (UI::InitializeBackend()) {
        UI::Loop();
        return 0;
    }

    spdlog::critical("Unable to initialize GUI backend");

    return 1;
}