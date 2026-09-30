#pragma once

#include <sasd/ui/container.hpp>

#include <cstddef>

namespace sasd::ui {

/**
 * Deterministic row-major grid container for the first M4 multi-axis layout slice.
 *
 * Visible children are assigned to cells in visual adoption order. columnCount() fixes the number of
 * columns; rows are created as needed. Each column's intrinsic width is the maximum desired width of
 * its visible occupants, and each row's intrinsic height is the maximum desired height of its visible
 * occupants. A child stretches to its complete arranged cell.
 *
 * GridLayout intentionally starts without spans, per-track weights, alignment, padding or implicit
 * named areas. Those policies are useful, but adding them before FormLayout and real application
 * screens exercise the basic grid would prematurely freeze a much larger public contract.
 */
class GridLayout final : public Container {
public:
    GridLayout() = default;
    explicit GridLayout(std::size_t columns);

    /** Returns the fixed number of columns. Always at least one. */
    [[nodiscard]] std::size_t columnCount() const noexcept { return columns_; }

    /**
     * Sets the fixed number of columns.
     *
     * Changing the count changes both intrinsic measurement and child cell assignment.
     *
     * @throws std::invalid_argument when columns is zero.
     */
    void setColumnCount(std::size_t columns);

    [[nodiscard]] Coordinate columnSpacing() const noexcept {
        return column_spacing_;
    }

    [[nodiscard]] Coordinate rowSpacing() const noexcept {
        return row_spacing_;
    }

    /**
     * Sets non-negative logical spacing between adjacent columns.
     *
     * @throws std::invalid_argument when spacing is negative.
     */
    void setColumnSpacing(Coordinate spacing);

    /**
     * Sets non-negative logical spacing between adjacent rows.
     *
     * @throws std::invalid_argument when spacing is negative.
     */
    void setRowSpacing(Coordinate spacing);

protected:
    [[nodiscard]] Size onMeasure(const MeasureConstraints& constraints) override;
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;
    void onArrange(Rect final_bounds) override;

private:
    std::size_t columns_{1};
    Coordinate column_spacing_{0};
    Coordinate row_spacing_{0};
};

} // namespace sasd::ui
