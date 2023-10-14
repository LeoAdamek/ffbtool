#include "hid.hxx"

int main(const int argc, const char **argv) {
    HID::InitResult result = HID::Init();

    if (result != HID::InitResult::SUCCESS) {
        return 1;
    }

    auto ffbDevices = HID::ListDevices(HID::Filters::HasFFB);

    return 0;
}