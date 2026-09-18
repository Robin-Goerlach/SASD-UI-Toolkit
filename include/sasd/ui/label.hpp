#pragma once

#include <sasd/ui/widget.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * Non-interactive semantic text widget.
 *
 * Public text is UTF-8, matching the toolkit-wide text boundary. Label deliberately does not guess
 * an intrinsic text width in the platform-neutral core: terminal cells, rendered fonts and native
 * controls require different measurement services. M2 will attach real text metrics above this
 * semantic state instead of baking byte/code-point counts into the public layout contract.
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
     * The API expects UTF-8. Validation/normalization policy is deliberately not forced into this
     * first semantic widget; presentation/text services must handle malformed input safely.
     */
    void setText(std::string text);

private:
    std::string text_;
};

} // namespace sasd::ui
