#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/rendered/menu_frame_presentation.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui::rendered {

/**
 * Rendered translation layer for the already-built menu frame.
 *
 * This class retains only popup-level/row value identities for a possible Primary click. It does not
 * retain a frame, MenuModel, MenuItem, Command or renderer pointer. Command activation is two-phase:
 * the controller closes transient state and returns Command::Reference; the host executes it later,
 * after presentation and focus state have been stabilized.
 */
class RenderedMenuPointerInteraction final {
public:
    class GestureState final {
    public:
        void reset() noexcept { pressed_.reset(); }
        [[nodiscard]] bool armed() const noexcept { return pressed_.has_value(); }

    private:
        friend class RenderedMenuPointerInteraction;
        std::optional<RenderedMenuHit> pressed_{};
    };

    RenderedMenuPointerInteraction() = delete;

    /** Pointer-surface leave is a lifecycle boundary and retires any armed release identity. */
    static void handleSurfaceEvent(const PointerSurfaceEvent& event,
                                   GestureState& gesture) noexcept {
        if (event.action == PointerSurfaceAction::left) {
            gesture.reset();
        }
    }

    /**
     * Translates one pointer event into a semantic controller transaction.
     *
     * An engaged result means the active menu surface consumed the event, including outside presses and
     * unavailable rows, so the host must not route that same physical event through covered widgets.
     */
    [[nodiscard]] static std::optional<MenuInteractionResult>
    handle(const MenuBarModel& bar,
           MenuInteractionController& controller,
           const RenderedMenuFramePresentationSnapshot& frame,
           const PointerEvent& event,
           GestureState& gesture) {
        const bool active_before = controller.isActive();
        const auto hit = menuFrameHitAt(frame, event.position);
        const bool primary_press = event.action == PointerAction::press &&
                                   event.button == PointerButton::primary;
        const bool primary_release = event.action == PointerAction::release &&
                                     event.button == PointerButton::primary;

        if (event.action == PointerAction::move) {
            if (!active_before) return std::nullopt;
            if (hit.has_value()) {
                return applyHover(bar, controller, *hit);
            }
            return MenuInteractionResult{};
        }

        if (primary_press) {
            gesture.reset();
            if (!hit.has_value()) {
                if (!active_before) return std::nullopt;
                controller.reset();
                return std::optional<MenuInteractionResult>{
                    MenuInteractionResult{MenuInteractionAction::closed, {}}};
            }

            if (hit->kind == RenderedMenuHit::Kind::menu_bar) {
                if (hit->index >= bar.menuCount() || !controller.begin(bar, hit->index)) {
                    return MenuInteractionResult{};
                }
                return controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
            }

            if (!active_before) return std::nullopt;
            const MenuInteractionResult selected = controller.selectPopupItem(
                bar, hit->level, hit->index);
            gesture.pressed_ = *hit;
            return selected;
        }

        if (primary_release) {
            if (!active_before) return std::nullopt;
            const auto pressed = gesture.pressed_;
            gesture.reset();
            if (!pressed.has_value() || !hit.has_value() || *pressed != *hit ||
                pressed->kind != RenderedMenuHit::Kind::popup_row) {
                return MenuInteractionResult{};
            }

            const MenuInteractionResult submenu = controller.openPopupSubmenu(
                bar, pressed->level, pressed->index);
            if (submenu.action != MenuInteractionAction::none) return submenu;

            // activatePopupItem() closes the menu before returning a Command::Reference. No application
            // callback is entered here, and this adapter performs no member access after that boundary.
            return controller.activatePopupItem(bar, pressed->level, pressed->index);
        }

        if (!active_before) return std::nullopt;
        return MenuInteractionResult{};
    }

private:
    [[nodiscard]] static MenuInteractionResult
    applyHover(const MenuBarModel& bar,
               MenuInteractionController& controller,
               const RenderedMenuHit& hit) {
        if (hit.kind == RenderedMenuHit::Kind::menu_bar) {
            if (hit.index >= bar.menuCount()) return {};
            if (controller.menuBarSelection() == std::optional<std::size_t>{hit.index} &&
                controller.popupOpen()) {
                return {};
            }
            if (!controller.begin(bar, hit.index)) return {};
            return controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
        }
        return controller.selectPopupItem(bar, hit.level, hit.index);
    }
};

} // namespace sasd::ui::rendered
