#include "hid.hxx"

#include <algorithm>
#include <map>
#include <thread>

#include <assert.h>
#include <wchar.h>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <hidapi.h>

#if _WIN32
    #include <hidapi_winapi.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif


/**
 * Get the size from a value
 * If the value is 3 then the item size is 4
 * Else the item size is the value.
 * 
 * HID Value sizes can only be: 1, 2, or 4 (which is given by the value 3 :Z)
 */
#define HID_ITEM_SIZE(V) ((uint8_t)V) == 3U ? 4 : (uint8_t)V

const auto processor_count = std::thread::hardware_concurrency();

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

        auto i = 0;
        std::vector<std::vector<DeviceInfo*>> device_map(processor_count);
        
        for (auto device = devices; device; device = device->next) {
            hid_device *handle = hid_open_path(device->path);
            DeviceInfo *dev = (DeviceInfo*)malloc(sizeof(DeviceInfo));

            hid_set_nonblocking(handle, 1);
            dev->device = handle;
            dev->current_buffer = 0;

            dev->report_descriptor.length = hid_get_report_descriptor(handle, dev->report_descriptor.data, sizeof(dev->report_descriptor.data));

            handles.emplace(device->path, dev);

            dev->buffers = (DeviceBuffer*)calloc(NUM_BUFFERS, sizeof(DeviceBuffer));
            memset(dev->buffers, 0, NUM_BUFFERS * sizeof(DeviceBuffer));
            
            device_map.at(i % processor_count).push_back(dev);
        }

        for (auto group : device_map) {
            readers.emplace_back(std::thread(&DeviceManager::readLoop, this, group));
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
    }

    const DeviceInfo* DeviceManager::get_device(const hid_device_info *device) {
        std::map<char*, DeviceInfo*>::iterator it = handles.find(device->path);
        return it->second;
    }

    float* DeviceManager::get_input_series(const hid_device_info *device, const Descriptor::Input *input) {
       std::map<char*, DeviceInfo*>::iterator it = handles.find(device->path);

        if (it == handles.end()) {
            return {};
        }

        DeviceInfo *dev = it->second;

        float *values = (float*)malloc(NUM_BUFFERS * sizeof(float));
        memset(values, 0, NUM_BUFFERS * sizeof(float));

        // Input sizes and indices are given as bits rather than bytes.

        size_t value_slot = input->report_index / 8;
        uint8_t bit_offset = input->report_index % 8;

        size_t value_sz = 1; 
        
        if (input->report_size >= 8) {
            value_sz = input->report_size / 8;
        }

        for (size_t i = 0; i < NUM_BUFFERS; i++) {
            int value = 0;

            switch(value_sz) {
                case 4:
                    value = *(uint32_t*)(dev->buffers[i].buffer + value_slot);
                    break;
                case 2:
                    value = *(uint16_t*)(dev->buffers[i].buffer +value_slot);
                    break;
                case 1:
                    value = dev->buffers[i].buffer[value_slot];
                    break;
                default:
                    memcpy(&value, dev->buffers[i].buffer + value_slot, value_sz);
            }

            if (bit_offset != 0) {
                value >>= bit_offset;
            }

            if (input->report_size < 8) {
                // TODO: Bit mask the non-required bytes
                uint8_t mask = 0xFF >> (8 - input->report_size);
                value &= mask;
            }
            
            values[i] = (float)value;
        }

        return values; 
    }

    void DeviceManager::readLoop(std::vector<DeviceInfo*> devices) {
        while(true) {
            auto next_tick = std::chrono::steady_clock::now() + std::chrono::microseconds( SAMPLE_INTERVAL );

            for (auto device : devices) {
                auto next_buffer = device->current_buffer + 1;
                if (next_buffer >= NUM_BUFFERS) next_buffer = 0;

                DeviceBuffer *n = &(device->buffers[next_buffer]);

                n->length = hid_read(device->device, n->buffer, HID::BUFFER_SIZE);
                n->lru = std::chrono::system_clock::now();

                if (n->length < 1) {
                    // If the read timed out, copy the data from the previous buffer
                    memcpy(n, &(device->buffers[device->current_buffer]), sizeof(DeviceBuffer));
                }

                device->current_buffer = next_buffer;
            }

            std::this_thread::sleep_until(next_tick);
        }
    }

}