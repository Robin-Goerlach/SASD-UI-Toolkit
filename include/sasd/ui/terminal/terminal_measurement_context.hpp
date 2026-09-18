#pragma once

#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

namespace sasd::ui::terminal {

/**
 * MeasurementContext implementation whose logical units are terminal columns and rows.
 *
 * It reuses the exact TextMetrics policy used by TerminalPresentationSink so layout and rendering
 * cannot silently disagree about wide/ambiguous character widths.
 */
class TerminalMeasurementContext final : public MeasurementContext {
public:
    explicit TerminalMeasurementContext(
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept
        : ambiguous_width_{ambiguous_width} {}

    [[nodiscard]] AmbiguousWidthMode ambiguousWidthMode() const noexcept {
        return ambiguous_width_;
    }

    /**
     * Changes East-Asian-Ambiguous width policy.
     *
     * revision() is derived directly from the current policy, so two independent terminal contexts
     * with the same revision always promise equivalent measurement behavior.
     */
    void setAmbiguousWidthMode(AmbiguousWidthMode mode) noexcept {
        ambiguous_width_ = mode;
    }

    [[nodiscard]] Size measureText(std::string_view utf8_text) const override;

    /**
     * Measures terminal Button chrome exactly as TerminalPresentationSink renders it:
     *
     *     [ caption ]
     *
     * Focus/disabled variants use different ASCII delimiters but preserve the same cell footprint.
     */
    [[nodiscard]] Size measureButton(std::string_view utf8_text) const override;

    /**
     * Measures terminal TextField as two delimiter cells plus content and one reserved caret cell.
     *
     * Reserving caret room even for non-empty intrinsic fields keeps focus transitions layout-stable:
     * a field measured exactly to its natural size can show the complete text and an end-of-text
     * hardware caret without immediately scrolling.
     */
    [[nodiscard]] Size measureTextField(std::string_view utf8_text) const override;

    [[nodiscard]] std::uint64_t revision() const noexcept override {
        return ambiguous_width_ == AmbiguousWidthMode::narrow ? 0U : 1U;
    }

private:
    AmbiguousWidthMode ambiguous_width_{AmbiguousWidthMode::narrow};
};

} // namespace sasd::ui::terminal
