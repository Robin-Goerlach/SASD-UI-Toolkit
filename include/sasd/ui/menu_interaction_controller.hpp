#pragma once

#include <sasd/ui/menu_bar_navigation.hpp>
#include <sasd/ui/menu_navigation.hpp>
#include <sasd/ui/menu_path.hpp>

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace sasd::ui {

/** High-level outcome produced by one menu-interaction transaction. */
enum class MenuInteractionAction {
    none,
    state_changed,
    activate_command,
    closed,
};

/**
 * Result returned by MenuInteractionController semantic transactions.
 *
 * activate_command carries a lifetime-safe Command::Reference instead of a borrowed MenuItem* or
 * Command*. The caller can therefore finish presentation/focus work after the controller returns and only
 * then execute the command if the semantic object is still alive. Other actions leave command empty.
 */
struct MenuInteractionResult {
    MenuInteractionAction action{MenuInteractionAction::none};
    Command::Reference command{};
};

/**
 * Backend-neutral coordinator for transient menu-bar and popup interaction state.
 *
 * The controller composes the small semantic contracts introduced by MenuBarModel, MenuPath,
 * interpretMenuBarKey(), and interpretMenuPopupKey(). It intentionally owns only transient value
 * state: top-level indices, submenu indices, and selection indices. It stores no MenuModel*, MenuItem*,
 * backend handle, focus object, or presentation object across calls.
 *
 * This makes structural mutation recoverable. Every semantic transaction revalidates the retained state
 * against the supplied MenuBarModel before interpreting input. Invalid submenu routes are truncated to
 * their longest valid prefix; invalid item selections are cleared rather than guessed. If the selected
 * top-level menu itself no longer exists, the whole interaction is closed.
 *
 * Command activation is also deliberately split into two phases. The controller copies a
 * Command::Reference, closes its complete interaction state, and returns activate_command. It never
 * invokes application callbacks itself. Callers may then repaint/dismiss/fix focus and finally execute
 * result.command if it is still live.
 *
 * Root-popup horizontal navigation deliberately coordinates with the menu bar. Left switches to the
 * previous top-level menu and Right switches to the next top-level menu when the currently selected root
 * item is not a submenu. A selected submenu keeps precedence for Right so ordinary submenu entry still
 * works. Switching top-level menus keeps the root popup open but clears its item selection because
 * selection indices belong to the old menu and must never be carried into unrelated semantics.
 *
 * Opening a selected top-level menu with a vertical arrow also spans menu-bar and popup state. Down
 * opens the root popup at its first selectable semantic item, Up opens it at its last selectable item,
 * and Enter opens it without forcing an item selection. The controller derives those edge selections
 * through navigateMenu(), so separators and unavailable command entries are skipped consistently with
 * later popup navigation rather than through a second ad-hoc scanning rule.
 *
 * Pointer geometry remains outside this class. Backends may translate their geometry into the explicit
 * selectPopupItem() and activatePopupItem() semantic transactions, but they cannot inject coordinates,
 * native handles, or retained presentation pointers into Core state. Hover policy, press/release gesture
 * interpretation, mnemonic activation, native menu handles, focus restoration, and concrete popup geometry
 * remain separate layers.
 */
class MenuInteractionController final {
public:
    MenuInteractionController() = default;

    /**
     * Enters menu-bar interaction and selects one top-level menu.
     *
     * A valid preferred index is honored. A missing or stale preferred index selects the first menu.
     * Empty bars cannot become active and leave the controller reset.
     */
    bool begin(const MenuBarModel& bar, std::optional<std::size_t> preferred = std::nullopt) {
        reset();
        if (bar.menuCount() == 0U) {
            return false;
        }

        menu_bar_selection_ = preferred.has_value() && *preferred < bar.menuCount()
                                  ? preferred
                                  : std::optional<std::size_t>{0U};
        return true;
    }

    /** Clears all transient interaction state without touching the semantic menu model. */
    void reset() noexcept {
        menu_bar_selection_.reset();
        popup_path_.reset();
        popup_selections_.clear();
    }

    [[nodiscard]] bool isActive() const noexcept { return menu_bar_selection_.has_value(); }
    [[nodiscard]] bool popupOpen() const noexcept { return popup_path_.has_value(); }

    /** Selected top-level menu while active. */
    [[nodiscard]] std::optional<std::size_t> menuBarSelection() const noexcept {
        return menu_bar_selection_;
    }

    /**
     * Structural path of the deepest open popup.
     *
     * std::nullopt means no popup is open. An engaged empty MenuPath means the root popup belonging to
     * the selected top-level menu is open.
     */
    [[nodiscard]] const std::optional<MenuPath>& popupPath() const noexcept { return popup_path_; }

    /** Selection of the deepest open popup, or std::nullopt when none/no item is selected. */
    [[nodiscard]] std::optional<std::size_t> popupSelection() const noexcept {
        return popup_selections_.empty() ? std::nullopt : popup_selections_.back();
    }

    /** Number of currently represented popup levels; zero when no popup is open. */
    [[nodiscard]] std::size_t popupDepth() const noexcept { return popup_selections_.size(); }

    /**
     * Selects one exact semantic item in an already-open popup level.
     *
     * This is the backend-neutral seam for pointer/accessibility layers that already proved a concrete
     * presentation identity. It deliberately accepts only value indices: terminal cells, pixels, native
     * menu handles, and presentation snapshots remain outside Core interaction state.
     *
     * The retained controller state is normalized against bar before the requested identity is used. A
     * stale level/index, separator, disabled/expired command, or inactive/closed popup is therefore ignored
     * rather than guessed. Such a rejected request returns action==none unless normalization itself repaired
     * state. Callers that own a physical pointer surface may still treat that no-op result as consumed.
     *
     * Selecting an item in an ancestor popup closes deeper popup levels unless the selected item is exactly
     * the submenu item that already owns the next open level. This keeps MenuPath structurally coherent: a
     * child popup may never remain open underneath a different parent selection. Re-selecting the existing
     * parent submenu preserves its already-open descendants and is otherwise a no-op.
     *
     * This transaction selects only. It never opens a submenu and never activates a command; those remain
     * separate semantic operations so pointer press/release/hover policy can be staged without overloading
     * selection with backend-specific gesture meaning.
     */
    [[nodiscard]] MenuInteractionResult selectPopupItem(const MenuBarModel& bar,
                                                        std::size_t level,
                                                        std::size_t item_index) {
        if (!isActive() || !popupOpen()) {
            return {};
        }

        const bool normalized = normalizeAgainst(bar);
        if (!isActive() || !popupOpen()) {
            return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none,
                    {}};
        }

        if (level >= popup_selections_.size()) {
            return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none,
                    {}};
        }

        const MenuModel* const current = popupMenuAtLevel(bar, level);
        if (current == nullptr || item_index >= current->itemCount()) {
            return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none,
                    {}};
        }

        const MenuItem& item = current->itemAt(item_index);
        if (!item.isEnabled()) {
            /*
             * isEnabled() is the existing semantic selectability contract: it rejects separators, disabled
             * or expired Commands, while retaining valid submenu entries. Do not duplicate kind-specific
             * availability policy in the pointer adapter or in this transaction.
             */
            return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none,
                    {}};
        }

        bool changed = normalized || popup_selections_[level] != item_index;
        popup_selections_[level] = item_index;

        const bool preserves_open_child =
            level < popup_path_->size() &&
            (*popup_path_)[level] == item_index &&
            item.kind() == MenuItemKind::submenu &&
            item.submenu() != nullptr;

        if (!preserves_open_child && level + 1U < popup_selections_.size()) {
            /*
             * MenuPath contains exactly one structural item index per transition from a popup level to the
             * next child level. Keeping only `level` path elements therefore leaves levels [0, level] open
             * and removes every descendant that belonged to the old ancestor selection.
             */
            popup_path_->resize(level);
            popup_selections_.resize(level + 1U);
            changed = true;
        }

        return {changed ? MenuInteractionAction::state_changed : MenuInteractionAction::none, {}};
    }

    /**
     * Activates one exact selected command in an already-open popup level.
     *
     * This transaction is intentionally separate from selectPopupItem(). Pointer adapters can therefore use
     * press for selection and release for activation without forcing Core to understand a physical gesture.
     * The request succeeds only when the supplied level is the deepest open popup, the supplied item remains
     * the current semantic selection at that level, and the live item is an enabled Command entry.
     * Submenus, separators, disabled/expired Commands, unrelated selections, and stale indices are rejected.
     *
     * Activation is commit-like, so any normalization repair causes this transaction to fail closed with
     * state_changed instead of activating an identity that came from pre-repair interaction state. A later
     * explicit gesture may activate the repaired model. This is deliberately more conservative than ordinary
     * navigation, where continuing after normalization is harmless.
     *
     * On success the lifetime-safe Command::Reference is captured first, then all transient menu state is
     * closed before activate_command is returned. The controller never invokes Command client code itself;
     * the host can repaint the closed menu and only then execute the returned command.
     */
    [[nodiscard]] MenuInteractionResult activatePopupItem(const MenuBarModel& bar,
                                                          std::size_t level,
                                                          std::size_t item_index) {
        if (!isActive() || !popupOpen()) {
            return {};
        }

        const bool normalized = normalizeAgainst(bar);
        if (!isActive() || !popupOpen()) {
            return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none,
                    {}};
        }

        if (normalized) {
            return {MenuInteractionAction::state_changed, {}};
        }

        if (level >= popup_selections_.size() || level + 1U != popup_selections_.size()) {
            return {};
        }

        const MenuModel* const current = popupMenuAtLevel(bar, level);
        if (current == nullptr || item_index >= current->itemCount()) {
            return {};
        }

        if (!popup_selections_[level].has_value() ||
            *popup_selections_[level] != item_index) {
            return {};
        }

        const MenuItem& item = current->itemAt(item_index);
        if (!item.isEnabled() || item.kind() != MenuItemKind::command) {
            return {};
        }

        Command* const command = item.command();
        if (command == nullptr) {
            return {};
        }

        /*
         * Capture the lifetime-safe semantic reference before reset() destroys every transient index/path.
         * No MenuItem or MenuModel pointer escapes this synchronous transaction.
         */
        Command::Reference command_reference = command->reference();
        reset();
        return {MenuInteractionAction::activate_command, std::move(command_reference)};
    }

    /**
     * Applies one keyboard event to the current interaction state.
     *
     * The controller must first be activated with begin(). Inactive controllers ignore input instead
     * of implicitly stealing arbitrary application keys. Active state is normalized against bar before
     * dispatch so application-side menu rebuilds cannot leave stale structural pointers behind.
     */
    [[nodiscard]] MenuInteractionResult handleKey(const MenuBarModel& bar, const KeyEvent& event) {
        if (!isActive()) {
            return {};
        }

        const bool normalized = normalizeAgainst(bar);
        if (!isActive()) {
            return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none,
                    {}};
        }

        return popupOpen() ? handlePopupKey(bar, event, normalized)
                           : handleMenuBarKey(bar, event, normalized);
    }

