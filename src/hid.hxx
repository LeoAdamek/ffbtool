#pragma once

#include <map>
#include <chrono>
#include <string>
#include <vector>
#include <thread>

#include <hidapi.h>

namespace HID {

    using UpdateRate = std::chrono::duration<std::chrono::system_clock, std::chrono::system_clock::period>;

    typedef enum {
        UNDEFINED = 0x00,
        GENERIC = 0x01,
        SIMULATION = 0x02,
        VIRTUAL_REALITY = 0x03,
        SPORT_CONTROLS = 0x04,
        GAME_CONTROLS = 0x05,
        GENERIC_DEVICE_CONTROLS = 0x06,
        KEYBOARD = 0x07,
        LED = 0x08,
        BUTTON = 0x09,
        ORDINAL = 0x0A,
        TELEPHONY = 0x0B,
        CONSUMER = 0x0C,
        DIGITIZER = 0x0D,
        HAPTICS = 0x0E,
        PID = 0x0F,
        UNICODE = 0x10,
        EYE_HEAD_TRACKER = 0x12,
        AUXILIARY_DISPLAY = 0x14,
        SENSORS = 0x20,
        MEDICAL_INSTRUMENT = 0x40,
        BRAILLE_DISPLAY = 0x41,
        LIGHTING_ILLUMINATION = 0x59,
        BAR_CODE_SCANNER = 0x8C,
    } UsagePage;

    typedef enum {
        SUCCESS,
        ERROR_UNDEFINED
    } InitResult;

    const size_t NUM_BUFFERS = 256;
    const size_t BUFFER_SIZE  = 256;

    typedef struct {
        size_t length;
        unsigned char buffer[BUFFER_SIZE];  
        std::chrono::time_point<std::chrono::system_clock> lru;
    } DeviceBuffer;

    typedef struct {
        hid_device *device;    
        size_t current_buffer;
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

            /**
             * Get the I/O handle for the specified device to interact with it directly.
             */
            hid_device* open_device(const hid_device_info *device);

            void get_update_rate(const hid_device_info *device);
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
             * Devices are quadruple-buffered.
             * When reading a report it is stored in buffer ahead of the
             * one being returned by `DeviceManager::get_latest_report`
             */
            void readLoop(DeviceInfo *device);
    };

    static DeviceManager GlobalDeviceManager;
}