#include <sasd/ui/native/appkit_backend.hpp>

#include "appkit_input_translation.hpp"
#include "appkit_render_context.hpp"

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/rendered/display_list_executor.hpp>
#include <sasd/ui/text/utf8.hpp>

#import <AppKit/AppKit.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace sasd::ui::native::appkit {

struct AppKitWindowBackend::Impl;

@interface SasdAppKitView : NSView <NSTextInputClient>
@property(nonatomic, assign) AppKitWindowBackend::Impl* backend;
@end

@interface SasdAppKitWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) AppKitWindowBackend::Impl* backend;
@end

struct AppKitWindowBackend::Impl {
    explicit Impl(AppKitWindowBackendConfig value) : config{std::move(value)} {
        if (config.initial_size.width <= 0 || config.initial_size.height <= 0 ||
            !std::isfinite(config.font_point_size) || config.font_point_size <= 0.0) {
            throw std::invalid_argument{"AppKit window size/font must be finite and positive"};
        }
        CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
        metric_surface = CGBitmapContextCreate(nullptr, 1, 1, 8, 4, color_space,
                                                kCGImageAlphaPremultipliedLast);
        CGColorSpaceRelease(color_space);
        if (metric_surface == nullptr) throw std::runtime_error{"AppKit metric context creation failed"};
        metric_context = std::make_unique<detail::AppKitRenderContext>(metric_surface,
                                                                         config.font_point_size);
    }

    ~Impl() {
        shutdown();
        metric_context.reset();
        if (metric_surface != nullptr) CGContextRelease(metric_surface);
    }

    void requireMainThread(const char* operation) const {
        if (![NSThread isMainThread]) {
            throw std::runtime_error{std::string{"AppKitWindowBackend::"} + operation +
                                     " must run on the AppKit main thread"};
        }
    }

    void requireInitialized(const char* operation) const {
        requireMainThread(operation);
        if (window == nil || view == nil) {
            throw std::logic_error{std::string{"AppKitWindowBackend::"} + operation +
                                   " requires initialize()"};
        }
    }

    [[nodiscard]] Size currentSize() const {
        const NSRect bounds = [view bounds];
        return {static_cast<Coordinate>(std::max(0.0, bounds.size.width)),
                static_cast<Coordinate>(std::max(0.0, bounds.size.height))};
    }

    void queuePointer(NSEvent* event, PointerAction action, PointerButton button) noexcept {
        @autoreleasepool {
            const NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
            if (const auto translated = detail::translatePoint(point.x, point.y)) {
                events.push(Event{PointerEvent{*translated, action, button,
                    action == PointerAction::move ? 0 : static_cast<std::uint8_t>(event.clickCount),
                    detail::translateModifiers(event.modifierFlags)}});
            }
        }
    }

    void queueKey(NSEvent* event, bool pressed) noexcept {
        @autoreleasepool {
            std::optional<Key> key = detail::translateKey(event.keyCode);
            // AppKit keyCode is physical identity. For the Core A-Z subset, prefer the layout-aware
            // logical character so shortcuts do not silently assume a US keyboard layout.
            NSString* characters = event.charactersIgnoringModifiers;
            if (characters.length == 1) {
                key = detail::translateLogicalLetter([characters characterAtIndex:0]);
            }
            if (key) {
                events.push(Event{KeyEvent{*key, pressed, detail::translateModifiers(event.modifierFlags)}});
            }
        }
    }

    void shutdown() noexcept {
        if (![NSThread isMainThread]) return;
        active_context = nullptr;
        if (view != nil) [view setBackend:nullptr];
        if (delegate != nil) [delegate setBackend:nullptr];
        if (window != nil) {
            [window orderOut:nil];
            [window setDelegate:nil];
            [window close];
        }
        window = nil; view = nil; delegate = nil;
        last_frame.clear(); events.clear(); presentation_requested = false;
        owner_thread = {};
    }

    void initialize() {
        requireMainThread("initialize");
        if (window != nil) throw std::logic_error{"AppKitWindowBackend is already initialized"};
        @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp finishLaunching];
        owner_thread = std::this_thread::get_id();

