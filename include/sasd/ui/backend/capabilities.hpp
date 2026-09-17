#pragma once

namespace sasd::ui {

/**
 * Describes optional backend facilities.
 *
 * Portability does not mean pretending every environment can do the same thing. A terminal,
 * rendered desktop backend and native desktop backend can report different capabilities while
 * sharing the same semantic component API.
 */
struct BackendCapabilities {
    bool pointer_input{false};
    bool clipboard{false};
    bool true_color{false};
    bool native_menus{false};
    bool multiple_windows{false};
    bool drag_and_drop{false};
    bool ime{false};
    bool accessibility{false};
    bool system_tray{false};
    bool notifications{false};
    bool printing{false};

    friend constexpr bool operator==(const BackendCapabilities&, const BackendCapabilities&) = default;
};

} // namespace sasd::ui
