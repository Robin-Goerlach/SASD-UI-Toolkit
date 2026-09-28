#include "rendered/sdl3/sdl3_window_backend.hpp"

#include "rendered/sdl3/sdl3_render_context.hpp"
#include "rendered/sdl3/sdl3_pointer_event_translation.hpp"

#include <sasd/ui/rendered/display_list_executor.hpp>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace sasd::ui::rendered::sdl3 {
namespace {

[[noreturn]] void throwSdlError(const char* operation) {
    throw std::runtime_error{
        std::string{operation} + " failed: " +
        (SDL_GetError() != nullptr ? SDL_GetError() : "unknown SDL error")};
}

[[nodiscard]] int checkedPositiveInt(Coordinate value, const char* what) {
    if (value <= 0 ||
        static_cast<std::int64_t>(value) >
            static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument{
            std::string{what} + " must be positive and inside SDL integer range"};
    }
    return static_cast<int>(value);
}

[[nodiscard]] Coordinate checkedCoordinate(int value, const char* what) {
    if (value < 0 ||
        static_cast<std::int64_t>(value) >
            static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
        throw std::runtime_error{std::string{what} + " is outside SASD Coordinate range"};
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] KeyModifier translateModifiers(SDL_Keymod modifiers) noexcept {
    KeyModifier result = KeyModifier::none;

    if ((modifiers & SDL_KMOD_SHIFT) != 0U) {
        result = result | KeyModifier::shift;
    }
    if ((modifiers & SDL_KMOD_CTRL) != 0U) {
        result = result | KeyModifier::control;
    }
    if ((modifiers & SDL_KMOD_ALT) != 0U) {
        result = result | KeyModifier::alt;
    }
    if ((modifiers & SDL_KMOD_GUI) != 0U) {
        result = result | KeyModifier::meta;
    }

    return result;
}

[[nodiscard]] std::optional<Key> translateKey(SDL_Keycode key) noexcept {
    switch (key) {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return Key::enter;
    case SDLK_ESCAPE:    return Key::escape;
    case SDLK_TAB:       return Key::tab;
    case SDLK_BACKSPACE: return Key::backspace;
    case SDLK_DELETE:    return Key::delete_forward;
    case SDLK_SPACE:     return Key::space;
    case SDLK_LEFT:      return Key::left;
    case SDLK_RIGHT:     return Key::right;
    case SDLK_UP:        return Key::up;
    case SDLK_DOWN:      return Key::down;
    case SDLK_HOME:      return Key::home;
    case SDLK_END:       return Key::end;
    case SDLK_PAGEUP:    return Key::page_up;
    case SDLK_PAGEDOWN:  return Key::page_down;
    case SDLK_F1:        return Key::f1;
    case SDLK_F2:        return Key::f2;
    case SDLK_F3:        return Key::f3;
    case SDLK_F4:        return Key::f4;
    case SDLK_F5:        return Key::f5;
    case SDLK_F6:        return Key::f6;
    case SDLK_F7:        return Key::f7;
    case SDLK_F8:        return Key::f8;
    case SDLK_F9:        return Key::f9;
    case SDLK_F10:       return Key::f10;
    case SDLK_F11:       return Key::f11;
    case SDLK_F12:       return Key::f12;
    default:
        /*
         * The semantic Key enum intentionally does not contain printable alphabetic/numeric keys yet.
         * Those arrive through SDL_TEXT_INPUT when text entry is enabled. Do not manufacture
         * Key::unknown events for every unrelated SDL key because that would add noisy duplicate
         * input to widget routing.
         */
        return std::nullopt;
    }
}

} // namespace

struct Sdl3WindowBackend::Impl {
    explicit Impl(Sdl3WindowBackendConfig value)
        : config{std::move(value)} {
        if (config.initial_size.width <= 0 || config.initial_size.height <= 0) {
            throw std::invalid_argument{"SDL3 window initial size must be positive"};
        }
        if (config.font_path.empty()) {
            throw std::invalid_argument{"SDL3 window backend requires a font file path"};
        }
        if (!(config.font_point_size > 0.0F) || !std::isfinite(config.font_point_size)) {
            throw std::invalid_argument{"SDL3 window font point size must be finite and positive"};
        }
    }

    ~Impl() {
        shutdown();
    }

    [[nodiscard]] bool initialized() const noexcept {
        return window != nullptr;
    }

    void requireInitialized(const char* operation) const {
        if (!initialized()) {
            throw std::logic_error{
                std::string{"Sdl3WindowBackend::"} + operation + " requires initialize()"};
        }
        requireOwnerThread();
    }

