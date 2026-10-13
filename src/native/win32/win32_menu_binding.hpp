#pragma once

#include <sasd/ui/menu_model.hpp>

#include <windows.h>

#include <cstddef>
#include <string>
#include <vector>

namespace sasd::ui::native::win32::detail {

/**
 * Private owned projection of one MenuBarModel into a Win32 HMENU tree.
 *
 * The model remains the only semantic source of truth. This object owns only the native tree and
 * lifetime-safe Command::Reference values needed to translate WM_COMMAND back to Core. Native menu
 * navigation, highlighting, popup opening and dismissal are deliberately left to Windows rather than
 * mirrored through MenuInteractionController.
 */
class Win32MenuBinding final {
public:
    struct Entry {
        HMENU menu{nullptr};
        UINT position{0};
        UINT command_id{0};
        Command::Reference command{};
        std::wstring shortcut_suffix{};
        std::wstring label{};
    };

    Win32MenuBinding() = default;
    ~Win32MenuBinding();

    Win32MenuBinding(const Win32MenuBinding&) = delete;
    Win32MenuBinding& operator=(const Win32MenuBinding&) = delete;

    /** Builds a complete replacement tree before the current tree is discarded. */
    void rebuild(const MenuBarModel& bar);

    /** Refreshes live command text and enabled state without changing menu structure. */
    void refresh() noexcept;

    /** Releases the owned native tree. */
    void clear() noexcept;

    /** Resolves and executes one native command identifier, if it belongs to this binding. */
    bool handleCommand(UINT_PTR command_id) noexcept;

    [[nodiscard]] HMENU root() const noexcept { return root_; }

private:
    HMENU root_{nullptr};
    std::vector<Entry> entries_;
};

} // namespace sasd::ui::native::win32::detail
