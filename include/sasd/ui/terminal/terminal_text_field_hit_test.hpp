#pragma once

#include <sasd/ui/container.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>
#include <sasd/ui/text_field.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Maps terminal-cell pointer positions onto TextField Unicode-scalar geometry.
 *
 * The semantic TextField remains backend-neutral. This helper is terminal-specific because the
 * mapping depends on fixed terminal cells, East Asian width policy and the TextField's current
 * horizontally scrolled terminal viewport. It is deliberately read-only: gesture ownership belongs
 * to PointerRouter, while applying returned caret/scalar information belongs to higher-level
 * interaction policy through TextField::setCursorPosition()/setSelection().
 *
 * Terminal pointer coordinates identify whole cells rather than sub-pixel positions. Caret mapping
 * treats an interior cell as its horizontal center, while scalar mapping answers a different question:
 * which actually painted Unicode scalar owns this cell? Keeping those contracts separate prevents a
 * trailing caret-space cell or control chrome from being mistaken for text during word selection.
 */
class TerminalTextFieldHitTest final {
public:
    TerminalTextFieldHitTest() = delete;

    /**
     * Returns the nearest representable caret boundary for a point geometrically inside field.
     *
     * point uses the same top-level logical coordinate system as PointerEvent. Chrome cells clamp to
     * the first/last caret boundary representable in the current terminal viewport. std::nullopt means
     * the field is hidden/disabled, point is outside the clipped Widget, the control has no text
     * interior, or its current Unicode content cannot be represented by the terminal Cell model.
     */
    [[nodiscard]] static std::optional<std::size_t> caretIndexAt(
        const TextField& field,
        Point point,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        return mapCaretIndex(field, point, ambiguous_width, PointPolicy::require_inside_field);
    }

    /**
     * Returns the Unicode-scalar index whose painted terminal cells contain point.
     *
     * Unlike caretIndexAt(), this operation does not choose a nearest boundary and never clamps chrome
     * or trailing caret space onto text. It succeeds only when point lies on cells occupied by one
     * scalar in the current terminal viewport. Both cells of a two-cell scalar return the same scalar
     * index; a wide scalar that cannot be painted completely at the right edge has no hit geometry.
     *
     * This stricter contract is intended for semantic operations such as double-click word selection,
     * where callers need the identity of text actually under the pointer rather than a nearby insertion
     * position. Hidden/disabled fields, clipped-out points and unsupported Unicode geometry return
     * std::nullopt rather than manufacturing a scalar identity.
     */
    [[nodiscard]] static std::optional<std::size_t> scalarIndexAt(
        const TextField& field,
        Point point,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        if (!field.isVisible() || !field.isEnabled() || !HitTest::contains(field, point)) {
            return std::nullopt;
        }

        const auto viewport = buildViewport(field, ambiguous_width);
        if (!viewport.has_value()) {
            return std::nullopt;
        }

        const std::int64_t raw_cell =
            static_cast<std::int64_t>(point.x) - viewport->content_x;
        if (raw_cell < 0 || raw_cell >= viewport->viewport_width) {
            /*
             * Left/right chrome and points outside the text interior do not identify a scalar. This is
             * intentionally stricter than caretIndexAt(), which clamps chrome to a useful insertion
             * boundary. Word selection must never turn UI chrome into adjacent text ownership.
             */
            return std::nullopt;
        }

        for (std::size_t index = viewport->start_index;
             index < viewport->scalar_count;
             ++index) {
            const ScalarGeometry& scalar = viewport->scalars[index];
            const std::int64_t relative_start = scalar.column - viewport->start_column;
            const std::int64_t relative_end =
                relative_start + static_cast<std::int64_t>(scalar.width);

            if (relative_start >= viewport->viewport_width) {
                break;
            }

            if (relative_end > viewport->viewport_width) {
                /*
                 * TerminalPresentationSink refuses to paint a partial wide scalar at the right edge.
                 * Treat the corresponding cells as non-text geometry as well; reporting a scalar here
                 * would make interaction disagree with what the user can actually see.
                 */
                break;
            }

            if (raw_cell >= relative_start && raw_cell < relative_end) {
                /*
                 * A wide scalar owns both its lead and continuation cells. Returning the same semantic
                 * index for the complete half-open cell span preserves Unicode-scalar identity without
                 * pretending that a continuation cell is independent text.
                 */
                return index;
            }
        }

        /*
         * A remaining interior cell is presentation space (most importantly the reserved caret cell),
         * not text. Returning nullopt keeps strict scalar-hit semantics distinct from caret clamping.
         */
        return std::nullopt;
    }

