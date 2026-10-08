#pragma once

#include <sasd/ui/list_view.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Owned terminal transaction for one ListView's visible rows.
 *
 * The snapshot contains semantic row identities alongside copied text. Rendering and pointer hit
 * testing consume this same value, so a model mutation between frames cannot make a pointer coordinate
 * select a newly reinterpreted numeric row. Rows outside the viewport never enter this object.
 */
struct TerminalListViewPresentationSnapshot final {
    Rect bounds{};
    std::vector<ListViewRow> rows;
    bool focused{false};
    std::uint64_t model_revision{0};
};

class TerminalListViewPresentation final {
public:
    TerminalListViewPresentation() = delete;

    /** Builds an owned snapshot and rejects rows that cannot fit one terminal cell row each. */
    [[nodiscard]] static std::optional<TerminalListViewPresentationSnapshot>
    snapshot(const ListView& view, Rect bounds);

    /** Renders only the rows already present in the snapshot; it never queries the model. */
    [[nodiscard]] static bool render(ScreenBuffer& buffer,
                                     const TerminalListViewPresentationSnapshot& snapshot,
                                     AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);

    /** Returns the semantic row identity from the exact final snapshot geometry. */
    [[nodiscard]] static std::optional<std::size_t>
    rowAt(const TerminalListViewPresentationSnapshot& snapshot, Point point) noexcept;

    /** Applies a primary pointer selection using snapshot identity, with no second geometry pass. */
    [[nodiscard]] static bool selectAt(const TerminalListViewPresentationSnapshot& snapshot,
                                       Point point,
                                       ListSelectionModel& selection) noexcept;
};

} // namespace sasd::ui::terminal
