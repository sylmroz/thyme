module;

#include "thyme/export_macros.hpp"

#include <ostream>
#include <variant>

export module thyme.core.event;

export namespace Thyme {

struct THYME_API WindowResize {
    int width;
    int height;

    std::string toString() {
        return std::format("WindowResize {{ width:{} height:{} }}", width, height);
    }
};

struct THYME_API WindowClose {};

struct THYME_API MouseMove {
    int x;
    int y;
};

struct THYME_API MouseButtonDown {};

struct THYME_API MouseButtonUp {};

struct THYME_API MouseWheel {};

struct THYME_API KeyPressed {};

struct THYME_API KeyPressedRepeated {};

struct THYME_API KeyReleased {};

using WindowEvent = std::variant<WindowResize, WindowClose>;

using MouseEvent = std::variant<MouseMove, MouseButtonDown, MouseButtonUp>;

using KeyEvent = std::variant<KeyPressed, KeyReleased>;


}// namespace Thyme