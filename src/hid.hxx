#pragma once

#include <map>
#include <chrono>
#include <string>
#include <vector>
#include <thread>

#include "hid_descriptor.hxx"

#include <hidapi.h>

namespace HID {

    using UpdateRate = std::chrono::duration<std::chrono::system_clock, std::chrono::system_clock::period>;

    typedef enum {
        SUCCESS,
        ERROR_UNDEFINED
    } InitResult;

    // Sample Interval in μs
    const size_t SAMPLE_INTERVAL = 8333;

    // Number of buffers to store. Total size requirement is NUM_BUFFERS * (BUFFER_SIZE + 16)
    const size_t NUM_BUFFERS  = 1200; //2000 / SAMPLE_INTERVAL;

    // Size of each buffer.
    const size_t BUFFER_SIZE  = 256;

    typedef struct {
        int length;
        unsigned char buffer[BUFFER_SIZE];  
        std::chrono::time_point<std::chrono::system_clock> lru;
    } DeviceBuffer;

    typedef struct {
        hid_device *device;    
        size_t current_buffer;
        struct {
            size_t length;
            unsigned char data[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
        } report_descriptor;
        DeviceBuffer *buffers;
    } DeviceInfo;

    class DeviceManager {
        public:
            ~DeviceManager();

            /**
             * Get the list of devices.
             *
             * Returns a pointer to the start of a single-linked list of devices.
             * This list is shared with the device manager and should be immutable.
             *
             * If the device manager has not yet been initialized, it will be initialized here.
             */
            const hid_device_info* get_devices(void);

            /**
             * Get the latest report for the given device.
             */
            const DeviceBuffer* get_latest_report(const hid_device_info *device);

            const DeviceInfo* get_device(const hid_device_info *device);

            /**
             * Get the I/O handle for the specified device to interact with it directly.
             */
            hid_device* open_device(const hid_device_info *device);

            void get_update_rate(const hid_device_info *device);

            /**
             * 
             */
            float* get_input_series(const hid_device_info *device, const Descriptor::Node *input);
        private:

            /**
             * A single-linked list of HID interfaces
             */
            hid_device_info *devices;

            /**
             * The number of devices
             */
            size_t device_count;

            /**
             * The raw IO handles to each device
             */
            std::map<char*,DeviceInfo*> handles;

            /**
             * A pool of worker threads which poll the devices for inputs
            */
            std::vector<std::thread> readers;

            bool initialized;
            
            /**
             * Initializer
            */
            void init();

            /**
             * Timer loop to read a device.
             *
             * Devices are multi-buffered.
             * When reading a report it is stored in buffer ahead of the
             * one being returned by `DeviceManager::get_latest_report`
             */
            void readLoop(std::vector<DeviceInfo*> devices);
    };

    static DeviceManager GlobalDeviceManager;

}