    /**
     * Returns the nearest representable caret boundary for an already captured drag gesture.
     *
     * Unlike caretIndexAt(), point may be outside the TextField. Horizontal positions clamp to the
     * first/last boundary visible in the current viewport and vertical position is intentionally
     * ignored once capture has established gesture ownership. No auto-scroll is hidden here: if the
     * caller applies the returned active end and TextField's semantic cursor moves the viewport, the
     * next drag event is evaluated against that new viewport.
     */
    [[nodiscard]] static std::optional<std::size_t> caretIndexForDrag(
        const TextField& field,
        Point point,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        return mapCaretIndex(field, point, ambiguous_width, PointPolicy::allow_captured_drag);
    }

private:
    enum class PointPolicy {
        require_inside_field,
        allow_captured_drag,
    };

    struct ScalarGeometry {
        int width{1};
        std::int64_t column{0};
    };

    struct ViewportGeometry {
        std::int64_t content_x{0};
        std::int64_t viewport_width{0};
        std::size_t scalar_count{0};
        std::size_t cursor_index{0};
        std::size_t start_index{0};
        std::int64_t start_column{0};
        std::vector<ScalarGeometry> scalars;
    };

    struct CaretCandidate {
        std::size_t index{0};
        std::int64_t relative_column{0};
    };

    /**
     * Adds one Coordinate to a widened absolute position without allowing signed overflow.
     *
     * Widget bounds are int32 values, but parent-relative offsets may accumulate through an arbitrary
     * visual-tree depth. Returning false is preferable to wrapping a malformed/pathological tree into
     * an apparently valid terminal cell position.
     */
    [[nodiscard]] static bool addCoordinate(std::int64_t& value, Coordinate delta) noexcept {
        const std::int64_t widened = static_cast<std::int64_t>(delta);
        if (widened > 0 && value > std::numeric_limits<std::int64_t>::max() - widened) {
            return false;
        }
        if (widened < 0 && value < std::numeric_limits<std::int64_t>::min() - widened) {
            return false;
        }
        value += widened;
        return true;
    }

    /**
     * Reconstructs the terminal TextField viewport from semantic state and terminal width rules.
     *
     * The policy mirrors TerminalPresentationSink's established ADR 0019 contract: two chrome cells
     * surround the text interior; viewport starts are Unicode-scalar boundaries; and the start advances
     * just far enough that the current caret column remains strictly inside the visible interior. The
     * function validates the same simple-cell Unicode domain before exposing pointer geometry.
     */
    [[nodiscard]] static std::optional<ViewportGeometry> buildViewport(
        const TextField& field,
        AmbiguousWidthMode ambiguous_width) {
        const Rect local_bounds = field.bounds();
        const std::int64_t raw_width = static_cast<std::int64_t>(local_bounds.width);
        const std::int64_t viewport_width = raw_width > 2 ? raw_width - 2 : 0;
        if (viewport_width <= 0) {
            return std::nullopt;
        }

        const TextMeasurement measurement = TextMetrics::measureUtf8(field.text(), ambiguous_width);
        if (!measurement.simpleCellRenderable() || measurement.rows != 1) {
            return std::nullopt;
        }

        ViewportGeometry result;
        result.viewport_width = viewport_width;
        result.scalars.reserve(field.text().size());

        std::int64_t total_columns = 0;
        for (std::size_t offset = 0; offset < field.text().size();) {
            const DecodedCodePoint decoded = TextMetrics::decodeOne(field.text(), offset);
            if (decoded.consumed == 0) {
                break;
            }
            offset += decoded.consumed;

            const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
            if (width <= 0) {
                /*
                 * measureUtf8() already rejected the unsupported simple-cell cases. Keep this guard
                 * defensive so a later TextMetrics change cannot silently turn pointer arithmetic into
                 * geometry for a scalar the presentation layer itself cannot paint faithfully.
                 */
                return std::nullopt;
            }

            result.scalars.push_back({width, total_columns});
            total_columns += static_cast<std::int64_t>(width);
        }

        result.scalar_count = result.scalars.size();
        result.cursor_index = std::min(field.cursorPosition(), result.scalar_count);

        const std::int64_t cursor_column =
            result.cursor_index < result.scalar_count
                ? result.scalars[result.cursor_index].column
                : total_columns;

        result.start_index = 0;
        while (result.start_index < result.cursor_index) {
            const std::int64_t candidate_column = result.scalars[result.start_index].column;
            if (cursor_column - candidate_column < result.viewport_width) {
                break;
            }
            ++result.start_index;
        }

        result.start_column =
            result.start_index < result.scalar_count
                ? result.scalars[result.start_index].column
                : total_columns;

        /*
         * Convert field-local x into the top-level PointerEvent coordinate system. Presentation uses
         * the same parent-relative accumulation. One additional cell skips the left terminal chrome.
         */
        std::int64_t absolute_x = static_cast<std::int64_t>(local_bounds.x);
        for (const Container* parent = field.parent(); parent != nullptr; parent = parent->parent()) {
            if (!addCoordinate(absolute_x, parent->bounds().x)) {
                return std::nullopt;
            }
        }

        if (absolute_x == std::numeric_limits<std::int64_t>::max()) {
            return std::nullopt;
        }
        result.content_x = absolute_x + 1;
        return result;
    }

