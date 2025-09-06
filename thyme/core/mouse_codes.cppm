export module th.core.mouse_codes;

import std;

namespace th {

export enum class MouseButton: uint8_t {
    button_1 = 0,
    button_2 = 1,
    button_3 = 2,
    button_4 = 3,
    button_5 = 4,
    button_6 = 5,
    button_7 = 6,
    button_8 = 7,
    button_last = button_8,
    button_left = button_1,
    button_right = button_2,
    button_middle = button_3,
};

}// namespace th
