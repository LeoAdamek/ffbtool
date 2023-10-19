#pragma once

#include <vector>
#include <map>
#include <stdint.h>

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>

#define GET_LOGICAL_MINIMUM(X) (HID::ReportItemTag::GLOBAL_ITEM | 1<<4U | (uint8_t)(X) & SIZE_MASK)

namespace HID {
    namespace Descriptor {

        const uint8_t SIZE_MASK = 0b00000011;
        const uint8_t TYPE_MASK = 0b00001100;
        const uint8_t TAG_MASK =  0b11110000;

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
            BAR_CODE_SCANNER = 0x8C
        } UsagePage;

        typedef uint16_t UsageControlFlags;

        const UsageControlFlags UCF_NONE = 0;

        const UsageControlFlags UCF_LC = 1;       // Linear Control
        const UsageControlFlags UCF_OOC = 1 << 1; // On/Off Control (Toggle)
        const UsageControlFlags UCF_MC = 1 << 2;  // Momentary Control
        const UsageControlFlags UCF_OSC = 1 << 3; // One-Shot Control
        const UsageControlFlags UCF_RTC = 1 << 4; // Re-Trigger Control

        const UsageControlFlags UCF_Sel = 1 << 5; // Selector
        const UsageControlFlags UCF_SV = 1 << 6;  // Static Value
        const UsageControlFlags UCF_SF = 1 << 7;  // Static Flag
        const UsageControlFlags UCF_DV = 1 << 8;  // Dynamic Value
        const UsageControlFlags UCF_DF = 1 << 9;  // Dynamic Flag

        const UsageControlFlags UCF_NAry = 1 << 10; // Named Array
        const UsageControlFlags UCF_CA = 1 << 11;   // Application Collection
        const UsageControlFlags UCF_CL = 1 << 12;   // Logical Collection
        const UsageControlFlags UCF_CP = 1 << 13;   // Physical Collection
        const UsageControlFlags UCF_US = 1 << 14;   // Usage Switch
        const UsageControlFlags UCF_UM = 1 << 15;   // Usage Modifier

        typedef struct UsageDef {
            UsagePage page;
            uint16_t min;
            uint16_t max;
            UsageControlFlags control_type;
            char name[36];
            // const char *name;
        } UsageDef;

        enum class UnitType : uint8_t {
            NONE,
            SI_LINEAR,
            SI_ANGULAR,
            IMPERIAL_LINEAR,
            IMPERIAL_ANGULAR
        };

        typedef enum ReportItemType {
            MAIN_ITEM = 0,
            GLOBAL_ITEM = 4,
            LOCAL_ITEM = 8,
        } ReportItemType;

        typedef enum MainItemTag {
            INPUT = 0b1000,
            OUTPUT = 0b1001,
            FEATURE = 0b1011,
            COLLECTION = 0b1010,
            END_COLLECTION = 0b1100,
        } MainItemTag;

        typedef enum GlobalItemTag {
            USAGE_PAGE,
            LOGICAL_MINIMUM,
            LOGICAL_MAXIMUM,
            PHYSICAL_MINIMUM,
            PHYSICAL_MAXIMUM,
            UNIT_EXPONENT,
            UNIT,
            REPORT_SIZE,
            REPORT_ID,
            REPORT_COUNT,
            PUSH,
            POP,
        } GlobalItemTag;

        enum class LocalItemTag : uint8_t {
            Usage,
            UsageMin,
            UsageMax,
            DesignatorIndex,
            DesignatorMin,
            DesignatorMax,
            StringIndex,
            StringMin,
            StringMax,
            Delimiter,
        };

        enum class CollectionType : uint8_t {
            Physical,
            Application,
            Logical,
            Report,
            NamedArray,
            UsageSwitch,
            UsageModifier,
        };

