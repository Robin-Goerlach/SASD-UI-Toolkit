#pragma once

#include <sasd/ui/command.hpp>
#include <sasd/ui/shortcut.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui {

/** Identifies the semantic role of one entry in a MenuModel. */
enum class MenuItemKind {
    command,
    separator,
};

/**
 * One backend-neutral semantic entry in a MenuModel.
 *
 * Command entries intentionally keep only Command::Reference. The menu model therefore neither owns
 * nor extends command lifetime; application Components remain the source of semantic ownership. Text
 * and enabled state are read from the live Command on demand instead of being duplicated into a second
 * cache that would require another synchronization protocol.
 *
 * A Shortcut stored here is presentation metadata only. Registering the corresponding gesture remains
 * the responsibility of ShortcutMap at the appropriate application/window/focus scope. Keeping those
 * responsibilities separate prevents merely displaying a menu item from unexpectedly changing input
 * routing policy.
 */
class MenuItem final {
public:
    /** Creates a semantic command item with optional shortcut-display metadata. */
    [[nodiscard]] static MenuItem command(Command& command,
                                          std::optional<Shortcut> shortcut = std::nullopt) {
        return MenuItem{MenuItemKind::command, command.reference(), std::move(shortcut)};
    }

    /** Creates a non-interactive separator entry. */
    [[nodiscard]] static MenuItem separator() noexcept {
        return MenuItem{MenuItemKind::separator, {}, std::nullopt};
    }

    [[nodiscard]] MenuItemKind kind() const noexcept { return kind_; }

    /**
     * Returns the currently live command for a command entry, or nullptr otherwise.
     *
     * Command destruction is therefore represented as an unavailable semantic target rather than a
     * dangling pointer. This is particularly useful for menu models whose lifetime exceeds a document,
     * view, or other owner that provided some of their commands.
     */
    [[nodiscard]] Command* command() const noexcept {
        return kind_ == MenuItemKind::command ? command_.get() : nullptr;
    }

    /**
     * Returns the current UTF-8 command text, or an empty view when no live command exists.
     *
     * The returned view follows Command::text() lifetime rules: consume it synchronously and do not
     * retain it across later command mutation/destruction. MenuItem deliberately does not cache text,
     * so Command text changes are immediately observable without a parallel menu-state store.
     */
    [[nodiscard]] std::string_view text() const noexcept {
        const Command* const live_command = command();
        return live_command != nullptr ? live_command->text() : std::string_view{};
    }

    /** Returns true only for a live enabled command entry. */
    [[nodiscard]] bool isEnabled() const noexcept {
        const Command* const live_command = command();
        return live_command != nullptr && live_command->isEnabled();
    }

    /** Returns optional shortcut-display metadata without registering any input route. */
    [[nodiscard]] const std::optional<Shortcut>& shortcut() const noexcept { return shortcut_; }

    /**
     * Executes the live command represented by this item.
     *
     * Separators, expired commands, and disabled commands return false. The lifetime-safe reference is
     * copied before entering Command::execute(); application code may destroy the Command during its
     * handler, so no Command or MenuItem state is accessed after execute() begins.
     */
    [[nodiscard]] bool activate() const {
        if (kind_ != MenuItemKind::command) {
            return false;
        }

        const Command::Reference command_reference = command_;
        Command* const live_command = command_reference.get();
        if (live_command == nullptr) {
            return false;
        }

        return live_command->execute();
    }

private:
    MenuItem(MenuItemKind kind,
             Command::Reference command,
             std::optional<Shortcut> shortcut) noexcept
        : kind_{kind}, command_{std::move(command)}, shortcut_{std::move(shortcut)} {}

    MenuItemKind kind_{MenuItemKind::separator};
    Command::Reference command_{};
    std::optional<Shortcut> shortcut_{};
};

