#include <sasd/ui/rendered/display_list.hpp>

#include <stdexcept>
#include <string>

namespace sasd::ui::rendered {

void DisplayList::validateRect(Rect bounds) {
    if (bounds.width < 0 || bounds.height < 0) {
        throw std::invalid_argument("rendered rectangle extents must be non-negative");
    }
}

void DisplayList::fillRect(Rect bounds, Color color) {
    validateRect(bounds);

    if (bounds.isEmpty()) {
        return;
    }

    commands_.emplace_back(FillRectCommand{bounds, color});
}

void DisplayList::strokeRect(Rect bounds, Color color, Coordinate thickness) {
    validateRect(bounds);

    if (thickness <= 0) {
        throw std::invalid_argument("rendered rectangle stroke thickness must be positive");
    }

    if (bounds.isEmpty()) {
        return;
    }

    commands_.emplace_back(StrokeRectCommand{bounds, color, thickness});
}

void DisplayList::drawText(Point origin,
                           std::string_view utf8_text,
                           TextStyle style,
                           std::optional<Rect> clip_bounds) {
    if (utf8_text.empty()) {
        return;
    }

    if (clip_bounds.has_value()) {
        validateRect(*clip_bounds);
        if (clip_bounds->isEmpty()) {
            return;
        }
    }

    /*
     * Copy the bytes now instead of storing string_view. The display list may outlive the widget
     * callback that produced it, and renderer replay must never depend on dangling widget storage.
     * The clip rectangle is copied for the same snapshot semantics.
     */
    commands_.emplace_back(
        DrawTextCommand{origin, std::string{utf8_text}, style, clip_bounds});
}

} // namespace sasd::ui::rendered
