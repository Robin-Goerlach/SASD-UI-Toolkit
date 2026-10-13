#include "appkit_render_context.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace sasd::ui::native::appkit::detail {
namespace {

struct Rgb { CGFloat r; CGFloat g; CGFloat b; };

Rgb palette(Color color, bool background) noexcept {
    switch (color) {
    case Color::default_color: return background ? Rgb{0, 0, 0} : Rgb{0.88, 0.88, 0.88};
    case Color::black: return {0, 0, 0}; case Color::red: return {0.67, 0, 0};
    case Color::green: return {0, 0.67, 0}; case Color::yellow: return {0.67, 0.33, 0};
    case Color::blue: return {0, 0, 0.67}; case Color::magenta: return {0.67, 0, 0.67};
    case Color::cyan: return {0, 0.67, 0.67}; case Color::white: return {0.67, 0.67, 0.67};
    case Color::bright_black: return {0.33, 0.33, 0.33};
    case Color::bright_red: return {1, 0.33, 0.33}; case Color::bright_green: return {0.33, 1, 0.33};
    case Color::bright_yellow: return {1, 1, 0.33}; case Color::bright_blue: return {0.33, 0.33, 1};
    case Color::bright_magenta: return {1, 0.33, 1}; case Color::bright_cyan: return {0.33, 1, 1};
    case Color::bright_white: return {1, 1, 1};
    }
    return background ? Rgb{0, 0, 0} : Rgb{0.88, 0.88, 0.88};
}

void setColor(CGContextRef context, Color color, bool background = false) noexcept {
    const Rgb value = palette(color, background);
    CGContextSetRGBFillColor(context, value.r, value.g, value.b, 1.0);
    CGContextSetRGBStrokeColor(context, value.r, value.g, value.b, 1.0);
}

CFStringRef makeString(std::string_view text) {
    const std::string sanitized = utf8::sanitizeSingleLine(text);
    return CFStringCreateWithBytes(kCFAllocatorDefault,
                                   reinterpret_cast<const UInt8*>(sanitized.data()),
                                   static_cast<CFIndex>(sanitized.size()), kCFStringEncodingUTF8,
                                   false);
}

CFAttributedStringRef makeAttributedString(std::string_view text, CTFontRef font, CGColorRef color) {
    CFStringRef value = makeString(text);
    if (value == nullptr) throw std::invalid_argument{"AppKit text is not valid UTF-8"};
    CGColorRef fallback = color;
    if (fallback == nullptr) fallback = CGColorCreateGenericRGB(0.88, 0.88, 0.88, 1.0);
    const void* keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
    const void* values[] = {font, fallback};
    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 2,
                                                     &kCFTypeDictionaryKeyCallBacks,
                                                     &kCFTypeDictionaryValueCallBacks);
    CFAttributedStringRef result = CFAttributedStringCreate(kCFAllocatorDefault, value, attributes);
    CFRelease(attributes);
    CFRelease(value);
    if (color == nullptr) CGColorRelease(fallback);
    if (result == nullptr) throw std::runtime_error{"CoreText attributed string creation failed"};
    return result;
}

[[nodiscard]] Coordinate checkedCoordinate(double value, const char* what) {
    if (!std::isfinite(value) || value < 0.0 ||
        value > static_cast<double>(std::numeric_limits<Coordinate>::max())) {
        throw std::overflow_error{std::string{what} + " is outside SASD Coordinate range"};
    }
    return static_cast<Coordinate>(std::ceil(value));
}

} // namespace

AppKitRenderContext::AppKitRenderContext(CGContextRef context, CGFloat font_size)
    : context_{context}, font_{CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, font_size, nullptr)} {
    if (context_ == nullptr || font_ == nullptr) throw std::runtime_error{"CoreText font creation failed"};
    const double height = CTFontGetAscent(font_) + CTFontGetDescent(font_) + CTFontGetLeading(font_);
    line_height_ = std::max<Coordinate>(1, checkedCoordinate(height, "AppKit line height"));
}

AppKitRenderContext::~AppKitRenderContext() { if (font_ != nullptr) CFRelease(font_); }

void AppKitRenderContext::clear(Color color) {
    CGContextSaveGState(context_);
    setColor(context_, color, true);
    CGContextFillRect(context_, CGContextGetClipBoundingBox(context_));
    CGContextRestoreGState(context_);
}

void AppKitRenderContext::fillRect(const rendered::FillRectCommand& command) {
    CGContextSaveGState(context_);
    setColor(context_, command.color, command.role == rendered::FillRole::background);
    CGContextFillRect(context_, CGRectMake(command.bounds.x, command.bounds.y,
                                           command.bounds.width, command.bounds.height));
    CGContextRestoreGState(context_);
}

void AppKitRenderContext::strokeRect(const rendered::StrokeRectCommand& command) {
    CGContextSaveGState(context_);
    setColor(context_, command.color);
    CGContextSetLineWidth(context_, std::max<Coordinate>(1, command.thickness));
    CGContextStrokeRect(context_, CGRectMake(command.bounds.x, command.bounds.y,
                                             command.bounds.width, command.bounds.height));
    CGContextRestoreGState(context_);
}

void AppKitRenderContext::drawText(const rendered::DrawTextCommand& command) {
    if (command.text.empty()) return;
    CGContextSaveGState(context_);
    if (command.clip_bounds) CGContextClipToRect(context_, CGRectMake(command.clip_bounds->x,
        command.clip_bounds->y, command.clip_bounds->width, command.clip_bounds->height));
    const Rgb value = palette(command.style.foreground, false);
    CGColorRef color = CGColorCreateGenericRGB(value.r, value.g, value.b, 1.0);
    CFAttributedStringRef attributed = makeAttributedString(command.text, font_, color);
    CTLineRef line = CTLineCreateWithAttributedString(attributed);
    CGContextSetTextPosition(context_, command.origin.x,
                             command.origin.y + CTFontGetAscent(font_));
    CTLineDraw(line, context_);
    CFRelease(line); CFRelease(attributed); CGColorRelease(color);
    CGContextRestoreGState(context_);
}

Size AppKitRenderContext::measureText(std::string_view text) const {
    CFAttributedStringRef attributed = makeAttributedString(text, font_, nullptr);
    CTLineRef line = CTLineCreateWithAttributedString(attributed);
    CGFloat ascent = 0; CGFloat descent = 0; CGFloat leading = 0;
    const double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
    CFRelease(line); CFRelease(attributed);
    return {checkedCoordinate(width, "AppKit text width"), line_height_};
}

std::optional<Coordinate> AppKitRenderContext::textAdvanceToScalar(std::string_view text,
                                                                    std::size_t scalar_index) const {
    if (text.find('\n') != std::string_view::npos || scalar_index > utf8::scalarCount(text)) return std::nullopt;
    const std::size_t byte_offset = utf8::byteOffsetForScalarIndex(text, scalar_index);
    const std::string prefix = utf8::sanitizeSingleLine(text.substr(0, byte_offset));
    const std::string full = utf8::sanitizeSingleLine(text);
    CFAttributedStringRef attributed = makeAttributedString(full, font_, nullptr);
    CTLineRef line = CTLineCreateWithAttributedString(attributed);
    CFStringRef prefix_string = makeString(prefix);
    const CFIndex index = prefix_string == nullptr ? 0 : CFStringGetLength(prefix_string);
    const double offset = CTLineGetOffsetForStringIndex(line, index, nullptr);
    if (prefix_string) CFRelease(prefix_string);
    CFRelease(line); CFRelease(attributed);
    return checkedCoordinate(offset, "AppKit text advance");
}

} // namespace sasd::ui::native::appkit::detail
