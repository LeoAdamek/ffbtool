#pragma once

#include <vector>
#include <string>
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

    class Device {
    public:
        Device(const char*);
        ~Device();
        uint16_t vendor_id;
        uint16_t product_id;
        hid_device *handle = nullptr;

        std::wstring product_serial;
        std::wstring vendor_name;
        std::wstring product_name;

        size_t Read(char *buffer, size_t buffer_sz);

        bool IsOpen(void);
        bool Open(void);
        bool Close(void);

        std::thread SpawnReader(bool *done);

        const unsigned char* GetInput(void);
    private:
        std::string path;
        uint32_t id;

        /**
         * Data is read into a series of buffers so that each buffer remains static for multiple read cycles.
         * Giving consuming applications more time to use, or copy, the input data without it changing as it is being read.
        */
        unsigned char *input_buffers[NUM_BUFFERS];
        size_t bufferSz;
        uint8_t current_buffer;

        uint8_t readNext();
    };

    InitResult Init(void);

    /**
     * DeviceFilterPredicate is a function which takes an `hid_device_info*`
     * and returns a boolean. Used to filter devices returned by `ListDevices`
    */
    typedef bool (*DeviceFilterPredicate)(hid_device_info *device);

    /**
     * List devices matching the given predicate function.
    */
    std::vector<Device> ListDevices(DeviceFilterPredicate);

    /**
     * List all devices.
    */
    std::vector<Device> ListDevices(void);

    namespace Filters {
        bool HasFFB(hid_device_info*);        
    }
}