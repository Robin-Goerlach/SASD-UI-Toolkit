#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>

#include "rendered_geometry.hpp"
#include "rendered_text_field_viewport.hpp"

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/text_field.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace sasd::ui::rendered {
namespace {

enum class PointPolicy {
    require_inside_field,
    allow_captured_drag,
};

[[nodiscard]] std::optional<std::size_t> mapCaretIndex(
    const TextField& field,
    Point point,
    const RenderedMeasurementContext& metrics,
    PointPolicy point_policy) {
    if (!field.isVisible() || !field.isEnabled()) {
        return std::nullopt;
    }

    if (point_policy == PointPolicy::require_inside_field &&
        !HitTest::contains(field, point)) {
        return std::nullopt;
    }

    const auto absolute = detail::absoluteRectOf(field);
    if (!absolute.has_value()) {
        return std::nullopt;
    }

    /*
     * Snapshot theme geometry once for this mapping operation. A mutable environment must bump the
     * MeasurementContext revision when theme metrics change, but one hit test should still be
     * internally coherent even if the provider is backed by live platform state.
     */
    const RenderedThemeMetrics theme = metrics.themeMetrics().normalized();
    const auto viewport =
        detail::buildTextFieldViewport(field, *absolute, metrics, theme);
    if (!viewport.has_value() || viewport->content.isEmpty()) {
        return std::nullopt;
    }

    /*
     * Build only the caret boundaries that can actually appear in the current viewport. The shared
     * viewport calculation already established start_index/start_advance from the current cursor.
     * Querying from that point forward preserves full-run shaping context through
     * textAdvanceToScalar() and avoids inventing substring measurements.
     */
    struct Candidate {
        std::size_t index{0};
        std::int64_t relative_advance{0};
    };

    std::vector<Candidate> candidates;
    candidates.reserve(viewport->scalar_count - viewport->start_index + 1);

    Coordinate previous_advance = viewport->start_advance;

    for (std::size_t index = viewport->start_index;
         index <= viewport->scalar_count;
         ++index) {
        Coordinate advance = viewport->start_advance;

        if (index != viewport->start_index) {
            const auto measured = metrics.textAdvanceToScalar(field.text(), index);
            if (!measured.has_value() ||
                *measured < previous_advance ||
                *measured < viewport->start_advance) {
                /*
                 * An unrepresentable/retrograde boundary might lie inside the visible run. We cannot
                 * safely skip over it because doing so could map through a shaping cluster that the
                 * metric provider explicitly said the current caret model cannot express.
                 */
                return std::nullopt;
            }
            advance = *measured;
            previous_advance = advance;
        }

        const std::int64_t relative =
            static_cast<std::int64_t>(advance) -
            static_cast<std::int64_t>(viewport->start_advance);

        if (relative > static_cast<std::int64_t>(viewport->text_capacity)) {
            break;
        }

        candidates.push_back({index, relative});
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    /*
     * The same scalar-selection rule is useful for an ordinary click and for a pointer captured by a
     * TextField drag. caretIndexAt() has already rejected points outside the field. The drag variant
     * deliberately permits them, so clamping the horizontal coordinate here maps motion to the first
     * or last boundary that is representable in the *current* viewport. Vertical position is not part
     * of single-line caret geometry once gesture ownership has already been established by the host.
     *
     * Auto-scroll is intentionally not hidden in this helper. If applying the returned active end
     * moves TextField's viewport, the next drag motion is evaluated against that new viewport through
     * the same shared builder.
     */
    const std::int64_t raw_relative =
        static_cast<std::int64_t>(point.x) -
        static_cast<std::int64_t>(viewport->content.x);

    const std::int64_t pointer_relative = std::clamp<std::int64_t>(
        raw_relative,
        0,
        static_cast<std::int64_t>(viewport->text_capacity));

    /*
     * Compare against the midpoint between adjacent caret boundaries. Widened arithmetic avoids
     * overflow for pathological metric values. A tie chooses the later boundary by using '<'
     * instead of '<=', which feels natural when clicking exactly halfway through a glyph advance.
     *
     * Equal advances (for example zero-advance scalar boundaries) naturally collapse to one visual
     * coordinate; the loop walks across ties and chooses the later representable scalar.
     */
    for (std::size_t i = 0; i + 1 < candidates.size(); ++i) {
        const std::int64_t doubled_pointer = pointer_relative * 2;
        const std::int64_t boundary_sum =
            candidates[i].relative_advance +
            candidates[i + 1].relative_advance;

        if (doubled_pointer < boundary_sum) {
            return candidates[i].index;
        }
    }

    return candidates.back().index;
}

} // namespace

std::optional<std::size_t> RenderedTextFieldHitTest::caretIndexAt(
    const TextField& field,
    Point point,
    const RenderedMeasurementContext& metrics) {
    return mapCaretIndex(
        field,
        point,
        metrics,
        PointPolicy::require_inside_field);
}

std::optional<std::size_t> RenderedTextFieldHitTest::caretIndexForDrag(
    const TextField& field,
    Point point,
    const RenderedMeasurementContext& metrics) {
    return mapCaretIndex(
        field,
        point,
        metrics,
        PointPolicy::allow_captured_drag);
}

} // namespace sasd::ui::rendered