    void requireOwnerThread() const {
        if (owner_thread != std::thread::id{} &&
            std::this_thread::get_id() != owner_thread) {
            throw std::runtime_error{
                "SDL3 window backend must be used from the thread that initialized it"};
        }
    }

    void initialize() {
        if (initialized()) {
            throw std::logic_error{"Sdl3WindowBackend is already initialized"};
        }
        if (!SDL_IsMainThread()) {
            throw std::runtime_error{
                "SDL3 window backend must be initialized on the process main thread"};
        }

        owner_thread = std::this_thread::get_id();

        /*
         * SDL video and SDL_ttf are process-level subsystems while Window/Renderer/Font are ordinary
         * owned resources. The try/catch boundary is deliberately outside the local smart pointers:
         * C++ first destroys font -> renderer -> window during stack unwinding, then the catch block
         * releases TTF/video. This preserves native dependency order on every failure path.
         */
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            owner_thread = {};
            throwSdlError("SDL_InitSubSystem(SDL_INIT_VIDEO)");
        }

        bool local_ttf_initialized = false;

        try {
            if (!TTF_Init()) {
                throwSdlError("TTF_Init");
            }
            local_ttf_initialized = true;

            using WindowPtr =
                std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
            using RendererPtr =
                std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
            using FontPtr =
                std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)>;

            SDL_WindowFlags flags = 0;
            if (config.hidden) {
                flags |= SDL_WINDOW_HIDDEN;
            }
            if (config.resizable) {
                flags |= SDL_WINDOW_RESIZABLE;
            }
            if (config.high_pixel_density) {
                flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
            }

            WindowPtr local_window{
                SDL_CreateWindow(
                    config.title.c_str(),
                    checkedPositiveInt(config.initial_size.width, "window width"),
                    checkedPositiveInt(config.initial_size.height, "window height"),
                    flags),
                &SDL_DestroyWindow};
            if (!local_window) {
                throwSdlError("SDL_CreateWindow");
            }

            RendererPtr local_renderer{
                SDL_CreateRenderer(local_window.get(), nullptr),
                &SDL_DestroyRenderer};
            if (!local_renderer) {
                throwSdlError("SDL_CreateRenderer");
            }

            FontPtr local_font{
                TTF_OpenFont(config.font_path.c_str(), config.font_point_size),
                &TTF_CloseFont};
            if (!local_font) {
                throwSdlError("TTF_OpenFont");
            }

            int logical_width = 0;
            int logical_height = 0;
            if (!SDL_GetWindowSize(local_window.get(), &logical_width, &logical_height)) {
                throwSdlError("SDL_GetWindowSize");
            }

            const Size local_size{
                checkedCoordinate(logical_width, "window width"),
                checkedCoordinate(logical_height, "window height")};

            if (!local_size.isEmpty() &&
                !SDL_SetRenderLogicalPresentation(
                    local_renderer.get(),
                    logical_width,
                    logical_height,
                    SDL_LOGICAL_PRESENTATION_STRETCH)) {
                throwSdlError("SDL_SetRenderLogicalPresentation");
            }

            const SDL_WindowID local_window_id = SDL_GetWindowID(local_window.get());
            if (local_window_id == 0) {
                throwSdlError("SDL_GetWindowID");
            }

            auto local_context =
                std::make_unique<detail::Sdl3RenderContext>(*local_renderer, *local_font);

