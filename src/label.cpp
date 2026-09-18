#include <sasd/ui/label.hpp>

#include <sasd/ui/measurement_context.hpp>

#include <utility>

namespace sasd::ui {

void Label::setText(std::string text) {
    if (text_ == text) {
        return;
    }

    text_ = std::move(text);

    /*
     * Different text may need different space in any presentation environment. Invalidate even when
     * no MeasurementContext is currently attached; the next context-aware pass will recompute it.
     */
    invalidateMeasure();
    invalidateVisual();
}

Size Label::onMeasure(const MeasurementContext& context, const MeasureConstraints&) {
    return context.measureText(text_);
}

} // namespace sasd::ui
