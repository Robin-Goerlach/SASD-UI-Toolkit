#include <sasd/ui/rendered/rendered_presentation_sink.hpp>

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/widget.hpp>
#include <sasd/ui/window.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <typeinfo>

namespace sasd::ui::rendered {
namespace {

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

    constexpr auto minimum = static_cast<std::int64_t>(
        std::numeric_limits<Coordinate>::min());
    constexpr auto maximum = static_cast<std::int64_t>(
        std::numeric_limits<Coordinate>::max());

    if (x < minimum || x > maximum || y < minimum || y > maximum) {
        return std::nullopt;
    }

    return Rect{
        static_cast<Coordinate>(x),
        static_cast<Coordinate>(y),
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

    const std::int64_t x = static_cast<std::int64_t>(bounds.x) + 1;
    const std::int64_t y = static_cast<std::int64_t>(bounds.y) + 1;
    const std::int64_t maximum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());

    if (x > maximum || y > maximum) {
        return std::nullopt;
    }

    return Rect{
        static_cast<Coordinate>(x),
        static_cast<Coordinate>(y),
        static_cast<Coordinate>(bounds.width - 2),
        static_cast<Coordinate>(bounds.height - 2),
    };
}

void eraseWidget(DisplayList& display_list, Rect bounds, Color background_color) {
    /*
     * The first rendered widgets are intentionally opaque inside their arranged bounds. Repainting
     * the background before current content prevents shorter Label/Button text from leaving stale
     * glyphs in an incremental frame. Rich background inheritance/compositing is a later theme step.
     */
    display_list.fillRect(bounds, background_color);
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

    /*
     * Keep focus/disabled appearance a backend presentation overlay. The semantic Button's TextStyle
     * remains untouched, matching the terminal backend's rule that focus does not mutate user state.
     */
    TextStyle style = button.textStyle();
    if (!button.isEnabled()) {
        style.dim = true;
    } else if (button.hasFocus()) {
        style.inverse = true;
    }

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
        const auto absolute = absoluteRectOf(*field);
        if (!absolute.has_value()) {
            return PresentationUpdateResult::deferred;
        }

        if (!field->isVisible() || absolute->isEmpty()) {
            eraseWidget(display_list_, *absolute, background_color_);
            return PresentationUpdateResult::synchronized;
        }

        /*
         * Visible TextField rendering is deliberately not approximated yet. Correct desktop
         * presentation needs measured glyph advances for horizontal viewport/caret placement.
         * Deferring keeps the Widget pending until the rendered text-metrics slice supplies that
         * information, rather than acknowledging an incomplete focus/caret representation.
         */
        return PresentationUpdateResult::deferred;
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
