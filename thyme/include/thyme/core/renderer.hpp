#pragma once

#include <thyme/export_macros.hpp>

#include <thyme/core/utils.hpp>

namespace Thyme {

class THYME_API Renderer: public NoCopyable {
public:
    Renderer() = default;
    virtual void draw() = 0;
};
}// namespace Thyme