        /**
         * The bits that define an input's direct properties.
         * 
         * The constants are named after their meaning if the bit is ON.
         * All params have the default state at 0, meaning some bits have
         * a negated meaning.
         */
        enum class InputProperty : uint32_t {
            Constant = 1 << 0,
            Variable = 1 << 1,
            Relative = 1 << 2,
            Wrap = 1 << 3,
            NonLinear = 1 << 4,
            NoPreferredState = 1 << 5,
            NullState = 1 << 6,
            BufferedBytes = 1 << 8
        };

        enum class OutputProperty : uint32_t
        {
            Constant = 1 << 0,
            Variable = 1 << 1,
            Relative = 1 << 2,
            Wrap = 1 << 3,
            NonLinear = 1 << 4,
            NoPreferredState = 1 << 5,
            NullState = 1 << 6,
            Volatile = 1 << 7,
            BufferedBytes = 1 << 8
        };

        typedef struct {
            UsagePage usage_page;
            uint16_t report_id;
            uint32_t usage_id;
            uint32_t designator_index;
            uint32_t string_index;
            uint32_t delimiter;

            // The size of the report data, in bits.
            uint8_t report_size;

            // The start bit of the data
            uint32_t report_index;

            int32_t min_value;
            int32_t max_value;
            std::vector<InputProperty> properties;
        } Input;

        typedef struct OutputFeature {
            UsagePage usage_page;
            uint16_t report_id;
            uint32_t usage_id;
            uint32_t designator_index;
            uint32_t string_index;
            uint32_t delimiter;
            std::vector<OutputProperty> properties;
        } Output, Feature;

        class Descriptor {
            public:
                std::vector<Input> inputs;
        };

        Descriptor parse(const unsigned char *buffer, size_t buffer_sz);

        UsageDef find_usage_definition(uint16_t usage_page, uint16_t usage_id);

