#include <sasd/ui/rendered/rendered_presentation_sink.hpp>

#include "rendered_geometry.hpp"
#include "rendered_text_field_viewport.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
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

[[nodiscard]] RenderedThemeMetrics activeThemeMetrics(
    const RenderedMeasurementContext* metrics) noexcept {
    return metrics != nullptr
               ? metrics->themeMetrics().normalized()
               : RenderedThemeMetrics{}.normalized();
}

[[nodiscard]] PresentationUpdateResult renderButton(
    DisplayList& display_list,
    const Button& button,
    const RenderedMeasurementContext* metrics,
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
    const RenderedThemeMetrics theme = activeThemeMetrics(metrics);
    const auto content =
        detail::inset(*absolute, theme.control_border_thickness);
    if (!content.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    Point text_origin{content->x, content->y};
    if (button.isPressed() &&
        theme.button_pressed_offset > 0 &&
        content->width > theme.button_pressed_offset &&
        content->height > theme.button_pressed_offset) {
        const auto pressed_x = detail::narrowCoordinate(
            static_cast<std::int64_t>(content->x) +
            static_cast<std::int64_t>(theme.button_pressed_offset));
        const auto pressed_y = detail::narrowCoordinate(
            static_cast<std::int64_t>(content->y) +
            static_cast<std::int64_t>(theme.button_pressed_offset));
        if (!pressed_x.has_value() || !pressed_y.has_value()) {
            return PresentationUpdateResult::deferred;
        }
        text_origin = {*pressed_x, *pressed_y};
    }

    TextStyle style = controlTextStyle(button, button.textStyle());

    /*
     * Hover is backend-neutral Widget state, but its appearance remains a Rendered presentation
     * choice. Underline is a deliberately small cue that changes no geometry and does not mutate the
     * Button's semantic/user-provided TextStyle. Pressed feedback remains the stronger interaction
     * state and therefore suppresses this transient hover decoration.
     */
    if (button.isEnabled() && button.isPointerOver() && !button.isPressed()) {
        style.underline = true;
    }

    eraseWidget(display_list, *absolute, background_color);
    if (theme.control_border_thickness > 0) {
        display_list.strokeRect(
            *absolute,
            style.foreground,
            theme.control_border_thickness);
    }

    if (!content->isEmpty()) {
        /*
         * Pressed offset is visual state only and intentionally does not participate in intrinsic
         * measurement. Theme geometry changes chrome without changing Button's semantic state.
         */
        display_list.drawText(text_origin, button.text(), style, *content);
    }

    return PresentationUpdateResult::synchronized;
}

[[nodiscard]] PresentationUpdateResult renderCheckBox(
    DisplayList& display_list,
    const CheckBox& check_box,
    const RenderedMeasurementContext* metrics,
    Color background_color) {
    const auto absolute = detail::absoluteRectOf(check_box);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    if (!check_box.isVisible() || absolute->isEmpty()) {
        eraseWidget(display_list, *absolute, background_color);
        return PresentationUpdateResult::synchronized;
    }

    /*
     * Unlike Button, a rendered CheckBox needs font metrics even when its caption is empty because
     * the indicator square is intentionally tied to line height. A metrics-free sink therefore
     * defers before mutating DisplayList instead of guessing a backend-specific indicator size.
     */
    if (metrics == nullptr) {
        return PresentationUpdateResult::deferred;
    }

    const RenderedThemeMetrics theme = metrics->themeMetrics().normalized();
    const Coordinate requested_indicator =
        std::max(Coordinate{1}, metrics->lineHeight());
    const Coordinate indicator_size =
        std::min(requested_indicator,
                 std::min(absolute->width, absolute->height));
    const Coordinate gap =
        std::max(Coordinate{1}, theme.control_border_thickness);

    /*
     * Preflight every coordinate derived from arranged geometry before erasing anything. This keeps
     * the sink transactional: an unrepresentable accumulated x coordinate leaves the previous frame
     * intact and the Widget pending.
     */
    const std::int64_t text_x_wide =
        static_cast<std::int64_t>(absolute->x) +
        static_cast<std::int64_t>(indicator_size) +
        static_cast<std::int64_t>(gap);
    const auto text_x = detail::narrowCoordinate(text_x_wide);
    if (!text_x.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    const std::int64_t right_wide =
        static_cast<std::int64_t>(absolute->x) +
        static_cast<std::int64_t>(absolute->width);
    const std::int64_t available_wide =
        std::max<std::int64_t>(0, right_wide - text_x_wide);
    if (available_wide > std::numeric_limits<Coordinate>::max()) {
        return PresentationUpdateResult::deferred;
    }

    const Rect indicator{
        absolute->x,
        absolute->y,
        indicator_size,
        indicator_size};
    const Rect caption_clip{
        *text_x,
        absolute->y,
        static_cast<Coordinate>(available_wide),
        absolute->height};

    TextStyle style =
        controlTextStyle(check_box, check_box.textStyle());

    /*
     * Hover and press are presentation overlays only. Hover underlines the caption; press uses
     * inverse text feedback. Neither changes geometry or mutates the user-provided TextStyle.
     */
    if (check_box.isEnabled() &&
        check_box.isPointerOver() &&
        !check_box.isPressed()) {
        style.underline = true;
    }
    if (check_box.isEnabled() && check_box.isPressed()) {
        style.inverse = true;
    }

    eraseWidget(display_list, *absolute, background_color);

    if (!indicator.isEmpty()) {
        if (theme.control_border_thickness > 0) {
            display_list.strokeRect(
                indicator,
                style.foreground,
                theme.control_border_thickness);
        }

        if (check_box.isChecked()) {
            /*
             * Checked state is represented as solid foreground ink inside the indicator border. When
             * the theme disables border geometry, filling the complete square still leaves an
             * unambiguous checked mark instead of making the control disappear.
             */
            const Coordinate inset_amount =
                theme.control_border_thickness > 0
                    ? theme.control_border_thickness
                    : Coordinate{0};
            const auto mark = detail::inset(indicator, inset_amount);
            if (!mark.has_value()) {
                return PresentationUpdateResult::deferred;
            }
            if (!mark->isEmpty()) {
                display_list.fillRect(
                    *mark,
                    style.foreground,
                    FillRole::foreground);
            }
        }
    }

    if (!caption_clip.isEmpty()) {
        display_list.drawText(
            {caption_clip.x, caption_clip.y},
            check_box.text(),
            style,
            caption_clip);
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

    const RenderedThemeMetrics theme = metrics->themeMetrics().normalized();
    const auto layout =
        detail::buildTextFieldViewport(field, *absolute, *metrics, theme);
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
    if (theme.control_border_thickness > 0) {
        display_list.strokeRect(
            layout->bounds,
            style.foreground,
            theme.control_border_thickness);
    }

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

    if (const auto* check_box = dynamic_cast<const CheckBox*>(&widget)) {
        return renderCheckBox(
            display_list_,
            *check_box,
            measurement_context_,
            background_color_);
    }

    if (const auto* button = dynamic_cast<const Button*>(&widget)) {
        return renderButton(
            display_list_,
            *button,
            measurement_context_,
            background_color_);
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