            /*
             * Commit ownership last. From this point shutdown() is the single release path and uses
             * reverse dependency order: context -> font -> renderer -> window -> TTF -> SDL video.
             */
            video_initialized = true;
            ttf_initialized = true;
            window = local_window.release();
            renderer = local_renderer.release();
            font = local_font.release();
            render_context = std::move(local_context);
            logical_size = local_size;
            window_id = local_window_id;
            presentation_requested = true;
            text_input_enabled = false;
            ++revision;
        } catch (...) {
            if (local_ttf_initialized) {
                TTF_Quit();
            }
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            owner_thread = {};
            throw;
        }
    }

    void shutdown() noexcept {
        if (owner_thread != std::thread::id{} &&
            std::this_thread::get_id() != owner_thread) {
            /*
             * SDL window/renderer destruction is main-thread-affine. We cannot throw from shutdown(),
             * and destroying them from the wrong thread would be worse. Application normally owns
             * backend lifetime on one thread, so this branch is a defensive misuse guard.
             */
            return;
        }

        if (window != nullptr && text_input_enabled) {
            (void)SDL_StopTextInput(window);
            text_input_enabled = false;
        }

        render_context.reset();

        if (font != nullptr) {
            TTF_CloseFont(font);
            font = nullptr;
        }
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        if (ttf_initialized) {
            TTF_Quit();
            ttf_initialized = false;
        }
        if (video_initialized) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            video_initialized = false;
        }

        logical_size = {};
        window_id = 0;
        presentation_requested = false;
        owner_thread = {};
    }

    [[nodiscard]] bool belongsToWindow(SDL_WindowID event_window_id) const noexcept {
        /*
         * SDL uses zero when an event has no specific window/virtual source. Accepting zero is useful
         * for global/virtual keyboard events and deterministic pushed-event tests while still
         * rejecting events that explicitly name another SDL window.
         */
        return event_window_id == 0 || event_window_id == window_id;
    }

    void configureLogicalPresentation(Size size) {
        if (size.isEmpty()) {
            return;
        }

        if (!SDL_SetRenderLogicalPresentation(
                renderer,
                checkedPositiveInt(size.width, "logical width"),
                checkedPositiveInt(size.height, "logical height"),
                SDL_LOGICAL_PRESENTATION_STRETCH)) {
            throwSdlError("SDL_SetRenderLogicalPresentation");
        }
    }

    [[nodiscard]] std::optional<Event> pollEvent() {
        requireInitialized("pollEvent");

        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                return Event{QuitEvent{}};

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (belongsToWindow(event.window.windowID)) {
                    return Event{QuitEvent{}};
                }
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                if (belongsToWindow(event.window.windowID)) {
                    if (event.window.data1 < 0 || event.window.data2 < 0) {
                        // Ignore malformed native extents instead of injecting invalid layout state.
                        break;
                    }

                    logical_size = {
                        checkedCoordinate(event.window.data1, "resized window width"),
                        checkedCoordinate(event.window.data2, "resized window height")};
                    configureLogicalPresentation(logical_size);
                    presentation_requested = true;
                    return Event{ResizeEvent{logical_size}};
                }
                break;

            case SDL_EVENT_WINDOW_EXPOSED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                if (belongsToWindow(event.window.windowID)) {
                    /*
                     * These events need a fresh frame but do not necessarily change semantic logical
                     * size. SDL's logical presentation tracks output pixels; replaying the current
                     * logical frame is sufficient for this first high-DPI foundation.
                     */
                    presentation_requested = true;
                }
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                if (belongsToWindow(event.key.windowID)) {
                    if (const auto key = translateKey(event.key.key)) {
                        return Event{KeyEvent{
                            *key,
                            event.type == SDL_EVENT_KEY_DOWN,
                            translateModifiers(event.key.mod)}};
                    }
                }
                break;

            case SDL_EVENT_TEXT_INPUT:
                if (belongsToWindow(event.text.windowID) &&
                    event.text.text != nullptr &&
                    event.text.text[0] != '\0') {
                    return Event{TextInputEvent{std::string{event.text.text}}};
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (belongsToWindow(event.motion.windowID)) {
                    float logical_x = 0.0F;
                    float logical_y = 0.0F;
                    if (!SDL_RenderCoordinatesFromWindow(
                            renderer,
                            event.motion.x,
                            event.motion.y,
                            &logical_x,
                            &logical_y)) {
                        throwSdlError("SDL_RenderCoordinatesFromWindow(mouse motion)");
                    }

                    SDL_Event logical_event = event;
                    logical_event.motion.x = logical_x;
                    logical_event.motion.y = logical_y;

                    if (const auto pointer =
                            detail::translateLogicalPointerEvent(logical_event)) {
                        return Event{*pointer};
                    }
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (belongsToWindow(event.button.windowID)) {
                    float logical_x = 0.0F;
                    float logical_y = 0.0F;
                    if (!SDL_RenderCoordinatesFromWindow(
                            renderer,
                            event.button.x,
                            event.button.y,
                            &logical_x,
                            &logical_y)) {
                        throwSdlError("SDL_RenderCoordinatesFromWindow(mouse button)");
                    }

                    SDL_Event logical_event = event;
                    logical_event.button.x = logical_x;
                    logical_event.button.y = logical_y;

                    if (const auto pointer =
                            detail::translateLogicalPointerEvent(logical_event)) {
                        return Event{*pointer};
                    }
                }
                break;

            default:
                /*
                 * Window focus, wheel/touch/pen, composition and other SDL events are intentionally
                 * not converted yet. In particular SDL window focus is not Widget FocusEvent:
                 * logical widget focus remains owned by FocusManager.
                 */
                break;
            }
        }

        return std::nullopt;
    }

    Sdl3WindowBackendConfig config;
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};
    TTF_Font* font{nullptr};
    std::unique_ptr<detail::Sdl3RenderContext> render_context;

    bool video_initialized{false};
    bool ttf_initialized{false};
    bool text_input_enabled{false};
    bool presentation_requested{false};

    Size logical_size{};
    SDL_WindowID window_id{0};
    std::thread::id owner_thread;
    std::uint64_t revision{0};
};

