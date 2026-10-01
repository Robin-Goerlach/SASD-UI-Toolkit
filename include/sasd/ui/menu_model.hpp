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

class MenuModel;

/** Identifies the semantic role of one entry in a MenuModel. */
enum class MenuItemKind {
    command,
    separator,
    submenu,
};

/**
 * One backend-neutral semantic entry in a MenuModel.
 *
 * Command entries intentionally keep only Command::Reference. The menu model therefore neither owns
 * nor extends command lifetime; application Components remain the source of semantic ownership. Text
 * and enabled state are read from the live Command on demand instead of being duplicated into a second
 * cache that would require another synchronization protocol.
 *
 * Submenu entries are different: menu structure is owned by menu structure. A submenu item therefore
 * owns exactly one nested MenuModel through unique_ptr. This gives the recursive semantic tree one
 * unambiguous lifetime root while preserving the existing rule that Commands themselves are never
 * owned by menus.
 *
 * A Shortcut stored here is presentation metadata only. Registering the corresponding gesture remains
 * the responsibility of ShortcutMap at the appropriate application/window/focus scope. Keeping those
 * responsibilities separate prevents merely displaying a menu item from unexpectedly changing input
 * routing policy.
 */
class MenuItem final {
public:
    MenuItem(const MenuItem&) = delete;
    MenuItem& operator=(const MenuItem&) = delete;
    MenuItem(MenuItem&&) noexcept;
    MenuItem& operator=(MenuItem&&) noexcept;
    ~MenuItem();

    /** Creates a semantic command item with optional shortcut-display metadata. */
    [[nodiscard]] static MenuItem command(Command& command,
                                          std::optional<Shortcut> shortcut = std::nullopt);

    /** Creates a non-interactive separator entry. */
    [[nodiscard]] static MenuItem separator() noexcept;

    [[nodiscard]] MenuItemKind kind() const noexcept { return kind_; }

    /** Returns the currently live command for a command entry, or nullptr otherwise. */
    [[nodiscard]] Command* command() const noexcept {
        return kind_ == MenuItemKind::command ? command_.get() : nullptr;
    }

    /** Returns the nested menu for a submenu entry, or nullptr for command/separator entries. */
    [[nodiscard]] MenuModel* submenu() noexcept;
    [[nodiscard]] const MenuModel* submenu() const noexcept;

    /**
     * Returns current user-facing UTF-8 text for this item.
     *
     * Command items expose the live Command text. Submenu items expose their owned MenuModel title.
     * Separators and expired Commands expose an empty view. Returned views are non-owning and must not
     * be retained across later semantic mutation or destruction.
     */
    [[nodiscard]] std::string_view text() const noexcept;

    /**
     * Returns whether the item is semantically selectable.
     *
     * Command entries mirror live Command enabled state. A structurally valid submenu is selectable so
     * a presenter can open it; separators and expired Commands are not. Empty-submenu presentation
     * policy remains a backend/application concern rather than being guessed here.
     */
    [[nodiscard]] bool isEnabled() const noexcept;

    /** Returns optional shortcut-display metadata without registering any input route. */
    [[nodiscard]] const std::optional<Shortcut>& shortcut() const noexcept { return shortcut_; }

    /**
     * Executes the live command represented by this item.
     *
     * Submenus and separators are structural and therefore return false. Expired/disabled Commands
     * also return false. The lifetime-safe reference is copied before entering Command::execute();
     * application code may destroy the Command during its handler, so no Command or MenuItem state is
     * accessed after execute() begins.
     */
    [[nodiscard]] bool activate() const;

private:
    friend class MenuModel;

    MenuItem(MenuItemKind kind,
             Command::Reference command,
             std::optional<Shortcut> shortcut,
             std::unique_ptr<MenuModel> submenu) noexcept;

    /**
     * Creates the owning structural form of a submenu entry.
     *
     * This factory is intentionally private and available only to MenuModel. Public callers build
     * submenu structure through MenuModel::appendSubmenu(), which always allocates a real nested model
     * before the MenuItem is constructed. Keeping the raw ownership factory out of the public API makes
     * the invariant "submenu kind implies a non-null nested MenuModel" unrepresentable through ordinary
     * client code instead of asking every navigation/presentation consumer to defend against a
     * synthetic null-submenu state.
     */
    [[nodiscard]] static MenuItem submenu(std::unique_ptr<MenuModel> submenu) noexcept;

    MenuItemKind kind_{MenuItemKind::separator};
    Command::Reference command_{};
    std::optional<Shortcut> shortcut_{};
    std::unique_ptr<MenuModel> submenu_{};
};

