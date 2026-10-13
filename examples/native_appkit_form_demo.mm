#include <sasd/ui/native/appkit_backend.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <chrono>
#include <thread>

int main() {
    using namespace sasd::ui;
    native::appkit::AppKitWindowBackend backend{{"SASD UI Toolkit AppKit", {800, 500}, false, true,
                                                  12.0, Color::black}};
    backend.initialize();
    Window window;
    auto& label = window.emplace<Label>("SASD UI Toolkit - AppKit Rendered Host");
    auto& button = window.emplace<Button>("Button");
    auto& field = window.emplace<TextField>("TextField: committed UTF-8 input");
    window.arrange({0, 0, 800, 500});
    label.arrange({42, 52, 500, 24}); button.arrange({24, 130, 220, 42}); field.arrange({24, 190, 420, 42});
    rendered::DisplayList frame;
    rendered::RenderedPresentationSink presentation{frame, backend, Color::black};
    if (presentation.synchronize(window) != PresentationUpdateResult::synchronized) return 1;
    (void)backend.presentFrame(frame);
    for (;;) {
        const auto event = backend.pollEvent();
        if (event && std::holds_alternative<QuitEvent>(*event)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }
    backend.shutdown();
}