Sdl3WindowBackend::Sdl3WindowBackend(Sdl3WindowBackendConfig config)
    : impl_{std::make_unique<Impl>(std::move(config))} {}

Sdl3WindowBackend::~Sdl3WindowBackend() {
    shutdown();
}

void Sdl3WindowBackend::initialize() {
    impl_->initialize();
}

void Sdl3WindowBackend::shutdown() noexcept {
    impl_->shutdown();
}

std::optional<Event> Sdl3WindowBackend::pollEvent() {
    return impl_->pollEvent();
}

bool Sdl3WindowBackend::isInitialized() const noexcept {
    return impl_->initialized();
}

Size Sdl3WindowBackend::windowSize() const {
    impl_->requireInitialized("windowSize");
    return impl_->logical_size;
}

Size Sdl3WindowBackend::pixelSize() const {
    impl_->requireInitialized("pixelSize");

    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(impl_->window, &width, &height)) {
        throwSdlError("SDL_GetWindowSizeInPixels");
    }

    return {
        checkedCoordinate(width, "window pixel width"),
        checkedCoordinate(height, "window pixel height")};
}

float Sdl3WindowBackend::displayScale() const {
    impl_->requireInitialized("displayScale");

    const float scale = SDL_GetWindowDisplayScale(impl_->window);
    if (!(scale > 0.0F) || !std::isfinite(scale)) {
        throwSdlError("SDL_GetWindowDisplayScale");
    }
    return scale;
}

bool Sdl3WindowBackend::presentationRequested() const noexcept {
    return impl_->presentation_requested;
}

void Sdl3WindowBackend::setTextInputEnabled(bool enabled) {
    impl_->requireInitialized("setTextInputEnabled");

    if (enabled == impl_->text_input_enabled) {
        return;
    }

    const bool success =
        enabled ? SDL_StartTextInput(impl_->window)
                : SDL_StopTextInput(impl_->window);
    if (!success) {
        throwSdlError(enabled ? "SDL_StartTextInput" : "SDL_StopTextInput");
    }

    impl_->text_input_enabled = enabled;
}

bool Sdl3WindowBackend::textInputEnabled() const noexcept {
    return impl_->text_input_enabled;
}

std::size_t Sdl3WindowBackend::presentFrame(const DisplayList& list) {
    impl_->requireInitialized("presentFrame");

    /*
     * SDL's window back buffer is not treated as persistent presentation state. Always clear before
     * replay and require callers to supply a complete frame. If command execution throws, Present is
     * skipped and presentation_requested remains true so the application can retry/recover.
     */
    impl_->render_context->clear(impl_->config.background_color);
    const std::size_t executed = DisplayListExecutor::execute(list, *this);

    if (!SDL_RenderPresent(impl_->renderer)) {
        throwSdlError("SDL_RenderPresent");
    }

    impl_->presentation_requested = false;
    return executed;
}

void Sdl3WindowBackend::fillRect(const FillRectCommand& command) {
    impl_->requireInitialized("fillRect");
    impl_->render_context->fillRect(command);
}

void Sdl3WindowBackend::strokeRect(const StrokeRectCommand& command) {
    impl_->requireInitialized("strokeRect");
    impl_->render_context->strokeRect(command);
}

void Sdl3WindowBackend::drawText(const DrawTextCommand& command) {
    impl_->requireInitialized("drawText");
    impl_->render_context->drawText(command);
}

Size Sdl3WindowBackend::measureText(std::string_view utf8_text) const {
    impl_->requireInitialized("measureText");
    return impl_->render_context->measureText(utf8_text);
}

Coordinate Sdl3WindowBackend::lineHeight() const noexcept {
    return impl_->render_context ? impl_->render_context->lineHeight() : 0;
}

std::optional<Coordinate> Sdl3WindowBackend::textAdvanceToScalar(
    std::string_view utf8_text,
    std::size_t scalar_index) const {
    impl_->requireInitialized("textAdvanceToScalar");
    return impl_->render_context->textAdvanceToScalar(utf8_text, scalar_index);
}

std::uint64_t Sdl3WindowBackend::revision() const noexcept {
    return impl_->revision;
}

} // namespace sasd::ui::rendered::sdl3
