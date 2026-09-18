#pragma once

#include <sasd/ui/container.hpp>

namespace sasd::ui {

/**
 * Simple vertical layout container for the first M2 automatic-layout slice.
 *
 * Visible children are measured in visual adoption order. Desired width is the widest child; desired
 * height is the sum of visible child heights plus spacing. During arrangement children stretch across
 * the full available width while retaining their desired heights until vertical space is exhausted.
 *
 * This deliberately omits margins, padding, alignment and flex weights. Those policies should be
 * added only after real controls demonstrate their requirements.
 */
class VBox final : public Container {
public:
    VBox() = default;

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
