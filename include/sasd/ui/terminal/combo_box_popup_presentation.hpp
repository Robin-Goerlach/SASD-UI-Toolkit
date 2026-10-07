#pragma once

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/presentation/anchored_popup_layout.hpp>
#include <sasd/ui/terminal/presentation_frame.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Owned terminal-presentation snapshot for one open ComboBox popup.
 *
 * The snapshot deliberately contains no ComboBox pointer and no borrowed item string_view. A frame may
 * therefore be inspected or rendered after presentation construction without retaining semantic object
 * lifetime. This mirrors the menu presentation boundary: semantic state is sampled once, then terminal
 * geometry and painting operate on an owned value.
 *
 * bounds is the final viewport-fitted rectangle in terminal-cell coordinates. One item occupies exactly
 * one row. An empty item collection is represented by an empty bounds rectangle and paints no popup rows;
 * the collapsed ComboBox can still expose its '^' open marker without inventing an "(empty)" semantic item.
 *
 * side records whether the generic anchored-placement policy chose below or above. It is retained even
 * though the first row renderer does not yet draw direction-sensitive border chrome; future interaction
 * and presentation code should consume this explicit choice instead of re-inferring it from coordinates.
 */
struct ComboBoxPopupPresentationSnapshot {
    Rect bounds{};
    presentation::PopupVerticalSide side{presentation::PopupVerticalSide::below};
    std::vector<std::string> items{};
    std::optional<std::size_t> preview_index{};
    TextStyle text_style{};
};

/** Terminal-cell size required by the popup's item rows before anchor-width expansion. */
struct ComboBoxPopupPresentationSize {
    Size size{};
};

namespace combo_box_popup_detail {

[[nodiscard]] inline std::optional<Coordinate>
measurePopupLine(std::string_view text,
                 AmbiguousWidthMode ambiguous_width) noexcept {
    const TextMeasurement measured = TextMetrics::measureUtf8(text, ambiguous_width);

    /*
     * A ComboBox popup item is one semantic row. Newlines would make an item span more than one hit-test
     * row, while zero-width/control values cannot yet be represented faithfully by ScreenBuffer's simple
     * cell model. Reject those cases before any geometry or buffer mutation.
     */
    if (measured.rows != 1 ||
        !measured.simpleCellRenderable() ||
        measured.saturated) {
        return std::nullopt;
    }

    return measured.columns;
}

[[nodiscard]] inline bool checkedAdd(Coordinate value,
                                     Coordinate addition,
                                     Coordinate& result) noexcept {
    const std::int64_t sum =
        static_cast<std::int64_t>(value) + static_cast<std::int64_t>(addition);
    if (sum > static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
        return false;
    }

    result = static_cast<Coordinate>(sum);
    return true;
}

[[nodiscard]] inline bool rectangleInsideBuffer(const ScreenBuffer& buffer,
                                                Rect bounds) noexcept {
    if (bounds.isEmpty()) {
        return false;
    }

    const std::int64_t left = bounds.x;
    const std::int64_t top = bounds.y;
    const std::int64_t right =
        left + static_cast<std::int64_t>(bounds.width);
    const std::int64_t bottom =
        top + static_cast<std::int64_t>(bounds.height);

    return left >= 0 &&
           top >= 0 &&
           right <= static_cast<std::int64_t>(buffer.size().width) &&
           bottom <= static_cast<std::int64_t>(buffer.size().height);
}

inline void writePopupLine(ScreenBuffer& buffer,
                           Point origin,
                           std::string_view text,
                           AmbiguousWidthMode ambiguous_width,
                           TextStyle style) {
    std::int64_t column = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedCodePoint decoded = TextMetrics::decodeOne(text, offset);
        if (decoded.consumed == 0U) {
            break;
        }
        offset += decoded.consumed;

        const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
        if (width <= 0) {
            // Complete preflight guarantees this cannot occur for an accepted snapshot.
            continue;
        }

        const std::int64_t x =
            static_cast<std::int64_t>(origin.x) + column;
        const Coordinate y = origin.y;

        if (width == 1) {
            buffer.set(
                {static_cast<Coordinate>(x), y},
                Cell{decoded.value, CellRole::normal, style});
        } else if (width == 2) {
            /*
             * Builder/renderer preflight guarantees the complete popup rectangle is inside the buffer
             * and measurement guarantees this scalar fits the chosen row width. Write both occupancy
             * cells as one unit so a later ANSI diff encoder never sees a half-wide glyph.
             */
            buffer.set(
                {static_cast<Coordinate>(x), y},
                Cell{decoded.value, CellRole::wide_lead, style});
            buffer.set(
                {static_cast<Coordinate>(x + 1), y},
                Cell{U' ', CellRole::wide_continuation, style});
        }

        column += width;
    }
}

} // namespace combo_box_popup_detail

