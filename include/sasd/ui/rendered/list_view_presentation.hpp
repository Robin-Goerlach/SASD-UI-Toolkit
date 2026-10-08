#pragma once

#include <sasd/ui/list_view.hpp>
#include <sasd/ui/rendered/display_list.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui::rendered {

/** Owned, backend-neutral rendered transaction for visible ListView rows. */
struct RenderedListViewPresentationSnapshot final {
    Rect bounds{};
    std::vector<ListViewRow> rows;
    bool focused{false};
    std::uint64_t model_revision{0};
};

class RenderedListViewPresentation final {
public:
    RenderedListViewPresentation() = delete;

    [[nodiscard]] static std::optional<RenderedListViewPresentationSnapshot>
    snapshot(const ListView& view, Rect bounds);

    /** Appends exactly the snapshot's rows and geometry; it does not query ListModel. */
    [[nodiscard]] static bool render(DisplayList& display_list,
                                     const RenderedListViewPresentationSnapshot& snapshot,
                                     Color background_color = Color::default_color);

    [[nodiscard]] static std::optional<std::size_t>
    rowAt(const RenderedListViewPresentationSnapshot& snapshot, Point point) noexcept;

    [[nodiscard]] static bool selectAt(const RenderedListViewPresentationSnapshot& snapshot,
                                       Point point,
                                       ListSelectionModel& selection) noexcept;
};

} // namespace sasd::ui::rendered