    /** Builds only caret boundaries that can actually be represented in the current viewport. */
    [[nodiscard]] static std::vector<CaretCandidate> buildCandidates(
        const ViewportGeometry& viewport) {
        std::vector<CaretCandidate> candidates;
        candidates.reserve(viewport.scalar_count - viewport.start_index + 1);
        candidates.push_back({viewport.start_index, 0});

        for (std::size_t index = viewport.start_index;
             index < viewport.scalar_count;
             ++index) {
            const ScalarGeometry& scalar = viewport.scalars[index];
            const std::int64_t relative_start = scalar.column - viewport.start_column;
            const std::int64_t relative_end =
                relative_start + static_cast<std::int64_t>(scalar.width);

            if (relative_start >= viewport.viewport_width) {
                break;
            }

            if (relative_end > viewport.viewport_width) {
                /*
                 * TerminalPresentationSink deliberately refuses to paint half of a wide scalar at the
                 * right edge. Do not manufacture a caret boundary through geometry that is not visible.
                 */
                break;
            }

            if (relative_end < viewport.viewport_width) {
                candidates.push_back({index + 1, relative_end});
            }

            if (relative_end == viewport.viewport_width) {
                /*
                 * The scalar itself can occupy the final interior cell(s), but its following caret
                 * position would coincide with the right chrome. Presentation requires caret_column <
                 * viewport_width, so that boundary is intentionally not a hit candidate yet.
                 */
                break;
            }
        }

        return candidates;
    }

    [[nodiscard]] static std::optional<std::size_t> mapCaretIndex(
        const TextField& field,
        Point point,
        AmbiguousWidthMode ambiguous_width,
        PointPolicy point_policy) {
        if (!field.isVisible() || !field.isEnabled()) {
            return std::nullopt;
        }

        if (point_policy == PointPolicy::require_inside_field &&
            !HitTest::contains(field, point)) {
            return std::nullopt;
        }

        const auto viewport = buildViewport(field, ambiguous_width);
        if (!viewport.has_value()) {
            return std::nullopt;
        }

        const std::vector<CaretCandidate> candidates = buildCandidates(*viewport);
        if (candidates.empty()) {
            return std::nullopt;
        }

        const std::int64_t raw_cell =
            static_cast<std::int64_t>(point.x) - viewport->content_x;

        /*
         * Chrome and captured motion outside the horizontal interior clamp to an actual boundary, not
         * to the center of the first/last cell. This preserves the intuitive meaning of clicking the
         * left/right delimiter and mirrors rendered captured-drag clamping.
         */
        if (raw_cell < 0) {
            return candidates.front().index;
        }
        if (raw_cell >= viewport->viewport_width) {
            return candidates.back().index;
        }

        if (candidates.size() == 1) {
            return candidates.front().index;
        }

        /*
         * PointerEvent gives an integer terminal cell, so evaluate its horizontal center without
         * floating point: cell n has doubled center 2*n+1. Adjacent caret boundaries use integer cell
         * columns, and comparing against their summed columns is exactly a midpoint comparison. A tie
         * deliberately chooses the later boundary by using '<' rather than '<='.
         */
        const std::int64_t doubled_pointer_center = raw_cell * 2 + 1;
        for (std::size_t index = 0; index + 1 < candidates.size(); ++index) {
            const std::int64_t boundary_sum =
                candidates[index].relative_column +
                candidates[index + 1].relative_column;
            if (doubled_pointer_center < boundary_sum) {
                return candidates[index].index;
            }
        }

        return candidates.back().index;
    }
};

} // namespace sasd::ui::terminal