/**
 * Measures one terminal ComboBox popup from an owned snapshot.
 *
 * Each item receives one leading and one trailing padding cell. There is deliberately no border or
 * scrollbar in this first popup presentation slice. The width therefore reflects only row content;
 * buildComboBoxPopupPresentation() later expands it to at least the collapsed control's anchor width.
 *
 * The measurement contract is independent from open/closed state and placement, making synthetic
 * snapshots useful for deterministic tests. An empty item set has height zero and a two-cell natural
 * width, but the builder treats it as a no-row overlay rather than asking generic placement to position
 * a zero-area rectangle.
 */
[[nodiscard]] inline std::optional<ComboBoxPopupPresentationSize>
measureComboBoxPopupPresentation(
    const ComboBoxPopupPresentationSnapshot& snapshot,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept {
    Coordinate widest = 2; // leading + trailing padding even for an empty item string

    for (const std::string& item : snapshot.items) {
        const auto item_width =
            combo_box_popup_detail::measurePopupLine(item, ambiguous_width);
        if (!item_width.has_value()) {
            return std::nullopt;
        }

        Coordinate padded = 0;
        if (!combo_box_popup_detail::checkedAdd(*item_width, 2, padded)) {
            return std::nullopt;
        }
        widest = std::max(widest, padded);
    }

    if (snapshot.items.size() >
        static_cast<std::size_t>(std::numeric_limits<Coordinate>::max())) {
        return std::nullopt;
    }

    if (snapshot.preview_index.has_value() &&
        *snapshot.preview_index >= snapshot.items.size()) {
        return std::nullopt;
    }

    return ComboBoxPopupPresentationSize{
        Size{widest, static_cast<Coordinate>(snapshot.items.size())}};
}

/**
 * Snapshots one currently open semantic ComboBox into final terminal popup geometry.
 *
 * absolute_anchor must describe the collapsed ComboBox in the same coordinate space as viewport. The
 * helper intentionally does not walk Widget parents: resolving relative Widget bounds is a host/
 * presentation-tree concern, while this function owns only terminal popup policy.
 *
 * The semantic open-state invariant is revalidated instead of trusted blindly. An open popup must still
 * belong to a visible, enabled, focused ComboBox. This makes presentation fail closed if application
 * callbacks mutate state between semantic input and frame construction.
 *
 * All item text is copied into the returned snapshot. Unsupported terminal text, stale preview identity,
 * malformed geometry or inability to place the complete popup returns std::nullopt.
 *
 * Empty item collections are a valid open state. They return an owned snapshot with empty bounds and no
 * rows, allowing composition to suppress stale caret metadata while painting no invented placeholder item.
 */
[[nodiscard]] inline std::optional<ComboBoxPopupPresentationSnapshot>
buildComboBoxPopupPresentation(
    const ComboBox& combo,
    Rect absolute_anchor,
    Rect viewport,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow,
    presentation::PopupVerticalSide preferred_side =
        presentation::PopupVerticalSide::below) {
    if (!combo.isDropDownOpen() ||
        !combo.hasFocus() ||
        !combo.isEnabled() ||
        !combo.isVisible() ||
        absolute_anchor.isEmpty() ||
        viewport.isEmpty()) {
        return std::nullopt;
    }

    ComboBoxPopupPresentationSnapshot snapshot;
    snapshot.preview_index = combo.previewIndex();
    snapshot.text_style = combo.textStyle();
    snapshot.side = preferred_side;
    snapshot.items.reserve(combo.itemCount());

    for (std::size_t index = 0; index < combo.itemCount(); ++index) {
        snapshot.items.emplace_back(combo.itemAt(index));
    }

    const auto measured =
        measureComboBoxPopupPresentation(snapshot, ambiguous_width);
    if (!measured.has_value()) {
        return std::nullopt;
    }

    if (snapshot.items.empty()) {
        /*
         * Generic anchored placement intentionally rejects zero-area popups. An open empty ComboBox has
         * no semantic rows to draw, so preserve it as a valid no-overlay snapshot rather than weakening
         * the generic geometry contract or manufacturing a placeholder row.
         */
        snapshot.bounds = {};
        return snapshot;
    }

    Size popup_size = measured->size;
    popup_size.width = std::max(popup_size.width, absolute_anchor.width);

    const auto placement = presentation::placeAnchoredPopup(
        absolute_anchor,
        popup_size,
        viewport,
        preferred_side);
    if (!placement.has_value()) {
        return std::nullopt;
    }

    snapshot.bounds = placement->bounds;
    snapshot.side = placement->side;
    return snapshot;
}

/**
 * Paints one owned ComboBox popup snapshot into ScreenBuffer transactionally.
 *
 * Preflight validates every UTF-8 item, preview identity, exact fixed-row height and complete buffer
 * containment before the first cell is touched. Synthetic or stale snapshots therefore fail without
 * partially overwriting the previous frame.
 *
 * Row chrome is deliberately minimal: one leading blank, item text, trailing blank, with the remainder
 * of a wider anchor-expanded row filled by styled spaces. The preview row toggles the base inverse bit
 * rather than forcing inverse=true, so user styles that are already inverse still receive a visible
 * contrast change.
 *
 * @returns true when the snapshot is valid and painting/no-op painting completed; false when preflight
 *          rejected the snapshot and the buffer remains unchanged.
 */
[[nodiscard]] inline bool
renderComboBoxPopupPresentation(
    ScreenBuffer& buffer,
    const ComboBoxPopupPresentationSnapshot& snapshot,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    const auto measured =
        measureComboBoxPopupPresentation(snapshot, ambiguous_width);
    if (!measured.has_value()) {
        return false;
    }

    if (snapshot.items.empty()) {
        return snapshot.bounds.isEmpty() && !snapshot.preview_index.has_value();
    }

    if (snapshot.bounds.isEmpty() ||
        snapshot.bounds.width < measured->size.width ||
        snapshot.bounds.height != measured->size.height ||
        !combo_box_popup_detail::rectangleInsideBuffer(buffer, snapshot.bounds)) {
        return false;
    }

    /*
     * Preflight every row rectangle through the shared backend-neutral fixed-row helper before any
     * mutation. This is intentionally redundant with the height equality above: the row helper is the
     * geometry contract later pointer hit-testing will reuse, so renderer tests should exercise it now.
     */
    for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
        if (!presentation::fixedPopupRowBounds(
                snapshot.bounds,
                snapshot.items.size(),
                index,
                1).has_value()) {
            return false;
        }
    }

    for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
        const auto row = presentation::fixedPopupRowBounds(
            snapshot.bounds,
            snapshot.items.size(),
            index,
            1);
        if (!row.has_value()) {
            // Complete preflight above makes this branch unreachable.
            return false;
        }

        TextStyle style = snapshot.text_style;
        if (snapshot.preview_index == std::optional<std::size_t>{index}) {
            style.inverse = !style.inverse;
        }

        buffer.fill(
            *row,
            Cell{U' ', CellRole::normal, style});

        combo_box_popup_detail::writePopupLine(
            buffer,
            Point{
                static_cast<Coordinate>(row->x + 1),
                row->y,
            },
            snapshot.items[index],
            ambiguous_width,
            style);
    }

    return true;
}

