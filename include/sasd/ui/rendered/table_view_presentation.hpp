#pragma once

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/table_view.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui::rendered {

struct RenderedTableViewPresentationSnapshot final {
    Rect bounds{};
    Rect header_bounds{};
    std::vector<Rect> column_bounds;
    std::vector<Rect> row_bounds;
    std::vector<std::string> headers;
    std::vector<TableViewRow> rows;
    std::uint64_t model_revision{0};
};

struct RenderedTableViewHit final {
    std::size_t row{0};
    std::size_t column{0};

    friend constexpr bool operator==(const RenderedTableViewHit&,
                                     const RenderedTableViewHit&) = default;
};

class RenderedTableViewPresentation final {
public:
    RenderedTableViewPresentation() = delete;

    [[nodiscard]] static std::optional<RenderedTableViewPresentationSnapshot>
    snapshot(const TableView& view,
             Rect bounds,
             const RenderedMeasurementContext& measurement_context);

    /** Renders the exact owned snapshot; it never queries TableModel or recomputes geometry. */
    [[nodiscard]] static bool render(DisplayList& display_list,
                                     const RenderedTableViewPresentationSnapshot& snapshot);

    [[nodiscard]] static std::optional<RenderedTableViewHit>
    hitAt(const RenderedTableViewPresentationSnapshot& snapshot, Point point) noexcept;
};

} // namespace sasd::ui::rendered
