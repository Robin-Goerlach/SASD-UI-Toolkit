#pragma once

#include <sasd/ui/clipboard.hpp>

#include <optional>
#include <string>
#include <utility>

namespace sasd::ui::testing {

/**
 * Deterministic in-memory Clipboard used by headless tests and semantic consumers.
 *
 * This is intentionally test infrastructure rather than an application clipboard implementation.
 * It mirrors Clipboard's ownership semantics exactly: reads return copies, writes replace the owned
 * payload, and clear() distinguishes "no text" from a present empty string. Future TextField or
 * command tests can therefore exercise copy/paste behavior without depending on the host OS clipboard.
 */
class MemoryClipboard final : public Clipboard {
public:
    MemoryClipboard() = default;

    [[nodiscard]] std::optional<std::string> readText() const override {
        return text_;
    }

    void writeText(std::string text) override {
        text_ = std::move(text);
    }

    void clear() override {
        text_.reset();
    }

private:
    std::optional<std::string> text_{};
};

} // namespace sasd::ui::testing