/**
 * Composes the current open ComboBox popup over an immutable terminal base frame.
 *
 * Closed ComboBoxes are a value-preserving pass-through: the returned frame is an independent copy of
 * base with identical caret metadata. Open state first builds/validates the complete popup snapshot
 * against exactly base.buffer's viewport. Only after successful construction is the frame copied and
 * painted. Thus placement/text failure returns std::nullopt without mutating the source frame.
 *
 * Open ComboBox interaction suppresses the hardware caret in the composed frame. Normally the focused
 * ComboBox means the underlying widget pass already has no TextField caret, but making the overlay
 * contract explicit prevents stale caller-supplied caret metadata from leaking through an active popup.
 */
[[nodiscard]] inline std::optional<TerminalPresentationFrame>
composeComboBoxPopupFrame(
    const TerminalPresentationFrame& base,
    const ComboBox& combo,
    Rect absolute_anchor,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow,
    presentation::PopupVerticalSide preferred_side =
        presentation::PopupVerticalSide::below) {
    if (!combo.isDropDownOpen()) {
        return base;
    }

    const Size viewport_size = base.buffer.size();
    const Rect viewport{0, 0, viewport_size.width, viewport_size.height};

    const auto snapshot = buildComboBoxPopupPresentation(
        combo,
        absolute_anchor,
        viewport,
        ambiguous_width,
        preferred_side);
    if (!snapshot.has_value()) {
        return std::nullopt;
    }

    TerminalPresentationFrame result = base;
    result.caret.reset();

    if (!renderComboBoxPopupPresentation(
            result.buffer,
            *snapshot,
            ambiguous_width)) {
        return std::nullopt;
    }

    return result;
}

} // namespace sasd::ui::terminal
