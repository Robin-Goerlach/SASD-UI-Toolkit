#pragma once

#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Owned terminal presentation for the visible TableView rectangle.
 *
 * All row/column/cell rectangles and semantic column identities are final values. Rendering and
 * hit testing consume this same snapshot; neither operation queries the TableModel or reconstructs
 * placement. This keeps stale frames fail-closed and keeps terminal-cell geometry out of Core.
 */
struct TerminalTableViewPresentationSnapshot final {
    Rect bounds{};
    Rect header_bounds{};
    std::vector<Rect> column_bounds;
    std::vector<std::size_t> column_indices;
    std::vector<Rect> row_bounds;
    std::vector<std::vector<Rect>> cell_bounds;
    std::vector<std::string> headers;
    std::vector<TableViewRow> rows;
    bool focused{false};
    AmbiguousWidthMode ambiguous_width{AmbiguousWidthMode::narrow};
    std::uint64_t model_revision{0};
};

struct TerminalTableViewHit final {
    std::size_t row{0};
    std::size_t column{0};

    friend constexpr bool operator==(const TerminalTableViewHit&, const TerminalTableViewHit&) = default;
};

class TerminalTableViewPresentation final {
public:
    TerminalTableViewPresentation() = delete;

    [[nodiscard]] static std::optional<TerminalTableViewPresentationSnapshot>
    snapshot(const TableView& view,
             Rect bounds,
             AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);

    /** Renders only owned final snapshot data and replaces the buffer transactionally. */
    [[nodiscard]] static bool render(ScreenBuffer& buffer,
                                     const TerminalTableViewPresentationSnapshot& snapshot,
                                     AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);

    [[nodiscard]] static std::optional<TerminalTableViewHit>
    hitAt(const TerminalTableViewPresentationSnapshot& snapshot, Point point) noexcept;

    /** Applies a snapshot hit only while the observed TableModel revision is unchanged. */
    [[nodiscard]] static bool selectAt(const TerminalTableViewPresentationSnapshot& snapshot,
                                       Point point,
                                       TableSelectionModel& selection);
};

} // namespace sasd::ui::terminal
