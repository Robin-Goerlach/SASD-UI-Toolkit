#pragma once

#include <sasd/ui/widget.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * Non-interactive semantic text widget.
 *
 * Public text is UTF-8, matching the toolkit-wide text boundary. Label does not know whether logical
 * units are terminal cells, rendered font units or native-control metrics. When a MeasurementContext
 * is supplied, the context measures text for the active presentation environment.
 */
class Label : public Widget {
public:
    Label() = default;
    explicit Label(std::string text) : text_{std::move(text)} {}

    /** Returns the UTF-8 text currently represented by this label. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /**
     * Replaces the label text.
     *
     * Text can affect intrinsic size and presentation, so both caches are invalidated. Supplying the
     * identical byte sequence is a no-op and preserves existing measure/presentation state.
     *
     * The API expects UTF-8. Validation/normalization policy remains a presentation/text-service
     * responsibility so the semantic widget does not embed one backend's Unicode policy.
     */
    void setText(std::string text);

protected:
    /**
     * Measures text through the caller-provided presentation context.
     *
     * The context-free Widget::measure() path intentionally retains the normal preferred-size
     * fallback; only context-aware measurement claims a real text metric.
     */
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;

private:
    std::string text_;
};

} // namespace sasd::ui
