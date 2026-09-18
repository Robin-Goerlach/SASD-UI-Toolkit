#include <sasd/ui/label.hpp>

#include <utility>

namespace sasd::ui {

void Label::setText(std::string text) {
    if (text_ == text) {
        return;
    }

    text_ = std::move(text);

    /*
     * A different text string may need different space once a backend/text-measurement service is
     * attached. Invalidation is therefore correct even though the current base onMeasure() cannot
     * derive text metrics yet.
     */
    invalidateMeasure();
    invalidateVisual();
}

} // namespace sasd::ui
