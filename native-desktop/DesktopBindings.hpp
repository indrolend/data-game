#pragma once

#include <array>

// GLFW key values stored as desktop-local preferences. The final slot remains
// serialized only so older progression files keep the same shape.
constexpr std::array<int, 10> DEFAULT_KEYBOARD_BINDINGS{{87, 83, 65, 68, 340, 32, 70, 81, 67, 0}};

inline bool migrateLegacyKeyboardBindings(std::array<int, 10>& bindings) {
    constexpr std::array<int, 10> mistakenNativeDefaults{{87, 83, 65, 68, 340, 32, 67, 81, 86, 70}};
    const bool shiftedSettingsPrefix = bindings[0] >= 0 && bindings[0] <= 2 && bindings[1] >= 0 && bindings[1] <= 2;
    const bool shiftedDefaultTail = bindings[2] == 87 && bindings[3] == 83 && bindings[4] == 65 && bindings[5] == 68 &&
        bindings[6] == 340 && bindings[7] == 32 && (bindings[8] == 67 || bindings[8] == 70) && bindings[9] == 81;
    if (bindings == mistakenNativeDefaults || (shiftedSettingsPrefix && shiftedDefaultTail)) {
        bindings = DEFAULT_KEYBOARD_BINDINGS;
        return true;
    }
    return false;
}
