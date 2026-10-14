#pragma once

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/menu_model.hpp>

#include <gtk/gtk.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sasd::ui::native::gtk::detail {

/**
 * Private, window-local projection of the Core menu model into GTK4.
 *
 * The binding owns the GMenu tree, the action group and the popover menu widget. It copies only
 * structural text and Command::Reference values; MenuBarModel/MenuModel/MenuItem are never retained.
 * GTK owns transient navigation and popup state. This class only translates activation back into the
 * backend EventQueue.
 */
class GtkMenuBinding final {
public:
    GtkMenuBinding() = default;
    ~GtkMenuBinding();

    GtkMenuBinding(const GtkMenuBinding&) = delete;
    GtkMenuBinding& operator=(const GtkMenuBinding&) = delete;

    void setHost(GtkWidget* root_box, EventQueue* events) noexcept;

    /** Builds a complete uninstalled replacement projection. */
    void rebuild(const MenuBarModel& bar);

    /** Installs the already-built menu widget into the host box. */
    void install();

    /** Refreshes live command text/enabled state without reading the semantic structure again. */
    void refresh() noexcept;

    /** Detaches and releases this projection. */
    void clear() noexcept;

    void swap(GtkMenuBinding& other) noexcept;

    // These accessors are intentionally private-adapter surface, not installed public API. They let
    // the GTK-native tests activate the real GAction path without teaching Core about GTK handles.
    [[nodiscard]] GActionGroup* actionGroup() const noexcept {
        return action_group_ == nullptr ? nullptr : G_ACTION_GROUP(action_group_);
    }
    [[nodiscard]] GtkWidget* menuWidget() const noexcept { return menu_widget_; }
    [[nodiscard]] GMenuModel* model() const noexcept {
        return root_model_ == nullptr ? nullptr : G_MENU_MODEL(root_model_);
    }

private:
    struct CallbackState;
    struct Entry {
        GMenu* parent{nullptr};
        gint position{0};
        GSimpleAction* action{nullptr};
        std::string action_name{};
        Command::Reference command{};
    };

    static void activate(GSimpleAction* action, GVariant* parameter, gpointer data) noexcept;
    static void destroyCallbackState(gpointer data) noexcept;

    void appendMenu(GMenu* parent, const MenuModel& model);
    void rebindCallbackState() noexcept;

    GtkWidget* root_box_{nullptr};
    EventQueue* events_{nullptr};
    GMenu* root_model_{nullptr};
    GSimpleActionGroup* action_group_{nullptr};
    GtkWidget* menu_widget_{nullptr};
    std::shared_ptr<CallbackState> callback_state_{};
    std::vector<Entry> entries_{};
    std::uint64_t next_command_id_{1};
    bool installed_{false};
};

} // namespace sasd::ui::native::gtk::detail
