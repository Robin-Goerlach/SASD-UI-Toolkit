#include <sasd/ui/rendered/rendered_presentation_sink.hpp>

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

/**
 * Narrows one widened coordinate only when it is representable by the public Coordinate type.
 *
 * Rendered presentation routinely combines parent offsets with measured text advances. Performing
 * that arithmetic directly in int32_t would make deeply nested or deliberately extreme test input
 * vulnerable to signed overflow. Deferring an unrepresentable visual state is safer than wrapping it
 * to an unrelated location.
 */
[[nodiscard]] std::optional<Coordinate> narrowCoordinate(std::int64_t value) noexcept {
    constexpr auto minimum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min());
    constexpr auto maximum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());

    if (value < minimum || value > maximum) {
        return std::nullopt;
    }

    return static_cast<Coordinate>(value);
}

/**
 * Resolves parent-relative Widget bounds into the rendered root coordinate system.
 *
 * Parent offsets are accumulated in 64 bits. A deep but valid int32 widget hierarchy can otherwise
 * overflow Coordinate while it is being summed. DisplayList intentionally remains based on the
 * toolkit's public Coordinate type, so an absolute position that cannot be represented there is
 * deferred instead of silently wrapping to an unrelated screen location.
 */
[[nodiscard]] std::optional<Rect> absoluteRectOf(const Widget& widget) noexcept {
    std::int64_t x = static_cast<std::int64_t>(widget.bounds().x);
    std::int64_t y = static_cast<std::int64_t>(widget.bounds().y);

    for (const Container* parent = widget.parent(); parent != nullptr; parent = parent->parent()) {
        x += static_cast<std::int64_t>(parent->bounds().x);
        y += static_cast<std::int64_t>(parent->bounds().y);
    }

    const auto narrowed_x = narrowCoordinate(x);
    const auto narrowed_y = narrowCoordinate(y);
    if (!narrowed_x.has_value() || !narrowed_y.has_value()) {
        return std::nullopt;
    }

    return Rect{
        *narrowed_x,
        *narrowed_y,
        widget.bounds().width,
        widget.bounds().height,
    };
}

/**
 * Computes a one-unit content rectangle without performing narrow-coordinate arithmetic first.
 *
 * Tiny controls simply have no drawable interior. If adding the inset itself would move the origin
 * outside Coordinate, returning nullopt causes conservative presentation deferral.
 */
