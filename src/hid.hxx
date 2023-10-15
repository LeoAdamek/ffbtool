#pragma once

#include <map>
#include <string>
#include <vector>
#include <thread>

#include <hidapi.h>

namespace HID {

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

    const uint8_t NUM_BUFFERS = 4;
    const size_t BUFFER_SIZE  = 256;

    typedef struct {
        hid_device *device;    
        uint8_t current_buffer;
        unsigned char buffers[NUM_BUFFERS][BUFFER_SIZE];
        size_t buffer_length[NUM_BUFFERS];
    } DeviceInfo;

    class DeviceManager {
        public:
            ~DeviceManager();
            const hid_device_info* get_devices(void);
            const unsigned char * get_latest_report(const hid_device_info *device);
            hid_device* open_device(const hid_device_info *device);
        private:
            hid_device_info *devices;
            size_t device_count;
            std::map<char*,DeviceInfo*> handles;
            std::vector<std::thread> readers;

            bool initialized;
            void init();
            void readLoop(DeviceInfo *device);
    };

    static DeviceManager GlobalDeviceManager;
}