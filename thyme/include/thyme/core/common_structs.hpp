#pragma once

#include <thyme/export_macros.hpp>

#include <stdint.h>

namespace Thyme {

struct THYME_API Resolution {
    uint32_t width;
    uint32_t height;
};

}// namespace Thyme