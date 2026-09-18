#pragma once

#include <sasd/ui/terminal/terminal_device.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sasd::ui::terminal::testing {

/**
 * Deterministic TerminalDevice test double.
 *
 * It records session transitions and exact byte writes without requiring a TTY. Failure injection
 * covers constructor rollback/transport paths in TerminalSession while native adapters remain
 * compile-tested on their real CI operating systems.
 */
class MockTerminalDevice final : public TerminalDevice {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "mock-terminal-device";
    }

    [[nodiscard]] bool isInteractive() const noexcept override {
        return interactive_;
    }

    [[nodiscard]] Size size() const override {
        if (fail_size_) {
            throw std::runtime_error("injected terminal size failure");
        }
        return size_;
    }

    void beginSession(const TerminalSessionOptions& options) override {
        ++begin_count_;
        if (fail_begin_) {
            throw std::runtime_error("injected begin failure");
        }
        if (active_) {
            throw std::logic_error("mock terminal session already active");
        }

        active_ = true;
        last_options_ = options;
    }

    void endSession() noexcept override {
        ++end_count_;
        active_ = false;
    }

    void write(std::string_view bytes) override {
        ++write_count_;
        if (fail_write_) {
            throw std::runtime_error("injected write failure");
        }
        if (!active_) {
            throw std::logic_error("mock terminal write requires active session");
        }

        writes_.emplace_back(bytes);
    }

    void setInteractive(bool value) noexcept { interactive_ = value; }
    void setSize(Size value) noexcept { size_ = value; }
    void setFailBegin(bool value) noexcept { fail_begin_ = value; }
    void setFailWrite(bool value) noexcept { fail_write_ = value; }
    void setFailSize(bool value) noexcept { fail_size_ = value; }

    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] int beginCount() const noexcept { return begin_count_; }
    [[nodiscard]] int endCount() const noexcept { return end_count_; }
    [[nodiscard]] int writeCount() const noexcept { return write_count_; }
    [[nodiscard]] const TerminalSessionOptions& lastOptions() const noexcept {
        return last_options_;
    }
    [[nodiscard]] const std::vector<std::string>& writes() const noexcept {
        return writes_;
    }

private:
    bool interactive_{true};
    bool active_{false};
    bool fail_begin_{false};
    bool fail_write_{false};
    bool fail_size_{false};
    Size size_{80, 24};
    TerminalSessionOptions last_options_{};
    int begin_count_{0};
    int end_count_{0};
    int write_count_{0};
    std::vector<std::string> writes_;
};

} // namespace sasd::ui::terminal::testing
