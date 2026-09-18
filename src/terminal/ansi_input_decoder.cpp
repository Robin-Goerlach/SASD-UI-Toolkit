#include <sasd/ui/terminal/ansi_input_decoder.hpp>

#include <sasd/ui/text/utf8.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace sasd::ui::terminal {
namespace {

constexpr unsigned char escape_byte = 0x1BU;

struct EscapeResult {
    std::size_t consumed{0};
    bool need_more{false};
    std::optional<KeyEvent> event;
};

[[nodiscard]] std::size_t expectedUtf8Length(unsigned char first) noexcept {
    if (first <= 0x7FU) {
        return 1;
    }
    if (first >= 0xC2U && first <= 0xDFU) {
        return 2;
    }
    if (first >= 0xE0U && first <= 0xEFU) {
        return 3;
    }
    if (first >= 0xF0U && first <= 0xF4U) {
        return 4;
    }

    // Invalid lead/continuation bytes are resolved immediately as U+FFFD by the shared decoder.
    return 1;
}

void appendText(std::vector<Event>& events, std::string_view bytes) {
    if (bytes.empty()) {
        return;
    }

    if (!events.empty()) {
        if (auto* existing = std::get_if<TextInputEvent>(&events.back())) {
            existing->text.append(bytes);
            return;
        }
    }

    events.emplace_back(TextInputEvent{std::string{bytes}});
}

void appendScalarText(std::vector<Event>& events, char32_t scalar) {
    std::string encoded;
    utf8::appendScalar(encoded, scalar);
    appendText(events, encoded);
}

[[nodiscard]] KeyModifier modifierFromXtermParameter(unsigned value) noexcept {
    switch (value) {
    case 2:
        return KeyModifier::shift;
    case 3:
        return KeyModifier::alt;
    case 4:
        return KeyModifier::shift | KeyModifier::alt;
    case 5:
        return KeyModifier::control;
    case 6:
        return KeyModifier::shift | KeyModifier::control;
    case 7:
        return KeyModifier::alt | KeyModifier::control;
    case 8:
        return KeyModifier::shift | KeyModifier::alt | KeyModifier::control;
    default:
        return KeyModifier::none;
    }
}

[[nodiscard]] std::optional<unsigned> parseUnsigned(std::string_view text) noexcept {
    if (text.empty()) {
        return std::nullopt;
    }

    unsigned result = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return std::nullopt;
        }

        const unsigned digit = static_cast<unsigned>(character - '0');
        if (result > 999U) {
            // Terminal parameters relevant here are tiny. Reject pathological/unbounded values.
            return std::nullopt;
        }
        result = result * 10U + digit;
    }

    return result;
}

struct CsiParameters {
    std::optional<unsigned> primary;
    KeyModifier modifiers{KeyModifier::none};
};

[[nodiscard]] CsiParameters parseCsiParameters(std::string_view parameters) noexcept {
    CsiParameters result;

    const std::size_t separator = parameters.find(';');
    const std::string_view first =
        separator == std::string_view::npos ? parameters : parameters.substr(0, separator);

    result.primary = parseUnsigned(first);

    if (separator != std::string_view::npos) {
        const std::string_view second = parameters.substr(separator + 1);
        if (const auto modifier = parseUnsigned(second)) {
            result.modifiers = modifierFromXtermParameter(*modifier);
        }
    }

    return result;
}

