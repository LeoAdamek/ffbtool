#include "hid.hxx"

#include <algorithm>
#include <map>
#include <thread>

#include <fmt/format.h>
#include <fmt/chrono.h>
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
            dev->current_buffer = 0;

            handles.emplace(device->path, dev);

            dev->buffers = (DeviceBuffer*)calloc(NUM_BUFFERS, sizeof(DeviceBuffer));
            memset(dev->buffers, 0, NUM_BUFFERS * sizeof(DeviceBuffer));

            readers.emplace_back(std::thread(&DeviceManager::readLoop, this, dev));
        }

        initialized = true;
    }

    hid_device* DeviceManager::open_device(const hid_device_info *device) {
        std::map<char*, DeviceInfo*>::iterator it = handles.find(device->path);
        return it->second->device;
    }

    const DeviceBuffer* DeviceManager::get_latest_report(const hid_device_info *device) {
        std::map<char*, DeviceInfo*>::iterator it = handles.find(device->path);
        return &(it->second->buffers[it->second->current_buffer]);
    }

    void DeviceManager::get_update_rate(const hid_device_info *device) {
        std::map<char*, DeviceInfo*>::iterator it = handles.find(device->path);
        std::chrono::time_point<std::chrono::system_clock> oldest, newest;
        auto buffers = it->second->buffers;

        oldest = newest = buffers[0].lru;

        for (auto i = 1; i < NUM_BUFFERS; i++) {
            auto lru = buffers[i].lru;

            oldest = lru < oldest ? lru : oldest;
            newest = lru > newest ? lru : newest;
        } 

        auto d = newest - oldest;

        //fmt::print("Update Period: {:}", d);
    }

    void DeviceManager::readLoop(DeviceInfo *device) {
        while(true) {
            auto next_tick = std::chrono::steady_clock::now() + std::chrono::microseconds(1667);

            auto next_buffer = device->current_buffer + 1;
            if (next_buffer >= NUM_BUFFERS) next_buffer = 0;

            DeviceBuffer *n = &(device->buffers[next_buffer]);

            n->length = hid_read_timeout(device->device, n->buffer, HID::BUFFER_SIZE, 500);
            n->lru = std::chrono::system_clock::now();

            if (n->length > 0) {

                device->current_buffer = next_buffer;
            }

            std::this_thread::sleep_until(next_tick);
        }
    }
}