#pragma once

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/tree_view.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui::rendered {

enum class RenderedTreeViewHitKind {
    row,
    indicator,
};

struct RenderedTreeViewHit final {
    TreeNodePath node;
    RenderedTreeViewHitKind kind{RenderedTreeViewHitKind::row};
};

struct RenderedTreeViewRow final {
    TreeNodePath node;
    std::size_t depth{0};
    std::string text;
    Rect bounds{};
    Rect indicator_bounds{};
    bool has_children{false};
    bool expanded{false};
    bool selected{false};
};

/** Owned backend-neutral graphical geometry for exactly the visible TreeView rows. */
struct RenderedTreeViewPresentationSnapshot final {
    Rect bounds{};
    std::vector<RenderedTreeViewRow> rows;
    bool focused{false};
    std::uint64_t model_revision{0};
};

class RenderedTreeViewPresentation final {
public:
    RenderedTreeViewPresentation() = delete;

    [[nodiscard]] static std::optional<RenderedTreeViewPresentationSnapshot>
    snapshot(const TreeView& view, Rect bounds);

    /** Composes a complete value transaction before replacing the caller's DisplayList. */
    [[nodiscard]] static bool render(DisplayList& display_list,
                                     const RenderedTreeViewPresentationSnapshot& snapshot,
                                     Color background_color = Color::default_color);

    [[nodiscard]] static std::optional<RenderedTreeViewHit>
    hitAt(const RenderedTreeViewPresentationSnapshot& snapshot, Point point) noexcept;

    [[nodiscard]] static bool selectAt(const RenderedTreeViewPresentationSnapshot& snapshot,
                                       Point point,
                                       TreeSelectionModel& selection) noexcept;

    [[nodiscard]] static bool toggleAt(const RenderedTreeViewPresentationSnapshot& snapshot,
                                       Point point,
                                       TreeView& view) noexcept;
};

} // namespace sasd::ui::rendered
