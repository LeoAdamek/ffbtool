#pragma once

#include <stdint.h>
#include <string>

namespace HID
{
    class Unit {
        public:
            Unit(int32_t unit, int32_t exp);
            std::string to_string();
    };
};
