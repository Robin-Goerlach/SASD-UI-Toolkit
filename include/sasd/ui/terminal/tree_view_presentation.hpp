#pragma once

#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>
#include <sasd/ui/tree_view.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

enum class TerminalTreeViewHitKind {
    row,
    indicator,
};

struct TerminalTreeViewHit final {
    TreeNodePath node;
    TerminalTreeViewHitKind kind{TerminalTreeViewHitKind::row};
};

struct TerminalTreeViewRow final {
    TreeNodePath node;
    std::size_t depth{0};
    std::string text;
    Rect bounds{};
    Rect indicator_bounds{};
    bool has_children{false};
    bool expanded{false};
    bool selected{false};
};

/** Owned terminal geometry for exactly the visible TreeView rows. */
struct TerminalTreeViewPresentationSnapshot final {
    Rect bounds{};
    std::vector<TerminalTreeViewRow> rows;
    bool focused{false};
    std::uint64_t model_revision{0};
};

class TerminalTreeViewPresentation final {
public:
    TerminalTreeViewPresentation() = delete;

    [[nodiscard]] static std::optional<TerminalTreeViewPresentationSnapshot>
    snapshot(const TreeView& view, Rect bounds);

    /** Preflights into a copied buffer so malformed later rows cannot partially paint earlier ones. */
    [[nodiscard]] static bool render(ScreenBuffer& buffer,
                                     const TerminalTreeViewPresentationSnapshot& snapshot,
                                     AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);

    [[nodiscard]] static std::optional<TerminalTreeViewHit>
    hitAt(const TerminalTreeViewPresentationSnapshot& snapshot, Point point) noexcept;

    [[nodiscard]] static bool selectAt(const TerminalTreeViewPresentationSnapshot& snapshot,
                                       Point point,
                                       TreeSelectionModel& selection) noexcept;

    [[nodiscard]] static bool toggleAt(const TerminalTreeViewPresentationSnapshot& snapshot,
                                       Point point,
                                       TreeView& view) noexcept;
};

} // namespace sasd::ui::terminal