/**
 * Ordered semantic model for one menu surface.
 *
 * MenuModel deliberately models one flat menu at this stage. It owns only its UTF-8 title and entry
 * order; command state remains owned by Command and shortcut routing remains owned by ShortcutMap.
 * This is enough to support File/Edit/Help-style menu surfaces without prematurely defining submenu
 * ownership, popup lifetimes, native menu handles, mnemonic syntax, or platform policy.
 *
 * Structural mutation may reallocate the internal vector. References returned by itemAt() must
 * therefore not be retained across append/clear operations. Presentation code should normally read a
 * stable snapshot synchronously while building or refreshing a backend representation.
 */
class MenuModel final {
public:
    MenuModel() = default;
    explicit MenuModel(std::string title) : title_{std::move(title)} {}

    [[nodiscard]] std::string_view title() const noexcept { return title_; }
    void setTitle(std::string title) { title_ = std::move(title); }

    [[nodiscard]] std::size_t itemCount() const noexcept { return items_.size(); }

    /** Returns one entry by stable order until the next structural mutation. */
    [[nodiscard]] const MenuItem& itemAt(std::size_t index) const {
        if (index >= items_.size()) {
            throw std::out_of_range{"MenuModel item index out of range"};
        }
        return items_[index];
    }

    /** Appends a command item without taking ownership of the command. */
    void appendCommand(Command& command, std::optional<Shortcut> shortcut = std::nullopt) {
        items_.push_back(MenuItem::command(command, std::move(shortcut)));
    }

    /** Appends a semantic separator. Consecutive separators are intentionally permitted for now. */
    void appendSeparator() { items_.push_back(MenuItem::separator()); }

    /** Removes all structural entries while preserving the menu title. */
    void clear() noexcept { items_.clear(); }

private:
    std::string title_;
    std::vector<MenuItem> items_;
};

/**
 * Ordered owner of the top-level menus that make up one semantic menu bar.
 *
 * MenuBarModel deliberately owns MenuModel instances rather than borrowing them. The bar therefore
 * defines the structural lifetime of File/Edit/Help-style top-level menus, while individual command
 * targets remain non-owning Command::Reference objects inside those menus. This keeps the ownership
 * graph explicit: application/component code owns commands; MenuBarModel owns menu structure.
 *
 * Menus are stored behind unique_ptr even though a simple vector<MenuModel> would be smaller. That
 * indirection is intentional architecture rather than optimization: appendMenu() returns a MenuModel&
 * for natural builder-style setup, and growing the vector must not invalidate references to previously
 * appended menus. clear() is the explicit lifetime boundary that destroys every owned menu.
 *
 * Submenus remain outside this first hierarchy slice. A submenu requires an explicit recursive
 * ownership model and presentation/navigation semantics; adding a top-level bar should not silently
 * commit the toolkit to one submenu policy before those requirements are exercised.
 */
class MenuBarModel final {
public:
    MenuBarModel() = default;

    [[nodiscard]] std::size_t menuCount() const noexcept { return menus_.size(); }

    /**
     * Appends and owns a top-level menu.
     *
     * References returned for earlier menus remain valid across later appendMenu() calls because the
     * MenuModel objects themselves are individually allocated. They remain valid until clear() or this
     * MenuBarModel's destruction.
     */
    MenuModel& appendMenu(std::string title) {
        auto menu = std::make_unique<MenuModel>(std::move(title));
        MenuModel& result = *menu;
        menus_.push_back(std::move(menu));
        return result;
    }

    /** Returns one top-level menu in insertion order. */
    [[nodiscard]] MenuModel& menuAt(std::size_t index) {
        if (index >= menus_.size()) {
            throw std::out_of_range{"MenuBarModel menu index out of range"};
        }
        return *menus_[index];
    }

    /** Const overload for presentation code that only reads semantic structure. */
    [[nodiscard]] const MenuModel& menuAt(std::size_t index) const {
        if (index >= menus_.size()) {
            throw std::out_of_range{"MenuBarModel menu index out of range"};
        }
        return *menus_[index];
    }

    /** Destroys all owned top-level menus. Commands remain owned independently by application code. */
    void clear() noexcept { menus_.clear(); }

private:
    std::vector<std::unique_ptr<MenuModel>> menus_;
};

} // namespace sasd::ui