[[nodiscard]] std::optional<Rect> insetOne(Rect bounds) noexcept {
    if (bounds.width <= 2 || bounds.height <= 2) {
        return Rect{bounds.x, bounds.y, 0, 0};
    }

    const auto x = narrowCoordinate(static_cast<std::int64_t>(bounds.x) + 1);
    const auto y = narrowCoordinate(static_cast<std::int64_t>(bounds.y) + 1);
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }

    return Rect{
        *x,
        *y,
        static_cast<Coordinate>(bounds.width - 2),
        static_cast<Coordinate>(bounds.height - 2),
    };
}

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
    const auto absolute = absoluteRectOf(label);
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
    const auto absolute = absoluteRectOf(button);
    if (!absolute.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    eraseWidget(display_list, *absolute, background_color);

    if (!button.isVisible() || absolute->isEmpty()) {
        return PresentationUpdateResult::synchronized;
    }

    const TextStyle style = controlTextStyle(button, button.textStyle());
    display_list.strokeRect(*absolute, style.foreground, 1);

    const auto content = insetOne(*absolute);
    if (!content.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    if (!content->isEmpty()) {
        display_list.drawText(
            {content->x, content->y},
            button.text(),
            style,
            *content);
    }

    return PresentationUpdateResult::synchronized;
}

struct TextFieldLayout {
    Rect bounds{};
    Rect content{};
    Point text_origin{};
    TextStyle style{};
    std::optional<Rect> caret;
};

/**
 * Computes the complete TextField frame before mutating DisplayList.
 *
 * This preflight is important. A metric provider may report an invalid/non-monotonic caret advance
 * or coordinate arithmetic may become unrepresentable. In either case the field must remain
 * deferred *without first erasing its previous good pixels*. DisplayList currently has no rollback
 * transaction, so all fallible calculations happen before the first command is appended.
 */
[[nodiscard]] std::optional<TextFieldLayout> layoutTextField(
    const TextField& field,
    Rect bounds,
    const RenderedMeasurementContext& metrics) {
    const auto content = insetOne(bounds);
    if (!content.has_value()) {
        return std::nullopt;
    }

    TextFieldLayout result{
        bounds,
        *content,
        Point{content->x, content->y},
        controlTextStyle(field, field.textStyle()),
        std::nullopt,
    };

    /*
     * A very small arranged field can still represent its border/focus state. There is simply no
     * interior in which text or a caret can be placed, so no font query is necessary.
     */
    if (content->isEmpty()) {
        return result;
    }

    const Coordinate line_height = metrics.lineHeight();
    if (line_height <= 0) {
        // RenderedMeasurementContext documents a positive line height; reject broken providers.
        return std::nullopt;
    }

    const std::size_t scalar_count = utf8::scalarCount(field.text());
    const std::size_t cursor = std::min(field.cursorPosition(), scalar_count);

    /*
     * Ask the font/shaping provider for scalar-boundary positions in the *complete* text run. We do
     * not measure independent substrings: doing so can lose kerning/ligature context and make the
     * caret disagree with the text renderer. The first implementation is intentionally simple and
     * may query O(cursor) boundaries; a later optimization can cache a shaped run without changing
     * this contract.
     */
    std::vector<Coordinate> advances;
    advances.reserve(cursor + 1);

    for (std::size_t index = 0; index <= cursor; ++index) {
        const std::optional<Coordinate> advance =
            metrics.textAdvanceToScalar(field.text(), index);

        /*
         * nullopt is a normal capability result, not an error: the metric provider may know how to
         * draw/measure a text run while the initial M3 LTR caret model cannot express one of its
         * visual boundaries. Keep the field pending rather than guessing.
         */
        if (!advance.has_value() ||
            *advance < 0 ||
            (index == 0 && *advance != 0) ||
            (!advances.empty() && *advance < advances.back())) {
            return std::nullopt;
        }
        advances.push_back(*advance);
    }

    const Coordinate cursor_advance = advances.back();

    /*
     * Reserve one logical interior unit for the insertion caret even while unfocused. This keeps the
     * viewport stable across focus transitions and matches RenderedMeasurementContext::measureTextField().
     */
    const Coordinate text_capacity =
        content->width > 0 ? static_cast<Coordinate>(content->width - 1) : 0;

    std::size_t start_index = 0;
    while (start_index < cursor) {
        const std::int64_t visible_advance =
            static_cast<std::int64_t>(cursor_advance) -
            static_cast<std::int64_t>(advances[start_index]);

        if (visible_advance <= static_cast<std::int64_t>(text_capacity)) {
            break;
        }
        ++start_index;
    }

    const Coordinate start_advance = advances[start_index];
    const std::int64_t relative_caret =
        static_cast<std::int64_t>(cursor_advance) -
        static_cast<std::int64_t>(start_advance);

    /*
     * Draw the complete shaped string and shift its origin left by the viewport start advance. The
     * per-command clip then exposes only the field interior. Keeping the complete string intact is
     * deliberate: a future renderer can shape it once with full context instead of reshaping a
     * suffix whose kerning/ligatures may differ.
     */
    const auto text_x = narrowCoordinate(
        static_cast<std::int64_t>(content->x) -
        static_cast<std::int64_t>(start_advance));
    if (!text_x.has_value()) {
        return std::nullopt;
    }
    result.text_origin.x = *text_x;

    if (field.hasFocus()) {
        const auto caret_x = narrowCoordinate(
            static_cast<std::int64_t>(content->x) + relative_caret);
        if (!caret_x.has_value() ||
            relative_caret < 0 ||
            relative_caret >= static_cast<std::int64_t>(content->width)) {
            return std::nullopt;
        }

        const Coordinate caret_height = std::min(line_height, content->height);
        if (caret_height > 0) {
            result.caret = Rect{
                *caret_x,
                content->y,
                1,
                caret_height,
            };
        }
    }

    return result;
}

[[nodiscard]] PresentationUpdateResult renderTextField(
    DisplayList& display_list,
    const TextField& field,
    const RenderedMeasurementContext* metrics,
    Color background_color) {
    const auto absolute = absoluteRectOf(field);
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

    const auto layout = layoutTextField(field, *absolute, *metrics);
    if (!layout.has_value()) {
        return PresentationUpdateResult::deferred;
    }

    /*
     * Everything fallible was preflighted above. From this point onward the commands form one
     * coherent field repaint: clear old pixels, draw chrome, draw clipped full-context text, then
     * place the caret last so it remains visible over the glyph run.
     */
    eraseWidget(display_list, layout->bounds, background_color);
    display_list.strokeRect(layout->bounds, layout->style.foreground, 1);

    if (!layout->content.isEmpty()) {
        display_list.drawText(
            layout->text_origin,
            field.text(),
            layout->style,
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
            layout->style.foreground,
            FillRole::foreground);
    }

    return PresentationUpdateResult::synchronized;
}

} // namespace

PresentationUpdateResult RenderedPresentationSink::synchronize(const Widget& widget) {
    if (const auto* window = dynamic_cast<const Window*>(&widget)) {
        const auto absolute = absoluteRectOf(*window);
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