        const NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
            (config.resizable ? NSWindowStyleMaskResizable : 0);
        window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0,
                    config.initial_size.width, config.initial_size.height)
                    styleMask:style backing:NSBackingStoreBuffered defer:NO];
        view = [[SasdAppKitView alloc] initWithFrame:NSMakeRect(0, 0,
                    config.initial_size.width, config.initial_size.height)];
        delegate = [[SasdAppKitWindowDelegate alloc] init];
        if (window == nil || view == nil || delegate == nil) {
            shutdown(); throw std::runtime_error{"AppKit window/view creation failed"};
        }
        [view setBackend:this]; [delegate setBackend:this];
        [window setTitle:[NSString stringWithUTF8String:config.title.c_str()]];
        [window setContentView:view]; [window setDelegate:delegate];
        [window setReleasedWhenClosed:NO]; [window makeFirstResponder:view];
        [window setAcceptsMouseMovedEvents:YES];
        last_size = currentSize();
        presentation_requested = true;
        if (!config.hidden) [window makeKeyAndOrderFront:nil];
        }
    }

    void draw(CGContextRef context) noexcept {
        @autoreleasepool {
            if (active_context != nullptr) return;
            try {
                detail::AppKitRenderContext render_context{context, config.font_point_size};
                render_context.clear(config.background_color);
                active_context = &render_context;
                (void)rendered::DisplayListExecutor::execute(last_frame, render_context);
                active_context = nullptr; presentation_requested = false;
            } catch (...) { active_context = nullptr; presentation_requested = true; }
        }
    }

    std::optional<Event> pollEvent() {
        requireInitialized("pollEvent");
        if (auto event = events.tryPop()) return event;
        @autoreleasepool {
            for (int count = 0; count != 32; ++count) {
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                    untilDate:[NSDate dateWithTimeIntervalSinceNow:0]
                    inMode:NSDefaultRunLoopMode dequeue:YES];
                if (event == nil) break;
                [NSApp sendEvent:event];
                if (auto queued = events.tryPop()) return queued;
            }
            [NSApp updateWindows];
            const Size size = currentSize();
            if (size != last_size) { last_size = size; presentation_requested = true; return Event{ResizeEvent{size}}; }
        }
        return std::nullopt;
    }

    AppKitWindowBackendConfig config;
    NSWindow* __strong window{nil};
    SasdAppKitView* __strong view{nil};
    SasdAppKitWindowDelegate* __strong delegate{nil};
    Size last_size{};
    std::thread::id owner_thread{};
    EventQueue events;
    rendered::DisplayList last_frame;
    bool presentation_requested{false};
    detail::AppKitRenderContext* active_context{nullptr};
    CGContextRef metric_surface{nullptr};
    std::unique_ptr<detail::AppKitRenderContext> metric_context;
};

@implementation SasdAppKitView
@synthesize backend = _backend;
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (void)drawRect:(NSRect)dirtyRect { (void)dirtyRect; if (_backend) _backend->draw([[NSGraphicsContext currentContext] CGContext]); }
- (void)viewDidChangeBackingProperties { [super viewDidChangeBackingProperties]; [self setNeedsDisplay:YES]; }
- (void)updateTrackingAreas { [super updateTrackingAreas]; for (NSTrackingArea* area in [self trackingAreas]) [self removeTrackingArea:area];
    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect options:NSTrackingMouseEnteredAndExited|NSTrackingMouseMoved|NSTrackingActiveInKeyWindow|NSTrackingInVisibleRect owner:self userInfo:nil]]; }
