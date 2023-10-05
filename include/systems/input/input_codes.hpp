#pragma once

#include "defines.hpp"

namespace ENGINE_NAMESPACE {

typedef uint8 InputCode;

/**
 * @brief Input codes for keyboard
 */
enum class KeyCode : InputCode {
    // Number pad
    NUM_LOCK,
    NUM_PAD_0,
    NUM_PAD_1,
    NUM_PAD_2,
    NUM_PAD_3,
    NUM_PAD_4,
    NUM_PAD_5,
    NUM_PAD_6,
    NUM_PAD_7,
    NUM_PAD_8,
    NUM_PAD_9,
    NUM_PAD_ADD,
    NUM_PAD_SUBTRACT,
    NUM_PAD_MULTIPLY,
    NUM_PAD_DIVIDE,
    NUM_PAD_DECIMAL,
    NUM_PAD_EQUAL,
    NUM_PAD_ENTER,

    // Top row numbers
    NUM_0,
    NUM_1,
    NUM_2,
    NUM_3,
    NUM_4,
    NUM_5,
    NUM_6,
    NUM_7,
    NUM_8,
    NUM_9,
    GRAVE, // "`" ("~")
    EQUAL, // "=" ("+")
    MINUS, // "-" ("_")

    // Alphabet
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // Punctuation
    LEFT_BRACKET,  // "["
    RIGHT_BRACKET, // "]"
    BACKSLASH,     // "\"
    SLASH,         // "/"
    COMMA,         // ","
    PERIOD,        // "."
    SEMICOLON,     // ";"
    APOSTROPHE,    // "'"

    // F row
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    // Additional F keys
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,

    // Navigation keys
    UP,
    DOWN,
    LEFT,
    RIGHT,
    HOME,
    END,
    PAGE_UP,
    PAGE_DOWN,

    // Special
    LEFT_ALT,
    LEFT_CONTROL,
    LEFT_SHIFT,
    LEFT_SUPER,

    RIGHT_ALT,
    RIGHT_CONTROL,
    RIGHT_SHIFT,
    RIGHT_SUPER,

    SPACE,
    TAB,
    ENTER,
    ESCAPE,
    BACKSPACE,
    CAPITAL, // "Caps lock"

    INSERT,
    DELETE,
    PRINT,
    PAUSE, // "Break"
    SCROLL_LOCK,

    // MAX
    MAX_KEY_CODE
};

} // namespace ENGINE_NAMESPACE