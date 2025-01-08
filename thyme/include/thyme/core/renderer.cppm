module;

#include <thyme/export_macros.hpp>

export module thyme.core.renderer;

import thyme.core.utils;

namespace Thyme {

export class THYME_API Renderer: public NoCopyable {
public:
    Renderer() = default;
    virtual void draw() = 0;
};
}// namespace Thyme