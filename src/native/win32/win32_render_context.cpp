#include "win32_render_context.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace sasd::ui::native::win32::detail {
namespace {

[[nodiscard]] std::wstring toWide(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (std::size_t offset = 0; offset < text.size();) {
        const auto decoded = utf8::decodeOne(text, offset);
        if (decoded.consumed == 0) break;
        if (decoded.valid) {
            normalized.append(text.substr(offset, decoded.consumed));
        } else {
            utf8::appendScalar(normalized, U'\uFFFD');
        }
        offset += decoded.consumed;
    }
    if (normalized.empty()) return {};
    if (normalized.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error{"Win32 UTF-8 text is too long"};
    }
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, normalized.data(),
                                          static_cast<int>(normalized.size()), nullptr, 0);
    if (count <= 0) throw std::runtime_error{"MultiByteToWideChar failed"};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, normalized.data(),
                            static_cast<int>(normalized.size()), result.data(), count) != count) {
        throw std::runtime_error{"MultiByteToWideChar failed"};
    }
    return result;
}

[[nodiscard]] int scaled(std::int64_t value, UINT dpi, const char* what) {
    const auto result = (value * static_cast<std::int64_t>(dpi)) / 96;
    if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max()) {
        throw std::overflow_error{std::string{"Win32 physical "} + what + " is unrepresentable"};
    }
    return static_cast<int>(result);
}

[[nodiscard]] COLORREF palette(Color value, bool background) noexcept {
    switch (value) {
    case Color::default_color: return background ? RGB(0, 0, 0) : RGB(224, 224, 224);
    case Color::black: return RGB(0, 0, 0);
    case Color::red: return RGB(170, 0, 0);
    case Color::green: return RGB(0, 170, 0);
    case Color::yellow: return RGB(170, 85, 0);
    case Color::blue: return RGB(0, 0, 170);
    case Color::magenta: return RGB(170, 0, 170);
    case Color::cyan: return RGB(0, 170, 170);
    case Color::white: return RGB(170, 170, 170);
    case Color::bright_black: return RGB(85, 85, 85);
    case Color::bright_red: return RGB(255, 85, 85);
    case Color::bright_green: return RGB(85, 255, 85);
    case Color::bright_yellow: return RGB(255, 255, 85);
    case Color::bright_blue: return RGB(85, 85, 255);
    case Color::bright_magenta: return RGB(255, 85, 255);
    case Color::bright_cyan: return RGB(85, 255, 255);
    case Color::bright_white: return RGB(255, 255, 255);
    }
    return background ? RGB(0, 0, 0) : RGB(224, 224, 224);
}

class FontScope final {
public:
    FontScope(HDC dc, HFONT font) noexcept : dc_{dc}, previous_{SelectObject(dc, font)} {}
    ~FontScope() { SelectObject(dc_, previous_); }
    FontScope(const FontScope&) = delete;
    FontScope& operator=(const FontScope&) = delete;
private:
    HDC dc_;
    HGDIOBJ previous_;
};

} // namespace

Win32RenderContext::Win32RenderContext(HDC device_context, HFONT font, UINT dpi) noexcept
    : device_context_{device_context}, font_{font}, dpi_{dpi == 0 ? 96U : dpi}, line_height_{1} {
    FontScope scope{device_context_, font_};
    TEXTMETRICW metrics{};
    if (GetTextMetricsW(device_context_, &metrics)) {
        line_height_ = std::max<Coordinate>(1, (static_cast<std::int64_t>(metrics.tmHeight) * 96) /
                                                    static_cast<std::int64_t>(dpi_));
    }
}

HFONT Win32RenderContext::selectFont() const noexcept { return font_; }

RECT Win32RenderContext::toPhysical(Rect value) const {
    const int left = scaled(value.x, dpi_, "coordinate");
    const int top = scaled(value.y, dpi_, "coordinate");
    return {left, top, left + scaled(value.width, dpi_, "extent"),
            top + scaled(value.height, dpi_, "extent")};
}

POINT Win32RenderContext::toPhysical(Point value) const {
    return {scaled(value.x, dpi_, "coordinate"), scaled(value.y, dpi_, "coordinate")};
}

COLORREF Win32RenderContext::color(Color value, bool background) const noexcept {
    return palette(value, background);
}

void Win32RenderContext::clear(Color value) {
    RECT client{};
    GetClientRect(WindowFromDC(device_context_), &client);
    const HBRUSH brush = CreateSolidBrush(color(value, true));
    if (brush == nullptr) throw std::runtime_error{"CreateSolidBrush failed"};
    FillRect(device_context_, &client, brush);
    DeleteObject(brush);
}

