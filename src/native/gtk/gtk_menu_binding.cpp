#include "gtk_menu_binding.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace sasd::ui::native::gtk::detail {
namespace {

[[nodiscard]] std::string validGtkText(std::string_view text, const char* what) {
    // GTK's menu constructors use NUL-terminated strings. Reject embedded NUL explicitly instead of
    // silently presenting a truncated semantic label.
    if (text.find('\0') != std::string_view::npos ||
        !g_utf8_validate(text.data(), static_cast<gssize>(text.size()), nullptr)) {
        throw std::invalid_argument{std::string{what} + " must be valid UTF-8 without NUL"};
    }
    return std::string{text};
}

[[nodiscard]] std::string actionName(std::uint64_t id) {
    return "command-" + std::to_string(id);
}

} // namespace

void GtkMenuBinding::appendMenu(GMenu* parent, const MenuModel& model) {
    for (std::size_t index = 0; index < model.itemCount(); ++index) {
        const MenuItem& item = model.itemAt(index);
        if (item.kind() == MenuItemKind::separator) {
            GMenu* section = g_menu_new();
            if (section == nullptr) throw std::runtime_error{"g_menu_new failed"};
            g_menu_append_section(parent, nullptr, G_MENU_MODEL(section));
            g_object_unref(section);
            continue;
        }

        const std::string label = validGtkText(item.text(), "GTK menu label");
        if (item.kind() == MenuItemKind::submenu) {
            GMenu* child = g_menu_new();
            if (child == nullptr) throw std::runtime_error{"g_menu_new failed"};
            try {
                appendMenu(child, *item.submenu());
                GMenuItem* submenu = g_menu_item_new_submenu(label.c_str(), G_MENU_MODEL(child));
                if (submenu == nullptr) throw std::runtime_error{"g_menu_item_new_submenu failed"};
                g_menu_append_item(parent, submenu);
                g_object_unref(submenu);
            } catch (...) {
                g_object_unref(child);
                throw;
            }
            g_object_unref(child);
            continue;
        }

        const std::uint64_t id = next_command_id_++;
        if (id == 0) throw std::overflow_error{"GTK menu action identifier space exhausted"};
        const std::string name = actionName(id);
        GSimpleAction* action = g_simple_action_new(name.c_str(), nullptr);
        if (action == nullptr) throw std::runtime_error{"g_simple_action_new failed"};
        const bool enabled = item.isEnabled() && item.command() != nullptr;
        g_simple_action_set_enabled(action, enabled);

        auto state_holder = new std::shared_ptr<CallbackState>{callback_state_};
        g_signal_connect_data(action, "activate", G_CALLBACK(&GtkMenuBinding::activate),
                              state_holder, &GtkMenuBinding::destroyCallbackState, G_CONNECT_DEFAULT);
        g_action_map_add_action(G_ACTION_MAP(action_group_), G_ACTION(action));

        GMenuItem* menu_item = g_menu_item_new(label.c_str(), ("sasdmenu." + name).c_str());
        if (menu_item == nullptr) {
            g_object_unref(action);
            throw std::runtime_error{"g_menu_item_new failed"};
        }
        const gint position = g_menu_model_get_n_items(G_MENU_MODEL(parent));
        g_menu_append_item(parent, menu_item);
        g_object_unref(menu_item);

        Entry entry{parent, position, action, name, {}};
        if (Command* const command = item.command()) entry.command = command->reference();
        entries_.push_back(std::move(entry));
        // The action map owns the action now. The raw pointer in Entry is only an identity within
        // this binding and remains valid until the map is destroyed.
        g_object_unref(action);
    }
}

struct GtkMenuBinding::CallbackState {
    GtkMenuBinding* binding{nullptr};
    EventQueue* events{nullptr};
    bool active{false};
};

GtkMenuBinding::~GtkMenuBinding() { clear(); }

void GtkMenuBinding::setHost(GtkWidget* root_box, EventQueue* events) noexcept {
    root_box_ = root_box;
    events_ = events;
    if (callback_state_ != nullptr) callback_state_->events = events;
}

