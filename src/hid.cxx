#include "hid.hxx"

#include <map>
#include <thread>
#include <wchar.h>

#include <hidapi.h>

#if _WIN32
    #include <hidapi_winapi.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace HID {

    DeviceManager::~DeviceManager() {
        if (this->devices != nullptr) {
            hid_free_enumeration(this->devices);
            this->devices = nullptr;
            this->initialized = false;

            for (auto dev : this->handles) {
                hid_close(dev.second->device);
                free(dev.second);
                //this->handles.erase(dev.first);
            }
        }
    }

    const hid_device_info * DeviceManager::get_devices() {
        if (!this->initialized) {
            this->init();
        }

        return this->devices;
    }

    void DeviceManager::init() {
        hid_init();
        //devices = hid_enumerate(0x16d0, 0x0d60);
        devices = hid_enumerate(0x00, 0x00);

        for (auto device = devices; device; device = device->next) {
            hid_device *handle = hid_open_path(device->path);
            DeviceInfo *dev = (DeviceInfo*)malloc(sizeof(DeviceInfo));

            dev->device = handle;

            handles.emplace(device->path, dev);

            for (auto i = 0; i < NUM_BUFFERS; i++) {
                memset(dev->buffers[i], 0, BUFFER_SIZE);
            }

            readers.emplace_back(std::thread(&DeviceManager::readLoop, this, dev));
        }


        initialized = true;
    }

    hid_device* DeviceManager::open_device(const hid_device_info *device) {
        std::map<char*, DeviceInfo*>::iterator it = this->handles.find(device->path);
        return it->second->device;
    }

    const unsigned char * DeviceManager::get_latest_report(const hid_device_info *device) {
        std::map<char*, DeviceInfo*>::iterator it = this->handles.find(device->path);
        return it->second->buffers[it->second->current_buffer];
    }

    void DeviceManager::readLoop(DeviceInfo *device) {
        while(true) {
            auto next_buffer = device->current_buffer + 1;
            if (next_buffer > NUM_BUFFERS) next_buffer = 0;
            size_t l = hid_read_timeout(device->device, device->buffers[next_buffer], HID::BUFFER_SIZE, 500);
            device->current_buffer = next_buffer;

            if (l > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
}