- (void)mouseEntered:(NSEvent*)event { (void)event; if (_backend) _backend->events.push(Event{PointerSurfaceEvent{PointerSurfaceAction::entered}}); }
- (void)mouseExited:(NSEvent*)event { (void)event; if (_backend) _backend->events.push(Event{PointerSurfaceEvent{PointerSurfaceAction::left}}); }
- (void)mouseMoved:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::move, PointerButton::none); }
- (void)mouseDown:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::press, PointerButton::primary); }
- (void)mouseUp:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::release, PointerButton::primary); }
- (void)rightMouseDown:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::press, PointerButton::secondary); }
- (void)rightMouseUp:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::release, PointerButton::secondary); }
- (void)otherMouseDown:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::press, PointerButton::other); }
- (void)otherMouseUp:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::release, PointerButton::other); }
- (void)mouseDragged:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::move, PointerButton::none); }
- (void)rightMouseDragged:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::move, PointerButton::none); }
- (void)otherMouseDragged:(NSEvent*)event { if (_backend) _backend->queuePointer(event, PointerAction::move, PointerButton::none); }
- (void)keyDown:(NSEvent*)event { if (_backend) _backend->queueKey(event, true); [self interpretKeyEvents:@[event]]; }
- (void)keyUp:(NSEvent*)event { if (_backend) _backend->queueKey(event, false); }
- (void)insertText:(id)string replacementRange:(NSRange)range { (void)range; if (!_backend) return; NSString* value = [string isKindOfClass:[NSAttributedString class]] ? [string string] : string; const std::string utf8 = [value UTF8String] ? [value UTF8String] : ""; if (!utf8.empty()) _backend->events.push(Event{TextInputEvent{::sasd::ui::utf8::sanitizeSingleLine(utf8)}}); }
- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange { (void)string; (void)selectedRange; (void)replacementRange; }
- (void)unmarkText {}
- (NSRange)selectedRange { return NSMakeRange(NSNotFound, 0); }
- (NSRange)markedRange { return NSMakeRange(NSNotFound, 0); }
- (BOOL)hasMarkedText { return NO; }
- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange { if (actualRange) *actualRange = range; return nil; }
- (NSUInteger)characterIndexForPoint:(NSPoint)point { (void)point; return 0; }
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange { if (actualRange) *actualRange = range; return NSZeroRect; }
- (void)doCommandBySelector:(SEL)selector { (void)selector; }
- (NSArray<NSString*>*)validAttributesForMarkedText { return @[]; }
@end

@implementation SasdAppKitWindowDelegate
@synthesize backend = _backend;
- (BOOL)windowShouldClose:(id)sender { (void)sender; if (_backend) _backend->events.push(Event{QuitEvent{}}); return NO; }
@end

AppKitWindowBackend::AppKitWindowBackend(AppKitWindowBackendConfig config)
    : impl_{std::make_unique<Impl>(std::move(config))} {}
AppKitWindowBackend::~AppKitWindowBackend() { shutdown(); }
BackendCapabilities AppKitWindowBackend::capabilities() const noexcept { BackendCapabilities result; result.pointer_input = true; return result; }
void AppKitWindowBackend::initialize() { impl_->initialize(); }
void AppKitWindowBackend::shutdown() noexcept { impl_->shutdown(); }
std::optional<Event> AppKitWindowBackend::pollEvent() { return impl_->pollEvent(); }
bool AppKitWindowBackend::isInitialized() const noexcept { return impl_->window != nil; }
Size AppKitWindowBackend::windowSize() const { impl_->requireInitialized("windowSize"); return impl_->last_size; }
bool AppKitWindowBackend::presentationRequested() const noexcept { return impl_->presentation_requested; }
std::size_t AppKitWindowBackend::presentFrame(const rendered::DisplayList& list) {
    impl_->requireInitialized("presentFrame"); impl_->last_frame = list; impl_->presentation_requested = true; [impl_->view setNeedsDisplay:YES]; return list.size();
}
void AppKitWindowBackend::fillRect(const rendered::FillRectCommand& command) { impl_->requireInitialized("fillRect"); if (!impl_->active_context) throw std::logic_error{"AppKit fillRect outside draw callback"}; impl_->active_context->fillRect(command); }
void AppKitWindowBackend::strokeRect(const rendered::StrokeRectCommand& command) { impl_->requireInitialized("strokeRect"); if (!impl_->active_context) throw std::logic_error{"AppKit strokeRect outside draw callback"}; impl_->active_context->strokeRect(command); }
void AppKitWindowBackend::drawText(const rendered::DrawTextCommand& command) { impl_->requireInitialized("drawText"); if (!impl_->active_context) throw std::logic_error{"AppKit drawText outside draw callback"}; impl_->active_context->drawText(command); }
Size AppKitWindowBackend::measureText(std::string_view text) const { return impl_->metric_context->measureText(text); }
Coordinate AppKitWindowBackend::lineHeight() const noexcept { return impl_->metric_context->lineHeight(); }
std::optional<Coordinate> AppKitWindowBackend::textAdvanceToScalar(std::string_view text, std::size_t scalar_index) const { return impl_->metric_context->textAdvanceToScalar(text, scalar_index); }
std::uint64_t AppKitWindowBackend::revision() const noexcept { return 1; }

} // namespace sasd::ui::native::appkit
