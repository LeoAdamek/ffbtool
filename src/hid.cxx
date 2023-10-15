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
                Device dev(device->path);
                dev.vendor_id = device->vendor_id;
                dev.product_id = device->product_id;
                dev.product_name = std::wstring(device->product_string);
                dev.vendor_name = std::wstring(device->manufacturer_string);
                dev.product_serial = std::wstring(device->product_string);

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

    Device::Device(const char *path) {
        this->path = std::string(path);

        this->bufferSz = 256;
        this->current_buffer = 0;
        
        for (auto i = 0; i < NUM_BUFFERS; i++) {
            this->input_buffers[i] = (unsigned char*)malloc(this->bufferSz);
            memset(this->input_buffers[i], 0, this->bufferSz);
        }
    }

    Device::~Device() {
        /*
        for (auto i = 0; i < NUM_BUFFERS; i++) {
            auto buffer = this->input_buffers[i];

            if (buffer != NULL) free(this->input_buffers[i]);
            this->input_buffers[i] = NULL;
        }
        */
    }

    const unsigned char * Device::GetInput() {
        return this->input_buffers[this->current_buffer];
    }

    std::thread Device::SpawnReader(bool *done) {
        if (!this->IsOpen()) {
            this->Open();
        }

        return std::thread([this]() {
            while(true) {
                this->readNext();
            }
        });
    }

    uint8_t Device::readNext() {
        // Advance and roll over the buffer
        if (++this->current_buffer % NUM_BUFFERS == 0) {
            this->current_buffer = 0;
        }

        unsigned char *buffer = this->input_buffers[this->current_buffer];

        // Reset the buffer
        memset(buffer, 0, this->bufferSz);

        hid_read_timeout(this->handle, buffer, this->bufferSz, 250);

        // Return the current buffer index
        return this->current_buffer;
    }

    size_t Device::Read(char *buffer, size_t size) {

        if (!this->IsOpen()) {
            this->Open();
        }

        return hid_read_timeout(this->handle, (unsigned char*)buffer, size, 100);
    }

    inline bool Device::IsOpen(void) {
        return this->handle != nullptr;
    }

    bool Device::Open(void) {
        if (!this->path.empty()) {
            this->handle = hid_open_path(this->path.c_str());
            hid_set_nonblocking(this->handle, 1);
            return this->handle != nullptr;
        }

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