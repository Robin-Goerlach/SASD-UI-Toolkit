#include <sasd/ui/terminal/ansi_input_decoder.hpp>

#include <sasd/ui/text/utf8.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
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

    /*
     * Pointer input shares the CSI transport with keyboard sequences, but keeping it as a separate
     * optional here avoids widening the keyboard parser's local responsibilities into a second Event
     * variant. Exactly one semantic payload is produced by any recognized escape sequence.
     */
    std::optional<PointerEvent> pointer_event;
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
            // Terminal parameters relevant to keyboard sequences are deliberately kept small.
            return std::nullopt;
        }
        result = result * 10U + digit;
    }

    return result;
}

/**
 * Parses an arbitrary unsigned decimal without relying on locale or integer-wrap behavior.
 *
 * SGR mouse coordinates are terminal-cell positions and can legitimately be larger than the small
 * keyboard parameters accepted by parseUnsigned(). They still need an explicit overflow guard because
 * input bytes are untrusted process input and must never wrap into a plausible coordinate.
 */
[[nodiscard]] std::optional<std::uint64_t> parseWideUnsigned(std::string_view text) noexcept {
    if (text.empty()) {
        return std::nullopt;
    }

    std::uint64_t result = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return std::nullopt;
        }

        const auto digit = static_cast<std::uint64_t>(character - '0');
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if (result > (maximum - digit) / 10U) {
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

    /*
     * VT/xterm families commonly encode F1-F4 either as SS3 P..S or as the older CSI 11~..14~
     * family. Supporting both is inexpensive and avoids tying semantic Key identity to one emulator.
     */
    case 11:
        return Key::f1;
    case 12:
        return Key::f2;
    case 13:
        return Key::f3;
    case 14:
        return Key::f4;
    case 15:
        return Key::f5;
    case 17:
        return Key::f6;
    case 18:
        return Key::f7;
    case 19:
        return Key::f8;
    case 20:
        return Key::f9;
    case 21:
        return Key::f10;
    case 23:
        return Key::f11;
    case 24:
        return Key::f12;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] KeyModifier sgrMouseModifiers(unsigned code) noexcept {
    KeyModifier modifiers = KeyModifier::none;

    if ((code & 4U) != 0U) {
        modifiers = modifiers | KeyModifier::shift;
    }
    if ((code & 8U) != 0U) {
        /*
         * xterm calls this the Meta bit. In the terminal input conventions already used by this
         * decoder, Meta/Alt keyboard intent is represented by KeyModifier::alt, so pointer reports use
         * the same portable semantic value instead of exposing an emulator-specific distinction.
         */
        modifiers = modifiers | KeyModifier::alt;
    }
    if ((code & 16U) != 0U) {
        modifiers = modifiers | KeyModifier::control;
    }

    return modifiers;
}

[[nodiscard]] std::optional<PointerButton> sgrMouseButton(unsigned code) noexcept {
    switch (code & 3U) {
    case 0U:
        return PointerButton::primary;
    case 1U:
        return PointerButton::middle;
    case 2U:
        return PointerButton::secondary;
    default:
        return std::nullopt;
    }
}

/**
 * Decodes one complete xterm SGR-1006 mouse report into the backend-neutral PointerEvent model.
 *
 * parameters contains the CSI parameter bytes including the leading '<', while final is either 'M'
 * (press/motion) or 'm' (release). The transport is one-based in terminal cells; SASD UI geometry is
 * zero-based, so conversion happens exactly once at this backend boundary.
 *
 * Wheel reports are deliberately consumed without an event because PointerEvent does not yet model a
 * wheel delta. Extended button encodings above the classic three-button/modifier/motion bit set are
 * likewise left for a future pointer-model extension. Rejecting those reports atomically is safer than
 * pretending that unsupported information is an ordinary button click.
 */