void Win32RenderContext::fillRect(const rendered::FillRectCommand& command) {
    const RECT rect = toPhysical(command.bounds);
    const HBRUSH brush = CreateSolidBrush(color(command.color, command.role == rendered::FillRole::background));
    if (brush == nullptr) throw std::runtime_error{"CreateSolidBrush failed"};
    FillRect(device_context_, &rect, brush);
    DeleteObject(brush);
}

void Win32RenderContext::strokeRect(const rendered::StrokeRectCommand& command) {
    const RECT rect = toPhysical(command.bounds);
    const HPEN pen = CreatePen(PS_SOLID, std::max(1, scaled(command.thickness, dpi_, "stroke")),
                               color(command.color, false));
    if (pen == nullptr) throw std::runtime_error{"CreatePen failed"};
    FontScope scope{device_context_, font_};
    const HGDIOBJ old_pen = SelectObject(device_context_, pen);
    const HGDIOBJ old_brush = SelectObject(device_context_, GetStockObject(NULL_BRUSH));
    Rectangle(device_context_, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(device_context_, old_brush);
    SelectObject(device_context_, old_pen);
    DeleteObject(pen);
}

void Win32RenderContext::drawText(const rendered::DrawTextCommand& command) {
    const std::wstring text = toWide(command.text);
    if (text.empty()) return;
    FontScope scope{device_context_, selectFont()};
    const int saved = SaveDC(device_context_);
    if (saved == 0) throw std::runtime_error{"SaveDC failed"};
    if (command.clip_bounds.has_value()) {
        const RECT clip = toPhysical(*command.clip_bounds);
        IntersectClipRect(device_context_, clip.left, clip.top, clip.right, clip.bottom);
    }
    SetTextColor(device_context_, color(command.style.foreground, false));
    SetBkMode(device_context_, TRANSPARENT);
    const POINT origin = toPhysical(command.origin);
    std::size_t start = 0;
    std::size_t line_index = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(L'\n', start);
        const std::size_t length = end == std::wstring::npos ? text.size() - start : end - start;
        const int y = origin.y + scaled(static_cast<std::int64_t>(line_index) * line_height_, dpi_, "line");
        if (length > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            RestoreDC(device_context_, saved);
            throw std::length_error{"Win32 text line is too long"};
        }
        TextOutW(device_context_, origin.x, y, text.data() + start, static_cast<int>(length));
        if (end == std::wstring::npos) break;
        start = end + 1;
        ++line_index;
    }
    RestoreDC(device_context_, saved);
}

Size Win32RenderContext::measureText(std::string_view utf8_text) const {
    const std::wstring text = toWide(utf8_text);
    FontScope scope{device_context_, font_};
    Coordinate width = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(L'\n', start);
        const std::size_t length = end == std::wstring::npos ? text.size() - start : end - start;
        SIZE measured{};
        if (length > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error{"Win32 text line is too long"};
        }
        if (!GetTextExtentPoint32W(device_context_, text.data() + start, static_cast<int>(length), &measured)) {
            throw std::runtime_error{"GetTextExtentPoint32W failed"};
        }
        width = std::max(width, static_cast<Coordinate>((static_cast<std::int64_t>(measured.cx) * 96) / dpi_));
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    const auto lines = static_cast<std::int64_t>(std::count(utf8_text.begin(), utf8_text.end(), '\n')) + 1;
    const auto height = std::min<std::int64_t>(std::numeric_limits<Coordinate>::max(),
                                               lines * line_height_);
    return {width, static_cast<Coordinate>(height)};
}

std::optional<Coordinate> Win32RenderContext::textAdvanceToScalar(std::string_view text,
                                                                    std::size_t scalar_index) const {
    if (text.find('\n') != std::string_view::npos) return std::nullopt;
    const std::size_t count = utf8::scalarCount(text);
    if (scalar_index > count) return std::nullopt;
    const std::string prefix = text.substr(0, utf8::byteOffsetForScalarIndex(text, scalar_index));
    const std::wstring wide = toWide(prefix);
    FontScope scope{device_context_, font_};
    SIZE measured{};
    if (wide.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error{"Win32 text prefix is too long"};
    }
    if (!GetTextExtentPoint32W(device_context_, wide.data(), static_cast<int>(wide.size()), &measured)) {
        throw std::runtime_error{"GetTextExtentPoint32W failed"};
    }
    return static_cast<Coordinate>((static_cast<std::int64_t>(measured.cx) * 96) / dpi_);
}

} // namespace sasd::ui::native::win32::detail
