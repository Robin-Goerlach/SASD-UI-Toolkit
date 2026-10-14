#include "win32_menu_binding.hpp"

#include <sasd/ui/shortcut_display.hpp>

#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace sasd::ui::native::win32::detail {
namespace {

[[nodiscard]] std::wstring utf8ToWide(std::string_view value) {
    if (value.empty()) return {};
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error{"Win32 menu text is too long"};
    }
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0) throw std::invalid_argument{"Win32 menu text must be valid UTF-8"};
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), result.data(), length) != length) {
        throw std::invalid_argument{"Win32 menu text must be valid UTF-8"};
    }
    return result;
}

struct ItemLabel {
    std::wstring text{};
    std::wstring shortcut_suffix{};
};

[[nodiscard]] std::wstring escapeAmpersands(std::wstring value) {
    std::wstring escaped;
    escaped.reserve(value.size());
    for (const wchar_t character : value) {
        escaped += character;
        if (character == L'&') escaped += L'&';
    }
    return escaped;
}

[[nodiscard]] ItemLabel itemLabel(const MenuItem& item) {
    std::string shortcut;
    if (item.shortcut().has_value()) {
        shortcut = shortcutDisplayText(*item.shortcut());
    }
    std::wstring label = escapeAmpersands(utf8ToWide(item.text()));
    std::wstring shortcut_suffix = escapeAmpersands(utf8ToWide(shortcut));
    if (!shortcut_suffix.empty()) {
        label += L'\t';
        label += shortcut_suffix;
    }
    return {std::move(label), std::move(shortcut_suffix)};
}

void appendMenu(HMENU menu,
                const MenuModel& model,
                std::vector<Win32MenuBinding::Entry>& entries,
                UINT_PTR& next_command_id) {
    for (std::size_t index = 0; index < model.itemCount(); ++index) {
        const MenuItem& item = model.itemAt(index);
        if (item.kind() == MenuItemKind::separator) {
            if (AppendMenuW(menu, MF_SEPARATOR, 0, nullptr) == FALSE) {
                throw std::runtime_error{"AppendMenuW failed"};
            }
            continue;
        }

        const ItemLabel item_label = itemLabel(item);
        if (item.kind() == MenuItemKind::submenu) {
            HMENU child = CreatePopupMenu();
            if (child == nullptr) throw std::runtime_error{"CreatePopupMenu failed"};
            try {
                appendMenu(child, *item.submenu(), entries, next_command_id);
                if (AppendMenuW(menu, MF_POPUP | MF_STRING,
                                reinterpret_cast<UINT_PTR>(child), item_label.text.c_str()) == FALSE) {
                    throw std::runtime_error{"AppendMenuW failed"};
                }
            } catch (...) {
                // A failed parent append leaves ownership with this transaction.
                DestroyMenu(child);
                throw;
            }
            continue;
        }

        if (next_command_id > static_cast<UINT_PTR>(std::numeric_limits<UINT>::max())) {
            throw std::overflow_error{"Win32 menu command identifier space exhausted"};
        }
        const UINT command_id = static_cast<UINT>(next_command_id++);
        const bool enabled = item.isEnabled() && item.command() != nullptr;
        const UINT flags = MF_STRING | (enabled ? MF_ENABLED : MF_GRAYED);
        if (AppendMenuW(menu, flags, command_id, item_label.text.c_str()) == FALSE) {
            throw std::runtime_error{"AppendMenuW failed"};
        }
        Win32MenuBinding::Entry entry{menu, static_cast<UINT>(GetMenuItemCount(menu) - 1),
                                      command_id, {}, item_label.shortcut_suffix, item_label.text};
        if (Command* const command = item.command()) entry.command = command->reference();
        entries.push_back(std::move(entry));
    }
}

} // namespace

Win32MenuBinding::~Win32MenuBinding() {
    if (root_ != nullptr) DestroyMenu(root_);
}

void Win32MenuBinding::rebuild(const MenuBarModel& bar) {
    HMENU replacement = CreateMenu();
    if (replacement == nullptr) throw std::runtime_error{"CreateMenu failed"};
    std::vector<Entry> replacement_entries;
    UINT_PTR next_command_id = 1;
    try {
        for (std::size_t index = 0; index < bar.menuCount(); ++index) {
            const MenuModel& model = bar.menuAt(index);
            const std::wstring title = escapeAmpersands(utf8ToWide(model.title()));
            HMENU popup = CreatePopupMenu();
            if (popup == nullptr) throw std::runtime_error{"CreatePopupMenu failed"};
            bool attached = false;
            try {
                appendMenu(popup, model, replacement_entries, next_command_id);
                if (AppendMenuW(replacement, MF_POPUP | MF_STRING,
                                reinterpret_cast<UINT_PTR>(popup), title.c_str()) == FALSE) {
                    throw std::runtime_error{"AppendMenuW failed"};
                }
                attached = true;
            } catch (...) {
                // The popup is still unowned by replacement when its top-level append failed.
                if (!attached) DestroyMenu(popup);
                throw;
            }
        }
    } catch (...) {
        DestroyMenu(replacement);
        throw;
    }

    if (root_ != nullptr) DestroyMenu(root_);
    root_ = replacement;
    entries_ = std::move(replacement_entries);
}

void Win32MenuBinding::clear() noexcept {
    if (root_ != nullptr) DestroyMenu(root_);
    root_ = nullptr;
    entries_.clear();
}

void Win32MenuBinding::refresh() noexcept {
    for (Entry& entry : entries_) {
        Command* const command = entry.command.get();
        const bool enabled = command != nullptr && command->isEnabled();
        const UINT native_command_id = entry.command_id;
        EnableMenuItem(entry.menu, native_command_id,
                       MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));

        if (command != nullptr) {
            try {
                std::wstring wide = escapeAmpersands(utf8ToWide(command->text()));
                if (!entry.shortcut_suffix.empty()) {
                    wide += L'\t';
                    wide += entry.shortcut_suffix;
                }
                MENUITEMINFOW info{};
                info.cbSize = sizeof(info);
                info.fMask = MIIM_STRING;
                info.dwTypeData = const_cast<wchar_t*>(wide.c_str());
                if (SetMenuItemInfoW(entry.menu, native_command_id, FALSE, &info) != FALSE) {
                    entry.label = wide;
                }
            } catch (...) {
                // Window/menu callbacks cannot report conversion failures across Win32. Disable the
                // stale native item instead of presenting an apparently invokable command.
                (void)EnableMenuItem(entry.menu, native_command_id, MF_BYCOMMAND | MF_GRAYED);
            }
        }
    }
}

Command::Reference Win32MenuBinding::resolveCommand(UINT_PTR command_id) const noexcept {
    for (const Entry& entry : entries_) {
        if (static_cast<UINT_PTR>(entry.command_id) != command_id) continue;
        if (!entry.command) return {};
        return entry.command;
    }
    return {};
}

void Win32MenuBinding::swap(Win32MenuBinding& other) noexcept {
    using std::swap;
    swap(root_, other.root_);
    swap(entries_, other.entries_);
}

} // namespace sasd::ui::native::win32::detail