[[nodiscard]] std::optional<PointerEvent> parseSgrMouse(
    std::string_view parameters,
    char final) noexcept {
    if (parameters.empty() || parameters.front() != '<') {
        return std::nullopt;
    }

    const std::string_view body = parameters.substr(1);
    const std::size_t first_separator = body.find(';');
    if (first_separator == std::string_view::npos) {
        return std::nullopt;
    }

    const std::size_t second_separator = body.find(';', first_separator + 1);
    if (second_separator == std::string_view::npos ||
        body.find(';', second_separator + 1) != std::string_view::npos) {
        return std::nullopt;
    }

    const auto raw_code = parseWideUnsigned(body.substr(0, first_separator));
    const auto raw_x = parseWideUnsigned(
        body.substr(first_separator + 1, second_separator - first_separator - 1));
    const auto raw_y = parseWideUnsigned(body.substr(second_separator + 1));

    if (!raw_code || !raw_x || !raw_y ||
        *raw_code > static_cast<std::uint64_t>(std::numeric_limits<unsigned>::max())) {
        return std::nullopt;
    }

    const unsigned code = static_cast<unsigned>(*raw_code);

    /*
     * Classic SGR mouse reports use bits 0..6. Higher bits encode additional buttons/protocol
     * extensions that PointerButton cannot faithfully represent yet. Bit 6 is wheel intent, which is
     * recognized here only so it can be consumed as one complete report without leaking CSI digits as
     * text; no synthetic press event is emitted for it.
     */
    if ((code & ~0x7FU) != 0U || (code & 64U) != 0U) {
        return std::nullopt;
    }

    constexpr std::uint64_t max_one_based_coordinate =
        static_cast<std::uint64_t>(std::numeric_limits<Coordinate>::max()) + 1U;
    if (*raw_x == 0U || *raw_y == 0U ||
        *raw_x > max_one_based_coordinate || *raw_y > max_one_based_coordinate) {
        return std::nullopt;
    }

    PointerEvent event;
    event.position = {
        static_cast<Coordinate>(*raw_x - 1U),
        static_cast<Coordinate>(*raw_y - 1U)};
    event.modifiers = sgrMouseModifiers(code);

    const bool motion = (code & 32U) != 0U;

    if (final == 'm') {
        // SGR 1006 uses a lowercase final specifically for a button release.
        if (motion) {
            return std::nullopt;
        }

        const auto button = sgrMouseButton(code);
        if (!button) {
            return std::nullopt;
        }

        event.action = PointerAction::release;
        event.button = *button;
        event.click_count = 1;
        return event;
    }

    if (final != 'M') {
        return std::nullopt;
    }

    if (motion) {
        /*
         * The low button bits describe which button is held while xterm reports motion. PointerEvent's
         * current contract reserves button=none for movement; PointerRouter already owns the active
         * captured gesture after a press, so duplicating held-button state here would violate that
         * contract without adding useful information.
         */
        event.action = PointerAction::move;
        event.button = PointerButton::none;
        event.click_count = 0;
        return event;
    }

    const auto button = sgrMouseButton(code);
    if (!button) {
        return std::nullopt;
    }

    event.action = PointerAction::press;
    event.button = *button;

    /*
     * SGR reports one transition but carries no native multi-click count. Treat the transition as one
     * ordinary click for now. A later terminal gesture layer may derive double/triple-click counts from
     * timing and position without contaminating this byte decoder with clock state.
     */
    event.click_count = 1;
    return event;
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
            return EscapeResult{0, true, std::nullopt};
        }

        /*
         * Timeout resolution preserves user bytes instead of silently dropping an incomplete CSI:
         * interpret ESC itself now and let the remaining '['/parameters be decoded normally.
         */
        return {1, false, KeyEvent{Key::escape, true, KeyModifier::none}};
    }

    const char final = input[final_index];
    const std::string_view parameters = input.substr(2, final_index - 2);
    const std::size_t consumed = final_index + 1;

    /*
     * SGR mouse reporting is a CSI family too. Recognize its private '<' introducer before generic
     * keyboard parameter parsing; otherwise the leading '<' would merely make primary parsing fail and
     * the report would be discarded as an unknown key sequence.
     */
    if ((final == 'M' || final == 'm') &&
        !parameters.empty() && parameters.front() == '<') {
        return {consumed, false, std::nullopt, parseSgrMouse(parameters, final)};
    }

    const CsiParameters parsed = parseCsiParameters(parameters);

    KeyModifier modifiers = parsed.modifiers;
    std::optional<Key> key;

    switch (final) {
    case 'P':
        /*
         * xterm uses CSI 1;<modifier>P for modified F1. Some terminals also send bare CSI P;
         * accept both empty parameters and primary=1, but do not reinterpret unrelated CSI P forms.
         */
        if (!parsed.primary || *parsed.primary == 1U) {
            key = Key::f1;
        }
        break;
    case 'Q':
        if (!parsed.primary || *parsed.primary == 1U) {
            key = Key::f2;
        }
        break;
    case 'R':
        if (!parsed.primary || *parsed.primary == 1U) {
            key = Key::f3;
        }
        break;
    case 'S':
        if (!parsed.primary || *parsed.primary == 1U) {
            key = Key::f4;
        }
        break;
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
            return EscapeResult{0, true, std::nullopt};
        }
        return {1, false, KeyEvent{Key::escape, true, KeyModifier::none}};
    }

    std::optional<Key> key;
    switch (input[2]) {
    case 'P':
        key = Key::f1;
        break;
    case 'Q':
        key = Key::f2;
        break;
    case 'R':
        key = Key::f3;
        break;
    case 'S':
        key = Key::f4;
        break;
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
            return EscapeResult{0, true, std::nullopt};
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

        /*
         * A CR was already emitted as Enter in the previous chunk. If LF arrives immediately after
         * it, consume only that translation byte. Any other next byte simply ends the suppression.
         */
        if (suppress_next_lf_) {
            suppress_next_lf_ = false;
            if (byte == static_cast<unsigned char>('\n')) {
                ++offset;
                continue;
            }
        }

        if (byte == escape_byte) {
            const EscapeResult parsed = parseEscape(remaining, flush);
            if (parsed.need_more) {
                break;
            }

            if (parsed.event) {
                events.emplace_back(*parsed.event);
            }
            if (parsed.pointer_event) {
                events.emplace_back(*parsed.pointer_event);
            }
            offset += parsed.consumed;
            continue;
        }

        if (byte == static_cast<unsigned char>('\r') ||
            byte == static_cast<unsigned char>('\n')) {
            /*
             * Treat CRLF as one Enter gesture. If LF is already in this buffer consume both bytes;
             * otherwise remember that a leading LF in the next nonblocking read belongs to the CR
             * already emitted now.
             */
            if (byte == static_cast<unsigned char>('\r')) {
                if (offset + 1 < pending_.size() &&
                    pending_[offset + 1] == '\n') {
                    offset += 2;
                } else {
                    ++offset;
                    suppress_next_lf_ = true;
                }
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