[[nodiscard]] std::optional<Key> tildeKey(unsigned primary) noexcept {
    switch (primary) {
    case 1:
    case 7:
        return Key::home;
    case 3:
        return Key::delete_forward;
    case 4:
    case 8:
        return Key::end;
    case 5:
        return Key::page_up;
    case 6:
        return Key::page_down;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] EscapeResult parseCsi(std::string_view input, bool flush) {
    // input starts with ESC '['.
    std::size_t final_index = 2;

    while (final_index < input.size()) {
        const unsigned char byte = static_cast<unsigned char>(input[final_index]);
        if (byte >= 0x40U && byte <= 0x7EU) {
            break;
        }
        ++final_index;
    }

    if (final_index >= input.size()) {
        if (!flush) {
            return {.need_more = true};
        }

        /*
         * Timeout resolution preserves user bytes instead of silently dropping an incomplete CSI:
         * interpret ESC itself now and let the remaining '['/parameters be decoded normally.
         */
        return {1, false, KeyEvent{Key::escape, true, KeyModifier::none}};
    }

    const char final = input[final_index];
    const std::string_view parameters = input.substr(2, final_index - 2);
    const CsiParameters parsed = parseCsiParameters(parameters);
    const std::size_t consumed = final_index + 1;

    KeyModifier modifiers = parsed.modifiers;
    std::optional<Key> key;

    switch (final) {
    case 'A':
        key = Key::up;
        break;
    case 'B':
        key = Key::down;
        break;
    case 'C':
        key = Key::right;
        break;
    case 'D':
        key = Key::left;
        break;
    case 'H':
        key = Key::home;
        break;
    case 'F':
        key = Key::end;
        break;
    case 'Z':
        key = Key::tab;
        modifiers = KeyModifier::shift;
        break;
    case '~':
        if (parsed.primary) {
            key = tildeKey(*parsed.primary);
        }
        break;
    default:
        break;
    }

    if (!key) {
        // Unknown but complete CSI is consumed atomically so its parameter bytes never leak as text.
        return {consumed, false, std::nullopt};
    }

    return {consumed, false, KeyEvent{*key, true, modifiers}};
}

[[nodiscard]] EscapeResult parseSs3(std::string_view input, bool flush) {
    // ESC O <final>, commonly emitted by application-cursor mode for arrows/Home/End.
    if (input.size() < 3) {
        if (!flush) {
            return {.need_more = true};
        }
        return {1, false, KeyEvent{Key::escape, true, KeyModifier::none}};
    }

    std::optional<Key> key;
    switch (input[2]) {
    case 'A':
        key = Key::up;
        break;
    case 'B':
        key = Key::down;
        break;
    case 'C':
        key = Key::right;
        break;
    case 'D':
        key = Key::left;
        break;
    case 'H':
        key = Key::home;
        break;
    case 'F':
        key = Key::end;
        break;
    default:
        break;
    }

    if (!key) {
        return {3, false, std::nullopt};
    }

    return {3, false, KeyEvent{*key, true, KeyModifier::none}};
}

[[nodiscard]] EscapeResult parseEscape(std::string_view input, bool flush) {
    if (input.size() == 1) {
        if (!flush) {
            return {.need_more = true};
        }
        return {1, false, KeyEvent{Key::escape, true, KeyModifier::none}};
    }

    if (input[1] == '[') {
        return parseCsi(input, flush);
    }
    if (input[1] == 'O') {
        return parseSs3(input, flush);
    }

    /*
     * Alt+printable is not claimed yet because the current Key enum has no letter/digit identity.
     * Once a second byte proves this is not CSI/SS3, treat ESC as an ordinary Escape key and allow
     * the following byte to be decoded independently as text/control input.
     */
    return {1, false, KeyEvent{Key::escape, true, KeyModifier::none}};
}

} // namespace

std::vector<Event> AnsiInputDecoder::feed(std::string_view bytes) {
    pending_.append(bytes);
    return decode(false);
}

std::vector<Event> AnsiInputDecoder::flushPending() {
    return decode(true);
}

std::vector<Event> AnsiInputDecoder::decode(bool flush) {
    std::vector<Event> events;
    std::size_t offset = 0;

    while (offset < pending_.size()) {
        const unsigned char byte = static_cast<unsigned char>(pending_[offset]);
        const std::string_view remaining{pending_.data() + offset, pending_.size() - offset};

        if (byte == escape_byte) {
            const EscapeResult parsed = parseEscape(remaining, flush);
            if (parsed.need_more) {
                break;
            }

            if (parsed.event) {
                events.emplace_back(*parsed.event);
            }
            offset += parsed.consumed;
            continue;
        }

        if (byte == static_cast<unsigned char>('\r') ||
            byte == static_cast<unsigned char>('\n')) {
            /*
             * Treat CRLF as one Enter gesture if a device/session happens to deliver both bytes.
             * Raw POSIX terminals normally deliver CR for Return, but the decoder should not create a
             * duplicate activation merely because another environment translates to CRLF.
             */
            if (byte == static_cast<unsigned char>('\r') &&
                offset + 1 < pending_.size() &&
                pending_[offset + 1] == '\n') {
                offset += 2;
            } else {
                ++offset;
            }

            events.emplace_back(KeyEvent{Key::enter, true, KeyModifier::none});
            continue;
        }

        if (byte == static_cast<unsigned char>('\t')) {
            events.emplace_back(KeyEvent{Key::tab, true, KeyModifier::none});
            ++offset;
            continue;
        }

        if (byte == 0x08U || byte == 0x7FU) {
            events.emplace_back(KeyEvent{Key::backspace, true, KeyModifier::none});
            ++offset;
            continue;
        }

        if (byte == static_cast<unsigned char>(' ')) {
            /*
             * Space has both control intent and textual meaning. Emitting both mirrors typical desktop
             * input stacks: Button consumes KeyEvent::space; TextField ignores that key and consumes
             * the following TextInputEvent containing the actual character.
             */
            events.emplace_back(KeyEvent{Key::space, true, KeyModifier::none});
            appendText(events, " ");
            ++offset;
            continue;
        }

        if (byte < 0x20U) {
            // Unsupported C0 controls are consumed rather than leaked into editable text.
            ++offset;
            continue;
        }

        const std::size_t expected = expectedUtf8Length(byte);
        if (!flush && expected > remaining.size()) {
            // The scalar may simply be split across nonblocking device reads.
            break;
        }

        const utf8::DecodedScalar decoded = utf8::decodeOne(pending_, offset);
        if (decoded.consumed == 0) {
            break;
        }

        appendScalarText(events, decoded.value);
        offset += decoded.consumed;
    }

    if (offset != 0) {
        pending_.erase(0, offset);
    }

    /*
     * In flush mode the loop always makes progress: incomplete ESC resolves as Escape and incomplete
     * UTF-8 becomes U+FFFD byte-by-byte. Therefore no pending bytes should remain unless a future
     * parser branch accidentally violates that invariant.
     */
    return events;
}

} // namespace sasd::ui::terminal
