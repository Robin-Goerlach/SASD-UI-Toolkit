#include <sasd/ui/rendered/rendered_presentation_sink.hpp>

#include "rendered_geometry.hpp"
#include "rendered_text_field_viewport.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/stack_layout.hpp>
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
#include <string_view>
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

[[nodiscard]] TextStyle textFieldSelectionStyle(TextStyle base) noexcept {
    /*
     * Selection is applied after the normal TextField control overlays have been resolved. Toggling
     * rather than forcing inverse keeps the selected run distinguishable in both unfocused fields
     * and focused fields whose complete base presentation is already inverse.
     *
     * Keep every unrelated semantic/user style bit intact. In particular foreground colour, bold,
     * dim and underline remain properties of the field; selection changes contrast only.
     */
    base.inverse = !base.inverse;
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

[[nodiscard]] PresentationUpdateResult renderComboBox(
    DisplayList& display_list,
    const ComboBox& combo,
    const RenderedMeasurementContext* metrics,
    Color background_color) {
    const auto absolute = detail::absoluteRectOf(combo);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    if (!combo.isVisible() || absolute->isEmpty()) {
        eraseWidget(display_list, *absolute, background_color);
        return PresentationUpdateResult::synchronized;
    }

    /*
     * The collapsed ComboBox needs the same metric provider that measured it: line height determines
     * the stable indicator lane, while measureText("v") lets the presentation center the drop marker
     * without assuming a monospace desktop font. A metrics-free sink therefore stays fail-closed.
     */
    if (metrics == nullptr) {
        return PresentationUpdateResult::deferred;
    }

    const RenderedThemeMetrics theme = metrics->themeMetrics().normalized();
    const auto content = detail::inset(*absolute, theme.control_border_thickness);
    if (!content.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    const std::string_view indicator_marker =
        combo.isDropDownOpen() ? std::string_view{"^"} : std::string_view{"v"};
    const Size indicator_text = metrics->measureText(indicator_marker);
    const Size opposite_indicator_text =
        metrics->measureText(combo.isDropDownOpen() ? std::string_view{"v"} : std::string_view{"^"});
    const Coordinate requested_indicator_width =
        std::max(
            std::max(Coordinate{1}, metrics->lineHeight()),
            std::max(indicator_text.width, opposite_indicator_text.width));
    const Coordinate indicator_width =
        std::min(requested_indicator_width, content->width);
    const Coordinate gap =
        std::max(Coordinate{1}, theme.control_border_thickness);

    /*
     * Preflight every widened coordinate before appending commands. DisplayList has no rollback API;
     * if an extreme parent offset cannot be represented, the previous good frame must remain intact
     * and the ComboBox must stay pending.
     */
    const std::int64_t content_right_wide =
        static_cast<std::int64_t>(content->x) +
        static_cast<std::int64_t>(content->width);
    const std::int64_t indicator_x_wide =
        content_right_wide - static_cast<std::int64_t>(indicator_width);
    const auto indicator_x = detail::narrowCoordinate(indicator_x_wide);
    if (!indicator_x.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    const std::int64_t text_right_wide =
        std::max<std::int64_t>(
            static_cast<std::int64_t>(content->x),
            indicator_x_wide - static_cast<std::int64_t>(gap));
    const std::int64_t text_width_wide =
        text_right_wide - static_cast<std::int64_t>(content->x);
    if (text_width_wide < 0 ||
        text_width_wide > static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
        return PresentationUpdateResult::deferred;
    }

    const Rect text_clip{
        content->x,
        content->y,
        static_cast<Coordinate>(text_width_wide),
        content->height};
    const Rect indicator_clip{
        *indicator_x,
        content->y,
        indicator_width,
        content->height};

    Coordinate arrow_x_offset = 0;
    if (indicator_text.width > 0 && indicator_text.width < indicator_clip.width) {
        arrow_x_offset =
            static_cast<Coordinate>((indicator_clip.width - indicator_text.width) / 2);
    }

    Coordinate arrow_y_offset = 0;
    if (indicator_text.height > 0 && indicator_text.height < indicator_clip.height) {
        arrow_y_offset =
            static_cast<Coordinate>((indicator_clip.height - indicator_text.height) / 2);
    }

    const auto arrow_x = detail::narrowCoordinate(
        static_cast<std::int64_t>(indicator_clip.x) +
        static_cast<std::int64_t>(arrow_x_offset));
    const auto arrow_y = detail::narrowCoordinate(
        static_cast<std::int64_t>(indicator_clip.y) +
        static_cast<std::int64_t>(arrow_y_offset));
    if (!arrow_x.has_value() || !arrow_y.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    TextStyle style = controlTextStyle(combo, combo.textStyle());

    /*
     * Hover and press are visual overlays only. Hover underlines the collapsed content; press toggles
     * inverse relative to the already-resolved focus/user style so a focused ComboBox still gets a
     * visible pressed transition. Neither overlay changes measurement or the stable indicator lane.
     */
    if (combo.isEnabled() &&
        combo.isPointerOver() &&
        !combo.isPressed()) {
        style.underline = true;
    }
    if (combo.isEnabled() && combo.isPressed()) {
        style.inverse = !style.inverse;
    }

    /*
     * Everything that can fail is resolved above. Repaint the whole arranged rectangle first so a
     * short newly selected item cannot leave pixels from a previously longer selection. The outer
     * border and right-anchored indicator lane remain stable across selection changes.
     */
    eraseWidget(display_list, *absolute, background_color);
    if (theme.control_border_thickness > 0) {
        display_list.strokeRect(
            *absolute,
            style.foreground,
            theme.control_border_thickness);
    }

    const std::optional<std::string_view> selected = combo.selectedText();
    if (selected.has_value() && !selected->empty() && !text_clip.isEmpty()) {
        display_list.drawText(
            {text_clip.x, text_clip.y},
            *selected,
            style,
            text_clip);
    }

    if (!indicator_clip.isEmpty()) {
        display_list.drawText(
            {*arrow_x, *arrow_y},
            indicator_marker,
            style,
            indicator_clip);
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

[[nodiscard]] PresentationUpdateResult renderRadioButton(
    DisplayList& display_list,
    const RadioButton& radio,
    const RenderedMeasurementContext* metrics,
    Color background_color) {
    const auto absolute = detail::absoluteRectOf(radio);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    if (!radio.isVisible() || absolute->isEmpty()) {
        eraseWidget(display_list, *absolute, background_color);
        return PresentationUpdateResult::synchronized;
    }

    /*
     * The selector box is tied to the active font line height, so metrics are mandatory even for an
     * empty caption. Do not guess a size in the metrics-free sink: deferral preserves the last known
     * good frame and keeps measurement/presentation contracts aligned.
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

    /*
     * Pre-compute the selected mark before mutating DisplayList. The first DisplayList has only
     * rectangular primitives, so a compact centered square is used as the radio "dot". This is a
     * presentation compromise, not Core semantics; a later richer graphics primitive can make the
     * selector circular without changing RadioButton or RadioGroup.
     */
    std::optional<Rect> selected_mark;
    if (radio.isSelected() && !indicator.isEmpty()) {
        Coordinate mark_size = indicator_size / 3;
        if (mark_size <= 0) {
            mark_size = 1;
        }

        const Coordinate offset =
            static_cast<Coordinate>((indicator_size - mark_size) / 2);

        const auto mark_x = detail::narrowCoordinate(
            static_cast<std::int64_t>(indicator.x) +
            static_cast<std::int64_t>(offset));
        const auto mark_y = detail::narrowCoordinate(
            static_cast<std::int64_t>(indicator.y) +
            static_cast<std::int64_t>(offset));
        if (!mark_x.has_value() || !mark_y.has_value()) {
            return PresentationUpdateResult::deferred;
        }

        selected_mark = Rect{*mark_x, *mark_y, mark_size, mark_size};
    }

    TextStyle style = controlTextStyle(radio, radio.textStyle());

    if (radio.isEnabled() &&
        radio.isPointerOver() &&
        !radio.isPressed()) {
        style.underline = true;
    }
    if (radio.isEnabled() && radio.isPressed()) {
        style.inverse = true;
    }

    eraseWidget(display_list, *absolute, background_color);

    if (!indicator.isEmpty() && theme.control_border_thickness > 0) {
        display_list.strokeRect(
            indicator,
            style.foreground,
            theme.control_border_thickness);
    }

    if (selected_mark.has_value() && !selected_mark->isEmpty()) {
        display_list.fillRect(
            *selected_mark,
            style.foreground,
            FillRole::foreground);
    }

    if (!caption_clip.isEmpty()) {
        display_list.drawText(
            {caption_clip.x, caption_clip.y},
            radio.text(),
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
     * Selection uses the same complete shaped UTF-8 run as the ordinary field text. We therefore
     * need only two additional scalar-boundary advances, not independently shaped substrings. That
     * preserves ligature/kerning context and makes the overlay a pure clipping/styling decision.
     *
     * Resolve every potentially unsupported metric and every widened coordinate before appending
     * any command. PresentationSink has no rollback API, so a failed selection boundary must leave
     * the previous DisplayList completely untouched just like a failed caret preflight.
     */
    std::optional<Rect> selection_clip;
    if (field.hasSelection() && !layout->content.isEmpty()) {
        const auto selection_start_advance =
            metrics->textAdvanceToScalar(field.text(), field.selectionStart());
        const auto selection_end_advance =
            metrics->textAdvanceToScalar(field.text(), field.selectionEnd());

        if (!selection_start_advance.has_value() ||
            !selection_end_advance.has_value() ||
            *selection_start_advance < 0 ||
            *selection_end_advance < *selection_start_advance) {
            return PresentationUpdateResult::deferred;
        }

        const std::int64_t selection_left_wide =
            static_cast<std::int64_t>(layout->text_origin.x) +
            static_cast<std::int64_t>(*selection_start_advance);
        const std::int64_t selection_right_wide =
            static_cast<std::int64_t>(layout->text_origin.x) +
            static_cast<std::int64_t>(*selection_end_advance);

        const std::int64_t content_left = layout->content.x;
        const std::int64_t content_right =
            static_cast<std::int64_t>(layout->content.x) +
            static_cast<std::int64_t>(layout->content.width);

        const std::int64_t visible_left =
            std::max(selection_left_wide, content_left);
        const std::int64_t visible_right =
            std::min(selection_right_wide, content_right);

        if (visible_right > visible_left) {
            const auto selection_x = detail::narrowCoordinate(visible_left);
            const std::int64_t selection_width_wide =
                visible_right - visible_left;

            if (!selection_x.has_value() ||
                selection_width_wide >
                    static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
                return PresentationUpdateResult::deferred;
            }

            selection_clip = Rect{
                *selection_x,
                layout->content.y,
                static_cast<Coordinate>(selection_width_wide),
                layout->content.height};
        }
    }

    /*
     * Everything fallible was preflighted above. From this point onward the commands form one
     * coherent field repaint: clear old pixels, draw chrome, draw the clipped full-context base run,
     * optionally replay that exact same run through the selection clip, then place the caret last so
     * it remains visible over both text passes.
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

        if (selection_clip.has_value()) {
            display_list.drawText(
                layout->text_origin,
                field.text(),
                textFieldSelectionStyle(style),
                *selection_clip);
        }
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

    if (const auto* combo = dynamic_cast<const ComboBox*>(&widget)) {
        return renderComboBox(
            display_list_,
            *combo,
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

    if (const auto* radio = dynamic_cast<const RadioButton*>(&widget)) {
        return renderRadioButton(
            display_list_,
            *radio,
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
     * Layout containers do not contribute their own rendered commands; they only establish child
     * geometry. Keep the whitelist explicit rather than accepting every Container subclass. A
     * future composite control may inherit Container while still requiring its own chrome, and
     * silently acknowledging it here would lose a pending visual update before the rendered backend
     * has learned how to represent that control.
     */
    if (typeid(widget) == typeid(Widget) ||
        typeid(widget) == typeid(Container) ||
        dynamic_cast<const VBox*>(&widget) != nullptr ||
        dynamic_cast<const HBox*>(&widget) != nullptr ||
        dynamic_cast<const GridLayout*>(&widget) != nullptr ||
        dynamic_cast<const FormLayout*>(&widget) != nullptr ||
        dynamic_cast<const StackLayout*>(&widget) != nullptr) {
        return PresentationUpdateResult::synchronized;
    }

    return PresentationUpdateResult::deferred;
}

} // namespace sasd::ui::rendered