        /**
         * A big list of all the usage definition ranges
        */
        const UsageDef usage_definitions[] = {
            // Generic
            {GENERIC, 0x01, 0x01, UCF_CP, "Pointer"},
            {GENERIC, 0x02, 0x02, UCF_CA, "Mouse"},
            {GENERIC, 0x03, 0x03, UCF_NONE, "RESERVED"},
            {GENERIC, 0x04, 0x04, UCF_CA, "Joystick"},
            {GENERIC, 0x05, 0x05, UCF_CA, "Gamepad"},
            {GENERIC, 0x06, 0x06, UCF_CA, "Keyboard"},
            {GENERIC, 0x07, 0x07, UCF_CA, "Keypad"},
            {GENERIC, 0x08, 0x08, UCF_CA, "Multi-Axis Controller"},
            {GENERIC, 0x09, 0x09, UCF_CA, "Tablet PC Controls"},
            {GENERIC, 0x0A, 0x0A, UCF_CA, "Water Cooling Device"},
            {GENERIC, 0x0B, 0x0B, UCF_CA, "Computer Chassis Device"},
            {GENERIC, 0x0C, 0x0C, UCF_CA, "Wireless Radio Controls"},
            {GENERIC, 0x0D, 0x0D, UCF_CA, "Portable Device Controls"},
            {GENERIC, 0x0E, 0x0E, UCF_CA, "System Multi-Controller"},
            {GENERIC, 0x0F, 0x0F, UCF_CA, "Spatial Controller"},
            {GENERIC, 0x10, 0x10, UCF_CA, "Assistive Control"},
            {GENERIC, 0x11, 0x11, UCF_CA, "Device Dock"},
            {GENERIC, 0x13, 0x2F, UCF_NONE, "RESERVED"},
            {GENERIC, 0x30, 0x30, UCF_DV, "X"},
            {GENERIC, 0x31, 0x31, UCF_DV, "Y"},
            {GENERIC, 0x32, 0x32, UCF_DV, "Z"},
            {GENERIC, 0x33, 0x33, UCF_DV, "Rx"},
            {GENERIC, 0x34, 0x34, UCF_DV, "Ry"},
            {GENERIC, 0x35, 0x35, UCF_DV, "Rz"},
            {GENERIC, 0x36, 0x36, UCF_DV, "Slider"},
            {GENERIC, 0x37, 0x37, UCF_DV, "Dial"},
            {GENERIC, 0x38, 0x38, UCF_DV, "Wheel"},
            {GENERIC, 0x39, 0x39, UCF_DV, "Hat Switch"},
            {GENERIC, 0x3a, 0x3a, UCF_CL, "Counted Buffer"},
            {GENERIC, 0x3b, 0x3b, UCF_DV, "Byte Count"},
            {GENERIC, 0x3c, 0x3c, UCF_OSC | UCF_DF, "Motion Wakeup"},
            {GENERIC, 0x3d, 0x3d, UCF_OOC, "Start "},
            {GENERIC, 0x3e, 0x3e, UCF_OOC, "Select"},
            {GENERIC, 0x3f, 0x3f, NULL, "RESERVED"},
            {GENERIC, 0x40, 0x40, UCF_DV, "Vx"},
            {GENERIC, 0x41, 0x41, UCF_DV, "Vy"},
            {GENERIC, 0x42, 0x42, UCF_DV, "Vz"},
            {GENERIC, 0x43, 0x43, UCF_DV, "Vbrx"},
            {GENERIC, 0x44, 0x44, UCF_DV, "Vbry"},
            {GENERIC, 0x45, 0x45, UCF_DV, "Vbrz"},
            {GENERIC, 0x46, 0x46, UCF_DV, "Vno"},
            {GENERIC, 0x47, 0x47, UCF_DV | UCF_DF, "Feature Notification"},
            {GENERIC, 0x48, 0x48, UCF_DV, "Resolution Multiplier"},
            /* TODO: 0x49 - 0x93 */
            {GENERIC, 0x94, 0x94, UCF_MC | UCF_DV, "Index Trigger"},
            {GENERIC, 0x95, 0x95, UCF_MC | UCF_DV, "Palm Trigger"},
            /* TODO: 0x96 ~ 0xD6 */
            {GENERIC, 0xD7, 0xFFFF, NULL, "RESERVED" },

            // VR Controls
            {VIRTUAL_REALITY, 0x01, 0x01, UCF_CA, "Belt" },
            {VIRTUAL_REALITY, 0x02, 0x02, UCF_CA, "Body Suit" },
            {VIRTUAL_REALITY, 0x03, 0x03, UCF_CP, "Flexor" },
            {VIRTUAL_REALITY, 0x04, 0x04, UCF_CA, "Glove" },
            {VIRTUAL_REALITY, 0x05, 0x05, UCF_CP, "Head Tracker" },
            {VIRTUAL_REALITY, 0x06, 0x06, UCF_CA, "Head Mounted Display" },
            {VIRTUAL_REALITY, 0x07, 0x07, UCF_CA, "Hand Tracker" },
            {VIRTUAL_REALITY, 0x08, 0x08, UCF_CA, "Oculometer" },
            {VIRTUAL_REALITY, 0x09, 0x09, UCF_CA, "Vest" },
            {VIRTUAL_REALITY, 0x0A, 0x0A, UCF_CA, "Animatronic Device" },
            {VIRTUAL_REALITY, 0x0B, 0x1F, NULL, "RESERVED" },
            {VIRTUAL_REALITY, 0x20, 0x20, UCF_OOC, "Stereo Enable" },
            {VIRTUAL_REALITY, 0x21, 0x21, UCF_OOC, "Display Enable" },
            {VIRTUAL_REALITY, 0x00, 0xFFFF, NULL, "RESERVED" },

            // Game Controls
            {GAME_CONTROLS, 0x01, 0x01, UCF_CA, "3D Game Controller"},
            {GAME_CONTROLS, 0x02, 0x02, UCF_CA, "Pinball Device"},
            {GAME_CONTROLS, 0x03, 0x03, UCF_CA, "Gun Device"},
            {GAME_CONTROLS, 0x04, 0x1F, NULL, "RESERVED"},
            {GAME_CONTROLS, 0x20, 0x20, UCF_CP, "Point of View"},
            {GAME_CONTROLS, 0x21, 0x21, UCF_DV, "Turn Right/Left"},
            {GAME_CONTROLS, 0x22, 0x22, UCF_DV, "Pitch Forward/Backward"},
            {GAME_CONTROLS, 0x23, 0x23, UCF_DV, "Roll Right/Left"},
            {GAME_CONTROLS, 0x24, 0x24, UCF_DV, "Move Right/Left"},
            {GAME_CONTROLS, 0x25, 0x25, UCF_DV, "Move Forward/Backward"},
            {GAME_CONTROLS, 0x26, 0x26, UCF_DV, "Move Up/Down"},
            {GAME_CONTROLS, 0x27, 0x27, UCF_DV, "Lean Left/Right"},
            {GAME_CONTROLS, 0x28, 0x28, UCF_DV, "Lean Forward/Backward"},
            {GAME_CONTROLS, 0x29, 0x29, UCF_DV, "Height of POV"},
            {GAME_CONTROLS, 0x2A, 0x2A, UCF_MC, "Flipper"},
            {GAME_CONTROLS, 0x2B, 0x2B, UCF_MC, "Secondary Flipper"},
            {GAME_CONTROLS, 0x2C, 0x2C, UCF_MC, "Bump"},
            {GAME_CONTROLS, 0x2D, 0x2D, UCF_OSC, "New Game"},
            {GAME_CONTROLS, 0x2E, 0x2E, UCF_OSC, "Shoot Ball"},
            {GAME_CONTROLS, 0x2F, 0x2F, UCF_OSC, "Player"},

            // Buttons -- They're all just... buttons.
            {BUTTON, 0x00, 0xFFFF, UCF_Sel | UCF_OOC | UCF_MC | UCF_OSC, "Button" },

            // Ordinals
            {ORDINAL, 0x00, 0xFFFF, UCF_UM, "Ordinal Instance"},

            // Consumer Devices
            {CONSUMER, 0x01, 0x01, UCF_CA, "Consumer Control" },
            {CONSUMER, 0x02, 0x02, UCF_NAry, "Numeric Key Pad" },
            {CONSUMER, 0x03, 0x03, UCF_NAry, "Programmable Buttons" },
            {CONSUMER, 0x04, 0x04, UCF_CA, "Microphone" },
            {CONSUMER, 0x05, 0x05, UCF_CA, "Headphone" },
            {CONSUMER, 0x06, 0x06, UCF_CA, "Graphic Equalizer" },
            {CONSUMER, 0x07, 0x1F, UCF_NONE, "RESERVED" },

            // PID (Physical Interface Device)
            {PID, 0x01, 0x01, UCF_CA, "Physical Interface Device"},
            {PID, 0x02, 0x1F, NULL, "RESERVED"},
            {PID, 0x20, 0x20, UCF_DV, "Normal" },
            {PID, 0x21, 0x21, UCF_CA, "Set Effect Report" },
            {PID, 0x22, 0x22, UCF_DV, "Effect Block Index" },
            {PID, 0x23, 0x23, UCF_DV, "Parameter Block Offset" },
            {PID, 0x24, 0x24, UCF_DV, "ROM Flag" },
            {PID, 0x25, 0x25, UCF_CA, "Effect Type" },
            {PID, 0x26, 0x26, UCF_DV, "ET Constant Force" },
            {PID, 0x27, 0x27, UCF_DV, "ET Ramp" },
            {PID, 0x28, 0x28, UCF_DV, "ET Custom Force Data" },
            {PID, 0x29, 0x2F, NULL, "RESERVED" },
            {PID, 0x30, 0x30, UCF_DV, "ET Square" },
            {PID, 0x31, 0x31, UCF_DV, "ET Sine" },
            {PID, 0x32, 0x32, UCF_DV, "ET Triangle" },
            {PID, 0x33, 0x33, UCF_DV, "ET Sawtooth Up" },
            {PID, 0x34, 0x34, UCF_DV, "ET Sawtooth Down" },
            {PID, 0x35, 0x3F, NULL, "RESERVED" },
            {PID, 0x40, 0x40, UCF_DV, "ET Spring" },
            {PID, 0x41, 0x41, UCF_DV, "ET Damper" },
            {PID, 0x42, 0x42, UCF_DV, "ET Inertia" },
            {PID, 0x43, 0x43, UCF_DV, "ET Friction" },
            {PID, 0x44, 0x4F, NULL, "RESERVED" },
            {PID, 0x50, 0x50, UCF_DV, "Duration" },
            {PID, 0x51, 0x51, UCF_DV, "Sample Period" },
            {PID, 0x52, 0x52, UCF_DV, "Gain" },
            {PID, 0x53, 0x53, UCF_DV, "Trigger Button" },
            {PID, 0x54, 0x54, UCF_DV, "Trigger Repeat Interval" },
            {PID, 0x55, 0x55, UCF_CA, "Axes Enable" },
            {PID, 0x56, 0x56, UCF_DV, "Direction Enable" },
            {PID, 0x57, 0x57, UCF_CA, "Direction" },
            {PID, 0x58, 0x58, UCF_CA, "Type Specific Block Offset" },
            {PID, 0x59, 0x59, UCF_CA, "Block Type" },
            {PID, 0x5A, 0x5A, UCF_CA, "Set Envelope Report" },
            {PID, 0x5B, 0x5B, UCF_DV, "Attack Level" },
            {PID, 0x5C, 0x5C, UCF_DV, "Attack Time" },
            {PID, 0x5D, 0x5D, UCF_DV, "Fade Level" },
            {PID, 0x5E, 0x5E, UCF_DV, "Fade Time" },
            {PID, 0x5F, 0x5F, UCF_CA, "Set Condition Report" },
            {PID, 0x60, 0x60, UCF_DV, "CP Offset" },
            {PID, 0x61, 0x61, UCF_DV, "Positive Coefficient" },
            {PID, 0x62, 0x62, UCF_DV, "Negative Coefficient" },
            {PID, 0x63, 0x63, UCF_DV, "Positive Saturation" },
            {PID, 0x64, 0x63, UCF_DV, "Negative Saturation" },
            {PID, 0x65, 0x65, UCF_DV, "Dead Band" },
            {PID, 0x66, 0x66, UCF_CA, "Download Force Sample" },
            {PID, 0x67, 0x67, UCF_DV, "Isoch Custom Force Enable" },
            {PID, 0x68, 0x68, UCF_CA, "Custom Force Data Report" },
            {PID, 0x69, 0x69, UCF_DV, "Custom Force Data" },
            {PID, 0x6A, 0x6A, UCF_DV, "Custom Force Vendor Defined Data"},
            {PID, 0x6B, 0x6B, UCF_CA, "Set Custom Force Report" },
            {PID, 0x6C, 0x6C, UCF_DV, "Custom Force Data Offset" },
            {PID, 0x6D, 0x6D, UCF_DV, "Sample Count" },
            {PID, 0x6E, 0x6E, UCF_CA, "Set Periodic Report" },
            {PID, 0x6F, 0x6F, UCF_DV, "Offset" },
            {PID, 0x70, 0x70, UCF_DV, "Magnitude" },
            {PID, 0x71, 0x71, UCF_DV, "Phase" },
            {PID, 0x72, 0x72, UCF_DV, "Period" },
            {PID, 0x73, 0x73, UCF_CA, "Set Constant Force Report" },
            {PID, 0x74, 0x74, UCF_CA, "Set Ramp Force Report" },
            {PID, 0x75, 0x75, UCF_DV, "Ramp Start" },
            {PID, 0x76, 0x76, UCF_DV, "Ramp End" },
            {PID, 0x77, 0x77, UCF_CA, "Effect Operation Report" },
            {PID, 0x78, 0x78, UCF_CA, "Effect Operation" },
            {PID, 0x79, 0x79, UCF_DV, "Op Effect Start" },
            {PID, 0x7A, 0x7A, UCF_DV, "Op Effect Solo" },
            {PID, 0x7B, 0x7B, UCF_DV, "Op Effect Stop" },
            {PID, 0x7C, 0x7C, UCF_DV, "Loop Count" },
            {PID, 0x7D, 0x7D, UCF_CA, "Device Gain Report" },
            {PID, 0x7E, 0x7E, UCF_DV, "Device Gain" },
            {PID, 0x7F, 0x7F, UCF_CA, "PID Pool Report" },
            {PID, 0x80, 0x80, UCF_DV, "RAM Pool Size" },
            {PID, 0x81, 0x81, UCF_DV, "ROM Pool Size" },
            {PID, 0x82, 0x82, UCF_DV, "ROM Effect Block Size" },
            {PID, 0x83, 0x83, UCF_DV, "Simultaneous Effects Max" },
            {PID, 0x84, 0x84, UCF_DV, "Pool Alignment" },
            {PID, 0x85, 0x85, UCF_CA, "PID Pool Move Report" },
            {PID, 0x86, 0x86, UCF_DV, "Move Source" },
            {PID, 0x87, 0x87, UCF_DV, "Move Destination" },
            {PID, 0x88, 0x88, UCF_DV, "Move Length" },
            {PID, 0x89, 0x89, UCF_CA, "PID Block Load Report" },
            {PID, 0x8A, 0x8A, NULL, "RESERVED" },
            {PID, 0x8B, 0x8B, UCF_CA, "Block Load Status" },
            {PID, 0x8C, 0x8C, UCF_DV, "Block Load Success" },
            {PID, 0x8D, 0x8D, UCF_DV, "Block Load Full" },
            {PID, 0x8E, 0x8E, UCF_DV, "Block Load Error" },
            {PID, 0x8F, 0x8F, UCF_DV, "Block Handle" },
            {PID, 0x90, 0x90, UCF_CA, "PID Block Free Report" },
            {PID, 0x91, 0x91, UCF_CA, "Type Specific Block Handle" },
            {PID, 0x92, 0x92, UCF_CA, "PID State Report" },
            {PID, 0x93, 0x93, NULL, "RESERVED" },
            {PID, 0x94, 0x94, UCF_DV, "Effect Playing" },
            {PID, 0x95, 0x95, UCF_CA, "PID Device Control Report" },
            {PID, 0x96, 0x96, UCF_CA, "PID Device Control" },
            {PID, 0x97, 0x97, UCF_DV, "DC Enable Actuators" },
            {PID, 0x98, 0x98, UCF_DV, "DC Disable Actuators" },
            {PID, 0x99, 0x99, UCF_DV, "DC Stop All Effects" },
            {PID, 0x9A, 0x9A, UCF_DV, "DC Device Reset" },
            {PID, 0x9B, 0x9B, UCF_DV, "DC Device Pause" },
            {PID, 0x9C, 0x9C, UCF_DV, "DC Device Continue" },
            {PID, 0x9D, 0x9E, NULL, "RESERVED" },
            {PID, 0x9F, 0x9F, UCF_DV, "Device Paused" },
            {PID, 0xA0, 0xA0, UCF_DV, "Actuators Enabled" },
            {PID, 0xA1, 0xA3, NULL, "RESERVED" },
            {PID, 0xA4, 0xA4, UCF_DV, "Safety Switch" },
            {PID, 0xA5, 0xA5, UCF_DV, "Actuator Override Switch" },
            {PID, 0xA6, 0xA6, UCF_DV, "Actuator Power" },
            {PID, 0xA7, 0xA7, UCF_DV, "Start Delay" },
            {PID, 0xA8, 0xA8, UCF_CA, "Parameter Block Size" },
            {PID, 0xA9, 0xA9, UCF_DV, "Device Managed Pool" },
            {PID, 0xAA, 0xAA, UCF_DV, "Shared Parameter Blocks" },
            {PID, 0xAB, 0xAB, UCF_CA, "Create New Effect Report" },
            {PID, 0xAC, 0xAC, UCF_DV, "RAM Pool Available" },
            {PID, 0xAB, 0xFFFF, NULL, "RESERVED"}
        };

        const size_t USAGES_LENGTH = sizeof(usage_definitions) / sizeof(UsageDef);
    }
}