module;

#include <ostream>
#include <thyme/export_macros.hpp>

#include <variant>

export module event;

namespace Thyme {

export struct THYME_API WindowResize {
    int width;
    int height;

    std::string toString() {
        return std::format("WindowResize {{ width:{} height:{} }}", width, height);
    }
};

export struct THYME_API WindowClose {};

export struct THYME_API MouseMove {
    int x;
    int y;
};

export struct THYME_API MouseButtonDown {};

export struct THYME_API MouseButtonUp {};

export struct THYME_API MouseWheel {};

export struct THYME_API KeyPressed {};

export struct THYME_API KeyPressedRepeated {};

export struct THYME_API KeyReleased {};

export using WindowEvent = std::variant<WindowResize, WindowClose>;

export using MouseEvent = std::variant<MouseMove, MouseButtonDown, MouseButtonUp>;

export using KeyEvent = std::variant<KeyPressed, KeyReleased>;


}// namespace Thyme