private:
    /**
     * Resolves one currently-open popup level against the controller's value state.
     *
     * The helper retains no pointer. Returned MenuModel* values are borrowed only for the immediate caller
     * transaction and are re-derived from the supplied MenuBarModel every time. Bounds/type checks precede
     * each indexed access so a stale route fails with nullptr rather than being reinterpreted.
     *
     * Callers normally invoke this after normalizeAgainst(), but keeping the checks complete makes the
     * helper robust and lets both selectPopupItem() and activatePopupItem() share one structural proof.
     */
    [[nodiscard]] const MenuModel* popupMenuAtLevel(const MenuBarModel& bar,
                                                    std::size_t level) const noexcept {
        if (!menu_bar_selection_.has_value() ||
            *menu_bar_selection_ >= bar.menuCount() ||
            !popup_path_.has_value() ||
            level >= popup_selections_.size()) {
            return nullptr;
        }

        const MenuModel* current = &bar.menuAt(*menu_bar_selection_);
        for (std::size_t depth = 0; depth < level; ++depth) {
            if (depth >= popup_path_->size()) {
                return nullptr;
            }

            const std::size_t parent_index = (*popup_path_)[depth];
            if (parent_index >= current->itemCount()) {
                return nullptr;
            }

            const MenuItem& parent = current->itemAt(parent_index);
            const MenuModel* const child = parent.submenu();
            if (parent.kind() != MenuItemKind::submenu || child == nullptr) {
                return nullptr;
            }

            current = child;
        }

        return current;
    }

    /**
     * Revalidates retained indices after arbitrary semantic-menu mutation.
     *
     * Selection repair is conservative. If an item disappears, becomes disabled, or stops being
     * selectable, the selection becomes empty. We intentionally do not jump to a neighboring item;
     * the next explicit navigation gesture decides where the user moves.
     */
    bool normalizeAgainst(const MenuBarModel& bar) {
        bool changed = false;

        if (!menu_bar_selection_.has_value() || *menu_bar_selection_ >= bar.menuCount()) {
            reset();
            return true;
        }

        if (!popup_path_.has_value()) {
            return false;
        }

        const MenuModel& root = bar.menuAt(*menu_bar_selection_);
        const MenuPath sanitized_path = sanitizeMenuPath(root, *popup_path_);
        if (sanitized_path != *popup_path_) {
            *popup_path_ = sanitized_path;
            changed = true;
        }

        const std::size_t expected_levels = popup_path_->size() + 1U;
        if (popup_selections_.size() != expected_levels) {
            popup_selections_.resize(expected_levels);
            changed = true;
        }

        const MenuModel* current = &root;
        for (std::size_t level = 0; level < expected_levels; ++level) {
            auto& selection = popup_selections_[level];
            if (selection.has_value()) {
                const bool valid_index = *selection < current->itemCount();
                const bool selectable = valid_index && current->itemAt(*selection).isEnabled();
                if (!selectable) {
                    selection.reset();
                    changed = true;
                }
            }

            if (level < popup_path_->size()) {
                /*
                 * sanitizeMenuPath() already proved that this element names a live submenu. Keeping
                 * traversal here explicit avoids retaining that borrowed pointer beyond normalization.
                 */
                current = current->itemAt((*popup_path_)[level]).submenu();
            }
        }

        return changed;
    }

    /**
     * Opens one top-level menu as the root popup with an optional semantic edge selection.
     *
     * The top-level index is already validated by the caller. initial_direction is intentionally a
     * navigation direction rather than a raw item index: navigateMenu() then applies exactly the same
     * selectability policy used by normal popup Up/Down/Home/End navigation. std::nullopt leaves the
     * root popup open with no selected item, which is the Enter behavior and the reset state used when
     * switching horizontally between unrelated top-level menus.
     */
    void openRootPopup(const MenuBarModel& bar,
                       std::size_t top_level_index,
                       std::optional<MenuNavigationDirection> initial_direction = std::nullopt) {
        menu_bar_selection_ = top_level_index;
        popup_path_ = MenuPath{};

        std::optional<std::size_t> initial_selection;
        if (initial_direction.has_value()) {
            initial_selection =
                navigateMenu(bar.menuAt(top_level_index), std::nullopt, *initial_direction);
        }

        popup_selections_.assign(1U, initial_selection);
    }

    [[nodiscard]] MenuInteractionResult handleMenuBarKey(const MenuBarModel& bar,
                                                         const KeyEvent& event,
                                                         bool normalized) {
        const MenuBarKeyResult interpreted = interpretMenuBarKey(bar, menu_bar_selection_, event);

        switch (interpreted.action) {
        case MenuBarKeyAction::select:
            menu_bar_selection_ = interpreted.selection;
            return {MenuInteractionAction::state_changed, {}};

        case MenuBarKeyAction::open_menu: {
            if (!interpreted.selection.has_value()) {
                break;
            }

            std::optional<MenuNavigationDirection> initial_direction;
            if (event.key == Key::down) {
                initial_direction = MenuNavigationDirection::next;
            } else if (event.key == Key::up) {
                initial_direction = MenuNavigationDirection::previous;
            }

            openRootPopup(bar, *interpreted.selection, initial_direction);
            return {MenuInteractionAction::state_changed, {}};
        }

        case MenuBarKeyAction::close_menu_bar:
            reset();
            return {MenuInteractionAction::closed, {}};

        case MenuBarKeyAction::none:
            break;
        }

        return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none, {}};
    }

    /**
     * Switches a currently open root popup to an adjacent top-level menu.
     *
     * The popup remains structurally open, but all item selection is cleared. Reusing the previous
     * root selection would silently reinterpret an index in a different MenuModel, which is exactly the
     * kind of accidental semantic carry-over the value-state design is intended to prevent.
     */
    bool switchRootPopup(const MenuBarModel& bar, MenuNavigationDirection direction) {
        const auto target = navigateMenuBar(bar, menu_bar_selection_, direction);
        if (!target.has_value() || target == menu_bar_selection_) {
            return false;
        }

        openRootPopup(bar, *target);
        return true;
    }

    [[nodiscard]] MenuInteractionResult handlePopupKey(const MenuBarModel& bar,
                                                       const KeyEvent& event,
                                                       bool normalized) {
        const MenuModel& root = bar.menuAt(*menu_bar_selection_);
        const MenuModel* const current = resolveMenuPath(root, *popup_path_);
        if (current == nullptr) {
            /*
             * normalizeAgainst() should make this unreachable for a stable single-threaded model, but
             * failing closed costs almost nothing and keeps this layer robust if future model hooks
             * introduce mutation between normalization and interpretation.
             */
            popup_path_ = MenuPath{};
            popup_selections_.assign(1U, std::nullopt);
            return {MenuInteractionAction::state_changed, {}};
        }

        /*
         * Horizontal movement at the root popup spans two interaction layers, so it belongs here rather
         * than in interpretMenuPopupKey(). Left always means "previous top-level menu" when there is an
         * alternative top-level target. Right normally means "next top-level menu", except that an
         * enabled selected submenu keeps precedence and is entered by the ordinary popup interpreter.
         * Nested popups are deliberately excluded: their Left/Right semantics remain local parent/child
         * navigation and must not unexpectedly jump across the menu bar.
         */
        if (popup_path_->empty() && event.pressed && event.modifiers == KeyModifier::none &&
            bar.menuCount() > 1U) {
            if (event.key == Key::left) {
                if (switchRootPopup(bar, MenuNavigationDirection::previous)) {
                    return {MenuInteractionAction::state_changed, {}};
                }
            } else if (event.key == Key::right) {
                bool selected_submenu = false;
                const auto selection = popup_selections_.back();
                if (selection.has_value() && *selection < current->itemCount()) {
                    const MenuItem& item = current->itemAt(*selection);
                    selected_submenu = item.kind() == MenuItemKind::submenu && item.isEnabled();
                }

                if (!selected_submenu && switchRootPopup(bar, MenuNavigationDirection::next)) {
                    return {MenuInteractionAction::state_changed, {}};
                }
            }
        }

        const MenuPopupKeyResult interpreted =
            interpretMenuPopupKey(*current, popup_selections_.back(), event);

        switch (interpreted.action) {
        case MenuPopupKeyAction::select:
            popup_selections_.back() = interpreted.selection;
            return {MenuInteractionAction::state_changed, {}};

        case MenuPopupKeyAction::open_submenu: {
            if (!interpreted.selection.has_value()) {
                break;
            }

            const auto child = enterMenuSubmenu(root, *popup_path_, *interpreted.selection);
            if (!child.has_value()) {
                break;
            }

            *popup_path_ = *child;
            popup_selections_.push_back(std::nullopt);
            return {MenuInteractionAction::state_changed, {}};
        }

        case MenuPopupKeyAction::close_menu:
            if (!popup_path_->empty()) {
                popup_path_->pop_back();
                popup_selections_.resize(popup_path_->size() + 1U);
            } else {
                popup_path_.reset();
                popup_selections_.clear();
            }
            return {MenuInteractionAction::state_changed, {}};

        case MenuPopupKeyAction::activate_command: {
            if (!interpreted.selection.has_value()) {
                break;
            }

            const MenuItem& item = current->itemAt(*interpreted.selection);
            Command* const command = item.command();
            if (command == nullptr) {
                break;
            }

            /*
             * Capture only the lifetime-safe semantic reference, then close all menu state before the
             * caller is allowed to execute client code. No MenuModel/MenuItem pointer escapes this call.
             */
            Command::Reference command_reference = command->reference();
            reset();
            return {MenuInteractionAction::activate_command, std::move(command_reference)};
        }

        case MenuPopupKeyAction::none:
            break;
        }

        return {normalized ? MenuInteractionAction::state_changed : MenuInteractionAction::none, {}};
    }

    std::optional<std::size_t> menu_bar_selection_{};
    std::optional<MenuPath> popup_path_{};
    std::vector<std::optional<std::size_t>> popup_selections_{};
};

} // namespace sasd::ui