/**
 * Ordered semantic model for one menu surface.
 *
 * MenuModel owns its UTF-8 title and structural item order. Command state remains owned by Command and
 * shortcut routing remains owned by ShortcutMap. Nested submenu structure is recursively owned by the
 * parent MenuItem, so deleting a menu deterministically deletes its complete submenu tree without any
 * separate registry or borrowed structural pointers.
 *
 * Structural mutation may reallocate the internal vector. References returned by itemAt() must
 * therefore not be retained across append/clear operations. References returned by appendSubmenu() are
 * intentionally stronger: the nested MenuModel is individually allocated, so later sibling appends do
 * not relocate it. Such references remain valid until the owning submenu item is removed by clear() or
 * the parent menu is destroyed.
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

    /**
     * Appends and owns a nested submenu, returning it for builder-style population.
     *
     * The nested MenuModel is individually allocated before its owning MenuItem enters the vector.
     * Therefore later sibling appends may move MenuItem values but cannot relocate the nested menu
     * itself. This mirrors MenuBarModel's stable builder-reference rule and keeps recursive setup code
     * predictable without exposing shared ownership.
     *
     * appendSubmenu() is also the sole public construction path for submenu items. It therefore
     * guarantees that every MenuItem whose kind is submenu actually owns a non-null MenuModel; callers
     * cannot manufacture an internally inconsistent submenu entry from a null unique_ptr.
     */
    MenuModel& appendSubmenu(std::string title) {
        auto nested = std::make_unique<MenuModel>(std::move(title));
        MenuModel& result = *nested;
        items_.push_back(MenuItem::submenu(std::move(nested)));
        return result;
    }

    /** Removes all structural entries while preserving the menu title. Owned submenus are destroyed. */
    void clear() noexcept { items_.clear(); }

private:
    std::string title_;
    std::vector<MenuItem> items_;
};

inline MenuItem::MenuItem(MenuItemKind kind,
                          Command::Reference command,
                          std::optional<Shortcut> shortcut,
                          std::unique_ptr<MenuModel> submenu) noexcept
    : kind_{kind},
      command_{std::move(command)},
      shortcut_{std::move(shortcut)},
      submenu_{std::move(submenu)} {}

inline MenuItem::MenuItem(MenuItem&&) noexcept = default;
inline MenuItem& MenuItem::operator=(MenuItem&&) noexcept = default;
inline MenuItem::~MenuItem() = default;

inline MenuItem MenuItem::command(Command& command, std::optional<Shortcut> shortcut) {
    return MenuItem{MenuItemKind::command,
                    command.reference(),
                    std::move(shortcut),
                    nullptr};
}

inline MenuItem MenuItem::separator() noexcept {
    return MenuItem{MenuItemKind::separator, {}, std::nullopt, nullptr};
}

inline MenuItem MenuItem::submenu(std::unique_ptr<MenuModel> submenu) noexcept {
    return MenuItem{MenuItemKind::submenu, {}, std::nullopt, std::move(submenu)};
}

inline MenuModel* MenuItem::submenu() noexcept {
    return kind_ == MenuItemKind::submenu ? submenu_.get() : nullptr;
}

inline const MenuModel* MenuItem::submenu() const noexcept {
    return kind_ == MenuItemKind::submenu ? submenu_.get() : nullptr;
}

inline std::string_view MenuItem::text() const noexcept {
    if (const Command* const live_command = command()) {
        return live_command->text();
    }
    if (const MenuModel* const nested = submenu()) {
        return nested->title();
    }
    return {};
}

inline bool MenuItem::isEnabled() const noexcept {
    if (const Command* const live_command = command()) {
        return live_command->isEnabled();
    }
    return submenu() != nullptr;
}

inline bool MenuItem::activate() const {
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

/**
 * Ordered owner of the top-level menus that make up one semantic menu bar.
 *
 * MenuBarModel deliberately owns MenuModel instances rather than borrowing them. The bar therefore
 * defines the structural lifetime of File/Edit/Help-style top-level menus, while individual command
 * targets remain non-owning Command::Reference objects inside those menus. Each MenuModel may in turn
 * own nested submenus, producing one recursively owned structural tree beneath the bar.
 *
 * Menus are stored behind unique_ptr even though a simple vector<MenuModel> would be smaller. That
 * indirection is intentional architecture rather than optimization: appendMenu() returns a MenuModel&
 * for natural builder-style setup, and growing the vector must not invalidate references to previously
 * appended menus. clear() is the explicit lifetime boundary that destroys every owned menu subtree.
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

    /** Destroys all owned top-level menus/submenus. Commands remain application-owned. */
    void clear() noexcept { menus_.clear(); }

private:
    std::vector<std::unique_ptr<MenuModel>> menus_;
};

} // namespace sasd::ui
