#pragma once

#include <sasd/ui/container.hpp>

namespace sasd::ui {

/**
 * Simple horizontal layout container for the first M2 automatic-layout slice.
 *
 * Visible children are measured in visual adoption order. Desired height is the tallest child;
 * desired width is the sum of visible child widths plus spacing. During arrangement children stretch
 * across the full available height while retaining desired widths until horizontal space is exhausted.
 */
class HBox final : public Container {
public:
    HBox() = default;

    [[nodiscard]] Coordinate spacing() const noexcept { return spacing_; }

    /**
     * Sets non-negative logical spacing between adjacent visible children.
     *
     * @throws std::invalid_argument when spacing is negative.
     */
    void setSpacing(Coordinate spacing);

protected:
    [[nodiscard]] Size onMeasure(const MeasureConstraints& constraints) override;
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;
    void onArrange(Rect final_bounds) override;

private:
    Coordinate spacing_{0};
};

} // namespace sasd::ui
