#include "test_framework.hpp"

#include "win32_input_translation.hpp"

#include <sasd/ui/native/win32_backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>

#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::native::win32;
using namespace sasd::ui::native::win32::detail;

TEST_CASE("Win32 key translation preserves semantic identity and modifiers") {
    const auto event = translateKeyMessage(WM_KEYDOWN, 'C', KeyModifier::control);
    CHECK(event.has_value());
    CHECK(event->key == Key::c);
    CHECK(event->pressed);
    CHECK(event->modifiers == KeyModifier::control);

    const auto release = translateKeyMessage(WM_KEYUP, VK_LEFT, KeyModifier::shift);
    CHECK(release.has_value());
    CHECK(release->key == Key::left);
    CHECK(!release->pressed);
    CHECK(release->modifiers == KeyModifier::shift);

    CHECK(!translateKeyMessage(WM_KEYDOWN, VK_F24, KeyModifier::none).has_value());
}

TEST_CASE("Win32 modifier and coordinate translation is backend-neutral") {
    const auto modifiers = translateModifiers(MK_SHIFT | MK_CONTROL, KeyModifier::alt);
    CHECK(hasModifier(modifiers, KeyModifier::shift));
    CHECK(hasModifier(modifiers, KeyModifier::control));
    CHECK(hasModifier(modifiers, KeyModifier::alt));

    const LPARAM native_point = static_cast<LPARAM>(MAKELONG(static_cast<WORD>(-7),
                                                             static_cast<WORD>(23)));
    const auto point = translateClientPoint(native_point);
    CHECK(point.has_value());
    CHECK(point->x == -7);
    CHECK(point->y == 23);
}

TEST_CASE("Win32 pointer translation maps buttons, action, coordinates and modifiers") {
    const LPARAM native_point = static_cast<LPARAM>(MAKELONG(31, 17));
    const auto press = translatePointerMessage(WM_LBUTTONDOWN, MK_SHIFT, native_point);
    CHECK(press.has_value());
    CHECK(press->action == PointerAction::press);
    CHECK(press->button == PointerButton::primary);
    CHECK(press->position == Point{31, 17});
    CHECK(hasModifier(press->modifiers, KeyModifier::shift));

    const auto motion = translatePointerMessage(WM_MOUSEMOVE, MK_CONTROL, native_point,
                                                 KeyModifier::alt);
    CHECK(motion.has_value());
    CHECK(motion->action == PointerAction::move);
    CHECK(motion->button == PointerButton::none);
    CHECK(hasModifier(motion->modifiers, KeyModifier::control));
    CHECK(hasModifier(motion->modifiers, KeyModifier::alt));

    CHECK(!translatePointerMessage(WM_MOUSEWHEEL, 0, native_point).has_value());

    const auto leave = translatePointerSurfaceMessage(WM_MOUSELEAVE);
    CHECK(leave.has_value());
    CHECK(leave->action == PointerSurfaceAction::left);
}

TEST_CASE("Win32 text and window lifecycle translation fails closed") {
    // Keep the source literal ASCII-only so this test is independent of the MSVC source code page.
    const auto text = translateTextInput(L"Gr\u00FC\u00DFe");
    CHECK(text.has_value());
    CHECK(text->text == std::string{"Gr\xC3\xBC\xC3\x9F" "e"});
    CHECK(!translateTextInput(std::wstring{static_cast<wchar_t>(0xD800)}).has_value());

    const auto close = translateWindowMessage(WM_CLOSE, 0, 0);
    CHECK(close.has_value());
    CHECK(std::holds_alternative<QuitEvent>(*close));

    const auto resize = translateWindowMessage(WM_SIZE, SIZE_RESTORED,
                                                static_cast<LPARAM>(MAKELONG(640, 480)));
    CHECK(resize.has_value());
    CHECK(std::get<ResizeEvent>(*resize).size == Size{640, 480});
    CHECK(!translateWindowMessage(WM_SIZE, SIZE_MINIMIZED, 0).has_value());
}

TEST_CASE("Win32 host owns a hidden top-level window through deterministic lifecycle") {
    Win32WindowBackend backend{{"SASD Win32 test", {320, 200}, true, false}};
    CHECK(!backend.isInitialized());
    CHECK(backend.capabilities().pointer_input);
    backend.initialize();
    CHECK(backend.isInitialized());
    CHECK(backend.windowSize() == Size{320, 200});
    CHECK(!backend.pollEvent().has_value());
    backend.shutdown();
    CHECK(!backend.isInitialized());
}

TEST_CASE("Win32 host consumes the backend-neutral DisplayList and rendered metrics") {
    Win32WindowBackend backend{{"SASD Win32 rendered test", {320, 200}, true, false}};
    backend.initialize();

    rendered::DisplayList frame;
    frame.fillRect({0, 0, 320, 200}, Color::black);
    frame.strokeRect({8, 8, 180, 36}, Color::bright_white);
    frame.drawText({16, 16}, "Win32 Rendered", TextStyle{Color::bright_white});

    CHECK(backend.presentFrame(frame) == frame.size());
    CHECK(!backend.presentationRequested());
    CHECK(backend.measureText("SASD").width > 0);
    CHECK(backend.lineHeight() > 0);
    CHECK(backend.textAdvanceToScalar("SASD", 2).has_value());

    backend.shutdown();
}