void GtkMenuBinding::rebuild(const MenuBarModel& bar) {
    clear();
    if (root_box_ == nullptr || events_ == nullptr) {
        throw std::logic_error{"GTK menu binding requires an initialized host"};
    }

    root_model_ = g_menu_new();
    action_group_ = g_simple_action_group_new();
    if (root_model_ == nullptr || action_group_ == nullptr) {
        clear();
        throw std::runtime_error{"GTK menu projection allocation failed"};
    }
    callback_state_ = std::make_shared<CallbackState>();
    callback_state_->binding = this;
    callback_state_->events = events_;
    callback_state_->active = true;
    next_command_id_ = 1;

    try {
        for (std::size_t index = 0; index < bar.menuCount(); ++index) {
            const MenuModel& model = bar.menuAt(index);
            const std::string title = validGtkText(model.title(), "GTK menu title");
            GMenu* popup = g_menu_new();
            if (popup == nullptr) throw std::runtime_error{"g_menu_new failed"};
            try {
                appendMenu(popup, model);
                GMenuItem* top_level = g_menu_item_new_submenu(title.c_str(), G_MENU_MODEL(popup));
                if (top_level == nullptr) throw std::runtime_error{"g_menu_item_new_submenu failed"};
                g_menu_append_item(root_model_, top_level);
                g_object_unref(top_level);
            } catch (...) {
                g_object_unref(popup);
                throw;
            }
            g_object_unref(popup);
        }
        menu_widget_ = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(root_model_));
        if (menu_widget_ == nullptr) throw std::runtime_error{"GtkPopoverMenuBar creation failed"};
        gtk_widget_set_hexpand(menu_widget_, TRUE);
        gtk_widget_set_visible(menu_widget_, TRUE);
    } catch (...) {
        clear();
        throw;
    }
}

void GtkMenuBinding::install() {
    if (menu_widget_ == nullptr || action_group_ == nullptr || root_box_ == nullptr) {
        throw std::logic_error{"GTK menu projection is incomplete"};
    }
    gtk_widget_insert_action_group(menu_widget_, "sasdmenu", G_ACTION_GROUP(action_group_));
    gtk_box_append(GTK_BOX(root_box_), menu_widget_);
    installed_ = true;
}

void GtkMenuBinding::activate(GSimpleAction* action, GVariant*, gpointer data) noexcept {
    auto* const holder = static_cast<std::shared_ptr<CallbackState>*>(data);
    if (holder == nullptr || *holder == nullptr) return;
    CallbackState& state = **holder;
    GtkMenuBinding* const binding = state.binding;
    EventQueue* const events = state.events;
    if (!state.active || binding == nullptr || events == nullptr) return;
    for (const Entry& entry : binding->entries_) {
        if (entry.action != action) continue;
        const Command::Reference command = entry.command;
        Command* const live = command.get();
        if (live == nullptr || !live->isEnabled()) return;
        events->push(Event{CommandActivationEvent{command}});
        return;
    }
}

void GtkMenuBinding::destroyCallbackState(gpointer data, GClosure*) noexcept {
    delete static_cast<std::shared_ptr<CallbackState>*>(data);
}

void GtkMenuBinding::refresh() noexcept {
    for (Entry& entry : entries_) {
        Command* const command = entry.command.get();
        bool enabled = command != nullptr && command->isEnabled();
        if (command != nullptr) {
            const std::string_view text = command->text();
            if (text.find('\0') != std::string_view::npos ||
                !g_utf8_validate(text.data(), static_cast<gssize>(text.size()), nullptr)) {
                enabled = false;
            } else {
                const std::string label{text};
                GMenuItem* replacement = g_menu_item_new(label.c_str(),
                                                          ("sasdmenu." + entry.action_name).c_str());
                if (replacement != nullptr) {
                    g_menu_remove(entry.parent, entry.position);
                    g_menu_insert_item(entry.parent, entry.position, replacement);
                    g_object_unref(replacement);
                } else {
                    enabled = false;
                }
            }
        }
        g_simple_action_set_enabled(entry.action, enabled);
    }
}

void GtkMenuBinding::clear() noexcept {
    if (callback_state_ != nullptr) {
        callback_state_->active = false;
        callback_state_->binding = nullptr;
        callback_state_->events = nullptr;
    }
    if (installed_ && menu_widget_ != nullptr) {
        gtk_widget_insert_action_group(menu_widget_, "sasdmenu", nullptr);
        if (root_box_ != nullptr && gtk_widget_get_parent(menu_widget_) == root_box_) {
            gtk_box_remove(GTK_BOX(root_box_), menu_widget_);
        }
    }
    installed_ = false;
    menu_widget_ = nullptr;
    entries_.clear();
    if (action_group_ != nullptr) g_object_unref(action_group_);
    if (root_model_ != nullptr) g_object_unref(root_model_);
    action_group_ = nullptr;
    root_model_ = nullptr;
    callback_state_.reset();
    next_command_id_ = 1;
}

void GtkMenuBinding::rebindCallbackState() noexcept {
    if (callback_state_ != nullptr) {
        callback_state_->binding = this;
        callback_state_->events = events_;
        callback_state_->active = true;
    }
}

void GtkMenuBinding::swap(GtkMenuBinding& other) noexcept {
    using std::swap;
    swap(root_box_, other.root_box_);
    swap(events_, other.events_);
    swap(root_model_, other.root_model_);
    swap(action_group_, other.action_group_);
    swap(menu_widget_, other.menu_widget_);
    swap(callback_state_, other.callback_state_);
    swap(entries_, other.entries_);
    swap(next_command_id_, other.next_command_id_);
    swap(installed_, other.installed_);
    rebindCallbackState();
    other.rebindCallbackState();
}

} // namespace sasd::ui::native::gtk::detail
