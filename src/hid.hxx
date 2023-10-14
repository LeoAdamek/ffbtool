#pragma once

#include <vector>
#include <string>

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

    class Device {
    public:
        uint16_t vendor_id;
        uint16_t product_id;

        std::wstring vendor_name;
        std::wstring product_name;

        std::string Read(size_t size);

        bool IsOpen(void);
        bool Open(void);
        bool Close(void);
    private:
        hid_device *handle = nullptr;
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