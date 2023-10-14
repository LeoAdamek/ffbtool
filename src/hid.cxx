#include "hid.hxx"

#include <hidapi.h>
#include <wchar.h>

#include <chrono>

#if _WIN32
    #include <hidapi_winapi.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace HID {

    InitResult Init(void) {
        if (hid_init()) {
            return InitResult::ERROR_UNDEFINED;
        }

        return InitResult::SUCCESS;
    }

    std::vector<Device> ListDevices(void) {
        return ListDevices(nullptr);
    }

    std::vector<Device> ListDevices(DeviceFilterPredicate filter) {
        
        std::string buffer;
        std::wstring wstr;
        hid_device *handle;

        std::vector<Device> devs;

        // Enumerate all devices
        auto devices = hid_enumerate(0x00, 0x00);
        for(auto device = devices->next; device; device = device->next) {
            if ( filter == nullptr || filter(device)) {
                Device dev;
                dev.vendor_id = device->vendor_id;
                dev.product_id = device->product_id;
                dev.product_name = std::wstring(device->product_string);
                dev.vendor_name = std::wstring(device->manufacturer_string);

                devs.push_back(dev);
            }
        }

        hid_free_enumeration(devices);

        return devs;
    }

    namespace Filters {

        bool HasFFB(hid_device_info *device) {
            return device->usage_page == HID::UsagePage::PID;
        }
    }

    std::string Device::Read(size_t size) {

        if (!this->IsOpen()) {
            return std::string();
        }

        unsigned char *buffer = (unsigned char*)malloc(size);
        memset(buffer, 0, size);

        hid_read(this->handle, buffer, size);

        std::string result = std::string((char*)buffer);

        free(buffer);
        return result;
    }

    inline bool Device::IsOpen(void) {
        return this->handle != nullptr;
    }

    bool Device::Open(void) {
        this->handle = hid_open(this->vendor_id, this->product_id, NULL);
        return this->handle != nullptr;
    }

    bool Device::Close(void) {
        if (this->handle != nullptr) {
            hid_close(this->handle);
            return true;
        }
        
        return false;
    }
}