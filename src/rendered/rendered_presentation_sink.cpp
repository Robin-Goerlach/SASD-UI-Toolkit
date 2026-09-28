#include <sasd/ui/rendered/rendered_presentation_sink.hpp>

#include "rendered_geometry.hpp"
#include "rendered_text_field_viewport.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/widget.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <typeinfo>
#include <vector>

namespace sasd::ui::rendered {
namespace {

void eraseWidget(DisplayList& display_list, Rect bounds, Color background_color) {
    /*
     * The first rendered widgets are intentionally opaque inside their arranged bounds. Repainting
     * the background before current content prevents shorter Label/Button/TextField content from
     * leaving stale glyphs in an incremental frame. Rich background inheritance/compositing is a
     * later theme step.
     */
    display_list.fillRect(bounds, background_color);
}

[[nodiscard]] TextStyle controlTextStyle(const Widget& widget, TextStyle base) noexcept {
    /*
     * Focus and disabled appearance are presentation overlays. Never mutate the semantic style
     * stored by Button/TextField merely because one backend currently has focus.
     */
    if (!widget.isEnabled()) {
        base.dim = true;
    } else if (widget.hasFocus()) {
        base.inverse = true;
    }

    return base;
}

[[nodiscard]] PresentationUpdateResult renderLabel(DisplayList& display_list,
                                                   const Label& label,
                                                   Color background_color) {
    const auto absolute = detail::absoluteRectOf(label);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    eraseWidget(display_list, *absolute, background_color);

    if (label.isVisible() && !absolute->isEmpty()) {
        display_list.drawText(
            {absolute->x, absolute->y},
            label.text(),
            label.textStyle(),
            *absolute);
    }

    return PresentationUpdateResult::synchronized;
}

[[nodiscard]] PresentationUpdateResult renderButton(DisplayList& display_list,
                                                    const Button& button,
                                                    Color background_color) {
    const auto absolute = detail::absoluteRectOf(button);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    if (!button.isVisible() || absolute->isEmpty()) {
        eraseWidget(display_list, *absolute, background_color);
        return PresentationUpdateResult::synchronized;
    }

    /*
     * Preflight all pressed-state coordinate arithmetic before mutating DisplayList. A failure must
     * not erase a previous good frame and then return deferred with no rollback mechanism.
     */
    const auto content = detail::insetOne(*absolute);
    if (!content.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    Point text_origin{content->x, content->y};
    if (button.isPressed() && content->width > 1 && content->height > 1) {
        const auto pressed_x =
            detail::narrowCoordinate(static_cast<std::int64_t>(content->x) + 1);
        const auto pressed_y =
            detail::narrowCoordinate(static_cast<std::int64_t>(content->y) + 1);
        if (!pressed_x.has_value() || !pressed_y.has_value()) {
            return PresentationUpdateResult::deferred;
        }
        text_origin = {*pressed_x, *pressed_y};
    }

    const TextStyle style = controlTextStyle(button, button.textStyle());

    eraseWidget(display_list, *absolute, background_color);
    display_list.strokeRect(*absolute, style.foreground, 1);

    if (!content->isEmpty()) {
        /*
         * One logical unit of caption offset is the first rendered pressed cue. It changes no
         * measurement and introduces no premature theme/brush contract.
         */
        display_list.drawText(text_origin, button.text(), style, *content);
    }

    return PresentationUpdateResult::synchronized;
}

[[nodiscard]] PresentationUpdateResult renderTextField(
    DisplayList& display_list,
    const TextField& field,
    const RenderedMeasurementContext* metrics,
    Color background_color) {
    const auto absolute = detail::absoluteRectOf(field);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    if (!field.isVisible() || absolute->isEmpty()) {
        eraseWidget(display_list, *absolute, background_color);
        return PresentationUpdateResult::synchronized;
    }

    /*
     * The metrics-free constructor remains useful for Window/Label/Button consumers. A visible
     * editable field, however, cannot be acknowledged without a coherent viewport/caret metric.
     */
    if (metrics == nullptr) {
        return PresentationUpdateResult::deferred;
    }

    const auto layout =
        detail::buildTextFieldViewport(field, *absolute, *metrics);
    if (!layout.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    const TextStyle style = controlTextStyle(field, field.textStyle());

    /*
     * Everything fallible was preflighted above. From this point onward the commands form one
     * coherent field repaint: clear old pixels, draw chrome, draw clipped full-context text, then
     * place the caret last so it remains visible over the glyph run.
     */
    eraseWidget(display_list, layout->bounds, background_color);
    display_list.strokeRect(layout->bounds, style.foreground, 1);

    if (!layout->content.isEmpty()) {
        display_list.drawText(
            layout->text_origin,
            field.text(),
            style,
            layout->content);
    }

    if (layout->caret.has_value()) {
        /*
         * A caret is foreground ink even though it happens to use the generic filled-rectangle
         * primitive. Preserve that semantic role so Color::default_color resolves to the device's
         * default foreground rather than disappearing into the control background.
         */
        display_list.fillRect(
            *layout->caret,
            style.foreground,
            FillRole::foreground);
    }

    return PresentationUpdateResult::synchronized;
}

} // namespace

PresentationUpdateResult RenderedPresentationSink::synchronize(const Widget& widget) {
    if (const auto* window = dynamic_cast<const Window*>(&widget)) {
        const auto absolute = detail::absoluteRectOf(*window);
        if (!absolute.has_value()) {
            return PresentationUpdateResult::deferred;
        }

        /*
         * Geometry/structural invalidation reaches the Window as a subtree refresh. Clearing its
         * complete logical surface is safe in that case because PresentationCoordinator then forces
         * every clean descendant to replay. On ordinary incremental child updates we must *not* clear
         * the whole Window, because clean siblings are intentionally not replayed.
         */
        if (window->isSubtreeRefreshPending() || !window->isVisible()) {
            eraseWidget(display_list_, *absolute, background_color_);
        }

        return PresentationUpdateResult::synchronized;
    }

    if (const auto* field = dynamic_cast<const TextField*>(&widget)) {
        return renderTextField(
            display_list_,
            *field,
            measurement_context_,
            background_color_);
    }

    if (const auto* button = dynamic_cast<const Button*>(&widget)) {
        return renderButton(display_list_, *button, background_color_);
    }

    if (const auto* label = dynamic_cast<const Label*>(&widget)) {
        return renderLabel(display_list_, *label, background_color_);
    }

    /*
     * These types are structural in the current rendered model. Exact-type checks matter: silently
     * acknowledging a future Widget subclass would discard its pending visual update before a
     * renderer has learned how to represent it.
     */
    if (typeid(widget) == typeid(Widget) ||
        typeid(widget) == typeid(Container) ||
        dynamic_cast<const VBox*>(&widget) != nullptr ||
        dynamic_cast<const HBox*>(&widget) != nullptr) {
        return PresentationUpdateResult::synchronized;
    }

    return PresentationUpdateResult::deferred;
}

} // namespace sasd::ui::rendered
